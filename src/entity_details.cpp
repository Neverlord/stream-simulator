#include "entity_details.hpp"

#include "entity.hpp"
#include "source.hpp"
#include "sink.hpp"
#include "stage.hpp"
#include "qstr.hpp"
#include "environment.hpp"

namespace {

struct render_mailbox_visitor {
  QListWidget* lw;
  QString from;
  int id;

  void operator()(const caf::stream_msg::open& x) {
    add(qstr("From: %1 -> open with priority %2")
        .arg(from)
        .arg(qstr(to_string(x.priority))));
  }

  void operator()(const caf::stream_msg::ack_open& x) {
    add(qstr("From: %1 -> ack_open with %2 credit")
        .arg(from)
        .arg(qstr(x.initial_demand)));
  }

  void operator()(const caf::stream_msg::batch& x) {
    add(qstr("From: %1 -> batch #%2 of size %3")
        .arg(from)
        .arg(qstr(x.id))
        .arg(qstr(x.xs_size)));
  }

  void operator()(const caf::stream_msg::ack_batch& x) {
    add(qstr("From: %1 -> ack_batch #%2 with %3 new credit")
        .arg(from)
        .arg(qstr(x.acknowledged_id))
        .arg(qstr(x.new_capacity)));
  }

  template <class T>
  void operator()(const T& x) {
    add(qstr("From: %1 -> %2").arg(from).arg(qstr(to_string(x))));
  }

  void add(QString x) {
    auto item = new QListWidgetItem;
    item->setText(x);
    item->setData(Qt::UserRole, id);
    lw->insertItem(lw->count(), item); // takes ownership
  }
};

} // namespace <anonymous>

entity_details::entity_details(entity* ptr)
    : QDialog(ptr->parent()),
      env_(ptr->env()) {
  setupUi(this);
  connect(ptr, SIGNAL(idling()), sink_idle_ticks, SLOT(stepUp()));
  connect(
    ptr, SIGNAL(message_received(int, caf::strong_actor_ptr, caf::message)),
    SLOT(entity_received_message(int, caf::strong_actor_ptr, caf::message)));
  connect(ptr, SIGNAL(message_consumed(int)),
          SLOT(entity_consumed_message(int)));
}

entity_details::~entity_details() {
  // nop
}

void entity_details::drop_sink_widgets() {
  drop_by_prefix(qstr("sink"));
}

void entity_details::drop_stage_widgets() {
  drop_by_prefix(qstr("stage"));
}

void entity_details::drop_source_widgets() {
  drop_by_prefix(qstr("source"));
}

void entity_details::drop_source_only_widgets() {
  QObject* lst[] = {source_rate_label, source_rate,
                    source_item_generation_label, source_item_generation};
  for (auto obj : lst)
    obj->deleteLater();
}

void entity_details::entity_received_message(int id, caf::strong_actor_ptr from,
                                             caf::message content) {
  render_mailbox_visitor v{mailbox, env_->id_by_handle(from), id};
  if (content.match_elements<caf::stream_msg>()) {
    auto& sm = content.get_as<caf::stream_msg>(0);
    caf::visit(v, sm.content);
  } else {
    v(content);
  }
}

void entity_details::entity_consumed_message(int id) {
  for (int row = 0; row < mailbox->count(); ++row) {
    auto item = mailbox->item(row);
    if (item->data(Qt::UserRole) == id) {
      delete item;
      return;
    }
  }
}

void entity_details::drop_by_prefix(const QString& prefix) {
  for (auto obj : children())
    if (obj->objectName().startsWith(prefix))
      obj->deleteLater();
}
