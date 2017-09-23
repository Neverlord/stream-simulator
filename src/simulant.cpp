#include "simulant.hpp"

#include <string>
#include <iostream>

#include "caf/stream.hpp"
#include "caf/stream_manager.hpp"

#include "caf/detail/sync_request_bouncer.hpp"

#include "qstr.hpp"
#include "entity.hpp"
#include "environment.hpp"

simulant::simulant(caf::actor_config& cfg, entity* parent)
  : caf::scheduled_actor(cfg),
    parent_(parent),
    model_(this) {
  // nop
}

void simulant::enqueue(caf::mailbox_element_ptr ptr, caf::execution_unit*) {
  CAF_ASSERT(ptr != nullptr);
  CAF_ASSERT(!getf(is_blocking_flag));
  CAF_LOG_TRACE(CAF_ARG(*ptr));
  CAF_LOG_SEND_EVENT(ptr);
  auto mid = ptr->mid;
  auto sender = ptr->sender;
  switch (mailbox().enqueue(ptr.release())) {
    case caf::detail::enqueue_result::unblocked_reader: {
      CAF_LOG_ACCEPT_EVENT(true);
      parent_->render_mailbox();
      break;
    }
    case caf::detail::enqueue_result::queue_closed: {
      CAF_LOG_REJECT_EVENT();
      if (mid.is_request()) {
        caf::detail::sync_request_bouncer f{caf::exit_reason::unknown};
        f(sender, mid);
      }
      break;
    }
    case caf::detail::enqueue_result::success:
      // enqueued to a running actors' mailbox; nothing to do
      CAF_LOG_ACCEPT_EVENT(false);
      parent_->render_mailbox();
      break;
  }
}

namespace {

qlonglong qt_fwd(simulant*, long x) {
  return static_cast<qlonglong>(x);
}

QString qt_fwd(simulant* self, caf::actor_addr x) {
  return self->parent()->env()->id_by_handle(x);
}

QString qt_fwd(simulant* self, caf::strong_actor_ptr x) {
  return self->parent()->env()->id_by_handle(x);
}

QString qt_fwd(simulant* self, caf::stream_id x) {
  auto result = qt_fwd(self, x.origin);
  result += ":";
  result += QString::number(x.nr);
  return result;
}

QString qt_fwd(simulant*, caf::stream_priority x) {
  return QString::fromStdString(to_string(x));
}

template <class T>
T qt_fwd(simulant*, T x) {
  return x;
}

} // namespace <anonymous>

// Put a field with a member function getter.
#define PUT_MF(var, field) pt.put(qstr(#field), qt_fwd(this, var.field()))

// Put a field with a member variable.
#define PUT_MV(var, field) pt.put(qstr(#field), qt_fwd(this, (var).field))

class scoped_path_entry {
public:
  scoped_path_entry(simulant_tree_item& root, std::vector<QString>& path,
                    QString id, QVariant val)
      : path_(&path) {
    path.emplace_back(id);
    root.insert_or_update(path, val);
  }

  scoped_path_entry(scoped_path_entry&& other) : path_(other.path_) {
    other.path_ = nullptr;
  }

  ~scoped_path_entry() {
    if (path_)
      path_->pop_back();
  }

private:
  std::vector<QString>* path_;
};

class path_traverser {
public:
  path_traverser(simulant_tree_item& root) : root_(root) {
    // nop
  }

  scoped_path_entry enter(QString id, QVariant val) {
    return {root_, path_, std::move(id), std::move(val)};
  }

  void put(QString id, QVariant val) {
    enter(id, val);
  }

private:
  simulant_tree_item& root_;
  std::vector<QString> path_;
};

void simulant::serialize_state(simulant_tree_item& root) {
  path_traverser pt{root};
  // Mark tree as stale.
  root.mark_as_stale();
  // Update the tree.
  { // lifetime scope of streams entry
    auto streams_entry = pt.enter(qstr("streams"), qstr("<list:stream>"));
    for (auto& x : streams()) {
      auto mgr = x.second;
      auto stream_entry = pt.enter(qstr(mgr.get()), qstr("<stream>"));
      { // lifetime scope of in
        auto& in = mgr->in();
        auto in_entry = pt.enter(qstr("in"), qstr("<stream_gatherer>"));
        PUT_MF(in, continuous);
        PUT_MF(in, high_watermark);
        PUT_MF(in, min_credit_assignment);
        PUT_MF(in, max_credit);
        auto paths_entry = pt.enter(qstr("paths"), qstr("<list:inbound_path>"));
        for (long path_id = 0; path_id < in.num_paths(); ++path_id) {
          auto path = in.path_at(path_id);
          auto path_entry = pt.enter(qstr(path), qstr("<inbound_path>"));
          PUT_MV(*path, sid);
          PUT_MV(*path, hdl);
          PUT_MV(*path, prio);
          PUT_MV(*path, last_acked_batch_id);
          PUT_MV(*path, last_batch_id);
          PUT_MV(*path, assigned_credit);
          PUT_MV(*path, redeployable);
        }
      }
      { // lifetime scope of out
        auto& out = mgr->out();
        auto out_entry = pt.enter(qstr("out"), qstr("<stream_scatterer"));
        PUT_MF(out, continuous);
        PUT_MF(out, credit);
        PUT_MF(out, buffered);
        PUT_MF(out, min_batch_size);
        PUT_MF(out, max_batch_size);
        PUT_MF(out, min_buffer_size);
        auto paths_entry = pt.enter(qstr("paths"), qstr("<list:outbound_path>"));
        for (long path_id = 0; path_id < out.num_paths(); ++path_id) {
          auto path = out.path_at(path_id);
          auto path_entry = pt.enter(qstr(path), qstr("<outbound_path>"));
          PUT_MV(*path, sid);
          PUT_MV(*path, hdl);
          PUT_MV(*path, next_batch_id);
          PUT_MV(*path, open_credit);
          PUT_MV(*path, redeployable);
          PUT_MV(*path, next_ack_id);
        }
      }
    }
  } // leave streams entry
  // Remove any leaf that hasen't been updated.
  root.purge();
}

void intrusive_ptr_add_ref(simulant* p) {
  intrusive_ptr_add_ref(p->ctrl());
}

void intrusive_ptr_release(simulant* p) {
  intrusive_ptr_release(p->ctrl());
}
