#include "simulant.hpp"

#include <string>
#include <iostream>

#include "caf/stream.hpp"
#include "caf/stream_manager.hpp"
#include "caf/mailbox_element.hpp"

#include "caf/detail/sync_request_bouncer.hpp"

#include "qstr.hpp"
#include "entity.hpp"
#include "environment.hpp"
#include "term_gatherer.hpp"

namespace {

caf::error silent_exception_handler(caf::scheduled_actor*,
                                    std::exception_ptr&) {
  return caf::sec::runtime_error;                                                    
}

} // namespace <anonymous>

simulant::simulant(caf::actor_config& cfg, entity* parent)
  : caf::scheduled_actor(cfg),
    env_(parent->env()),
    parent_(parent),
    model_(this, parent->id()),
    msg_ids_(0) {
  set_exception_handler(silent_exception_handler);
}

void simulant::enqueue(caf::mailbox_element_ptr ptr, caf::execution_unit*) {
  CAF_ASSERT(ptr != nullptr);
  CAF_ASSERT(!getf(is_blocking_flag));
  CAF_LOG_TRACE(CAF_ARG(*ptr));
  CAF_LOG_SEND_EVENT(ptr);
  // Instead of delivering the message directly, we first give it an ID and put
  // it into the environment's in-flight queue. The environment then
  // re-enqueues the mailbox element at a later time.
  auto local_mid = peek_pending_message(ptr.get());
  if (local_mid == 0) {
    local_mid = push_pending_message(ptr.get());
    auto self = caf::strong_actor_ptr{ctrl()};
    env_->post_f(-1, [=, me = std::move(ptr)](tick_time) mutable {
      self->enqueue(std::move(me), nullptr);
    });
    return;
  }
  auto msg = ptr->copy_content_to_message();
  auto sender = ptr->sender;
  super::enqueue(std::move(ptr), nullptr);
  critical_section(parent_mtx_, [&] {
    auto pptr = parent_.load();
    if (pptr)
      emit pptr->message_received(local_mid, sender, msg);
  });
}

caf::invoke_message_result simulant::consume(caf::mailbox_element& x) {
  auto local_mid = pop_pending_message(&x);
  env_->post_f([=](tick_time) {
    critical_section(parent_mtx_, [&] {
      auto pptr = parent_.load();
      if (pptr)
        emit pptr->message_consumed(local_mid);
    });
  });
  return super::consume(x);
}

namespace {

qlonglong qt_fwd(environment*, long x) {
  return static_cast<qlonglong>(x);
}

QString qt_fwd(environment* env, caf::actor_addr x) {
  return env->id_by_handle(x);
}

QString qt_fwd(environment* env, caf::strong_actor_ptr x) {
  return env->id_by_handle(x);
}

QString qt_fwd(environment* env, caf::stream_id x) {
  auto result = qt_fwd(env, x.origin);
  result += ":";
  result += QString::number(x.nr);
  return result;
}

QString qt_fwd(environment*, caf::stream_priority x) {
  return QString::fromStdString(to_string(x));
}

template <class T>
T qt_fwd(environment*, T x) {
  return x;
}

} // namespace <anonymous>

// Put a field with a member function getter.
#define PUT_MF(var, field) pt.put(qstr(#field), qt_fwd(env_, var.field()))

// Put a field with a member variable.
#define PUT_MV(var, field) pt.put(qstr(#field), qt_fwd(env_, (var).field))

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
        auto tg = dynamic_cast<term_gatherer*>(&in);
        if (tg != nullptr) {
          PUT_MV(*tg, min_tokens_);
          PUT_MV(*tg, desired_batch_complexity_);
          PUT_MV(*tg, last_cycle_);
          PUT_MV(*tg, last_token_count_);
          PUT_MV(*tg, historic_time_per_item_);
          PUT_MV(*tg, processing_time_);
          PUT_MV(*tg, processed_items_);
        }
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
        auto out_entry = pt.enter(qstr("out"), qstr("<stream_scatterer>"));
        PUT_MF(out, continuous);
        PUT_MF(out, credit);
        PUT_MF(out, buffered);
        PUT_MF(out, min_batch_size);
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

void simulant::detach_from_parent() {
  parent_ = nullptr;
}

int simulant::push_pending_message(caf::mailbox_element* ptr) {
  auto id = ++msg_ids_;
  critical_section(pending_messages_mtx_, [&] {
    pending_messages_.emplace(ptr, id);
  });
  return id;
}

int simulant::pop_pending_message(caf::mailbox_element* ptr) {
  return critical_section(pending_messages_mtx_, [&] {
    auto i = pending_messages_.find(ptr);
    if (i == pending_messages_.end())
      return 0;
    auto res = i->second;
    pending_messages_.erase(i);
    return res;
  });
}

int simulant::peek_pending_message(caf::mailbox_element* ptr) {
  return critical_section(pending_messages_mtx_, [&] {
    auto i = pending_messages_.find(ptr);
    return (i == pending_messages_.end()) ? 0 : i->second;
  });
}

void intrusive_ptr_add_ref(simulant* p) {
  intrusive_ptr_add_ref(p->ctrl());
}

void intrusive_ptr_release(simulant* p) {
  intrusive_ptr_release(p->ctrl());
}
