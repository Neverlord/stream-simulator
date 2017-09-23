/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2015                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include "entity.hpp"

#include <QSpinBox>
#include <QTreeView>
#include <QListWidget>

#include "qstr.hpp"
#include "environment.hpp"

entity::entity(environment* env, QWidget* parent, QString name)
  : env_(env),
    parent_(parent),
    name_(name),
    state_(idle),
    last_state_(idle) {
  using storage = caf::actor_storage<simulant>;
  auto& sys = env->sys();
  caf::actor_config cfg;
  auto ptr = new storage(sys.next_actor_id(), sys.node(), &sys, cfg, this);
  simulant_.reset(&ptr->data, false);
  auto tv = parent_->findChild<QTreeView*>(name_ + "StateTree");
  if (tv) {
    tv->setModel(simulant_->model());
  }
}

entity::~entity() {
  // nop
}

void entity::before_tick() {
  if (state_ == idle && mailbox_ready())
    state_ = read_mailbox;
}

void entity::after_tick() {
  simulant_->model()->update();
  last_state_ = state_;
}

void entity::tock() {
  // nop
}

simulant_tree_model* entity::model() {
  return simulant_->model();
}

int entity::tick_time() const {
  return parent_->findChild<QSpinBox*>("ticks")->value();
}

namespace {

struct render_mailbox_visitor {
  QListWidget* lw;
  QString from;

  void operator()(const caf::stream_msg::open& x) {
    lw->addItem(qstr("From: %1 -> open with priority %2")
                .arg(from)
                .arg(qstr(to_string(x.priority))));
  }

  void operator()(const caf::stream_msg::ack_open& x) {
    lw->addItem(qstr("From: %1 -> ack_open with %2 credit")
                .arg(from)
                .arg(qstr(x.initial_demand)));
  }

  void operator()(const caf::stream_msg::batch& x) {
    lw->addItem(qstr("From: %1 -> batch #%2 of size %3")
                .arg(from)
                .arg(qstr(x.id))
                .arg(qstr(x.xs_size)));
  }

  void operator()(const caf::stream_msg::ack_batch& x) {
    lw->addItem(qstr("From: %1 -> ack_batch #%2 with %3 new credit")
                .arg(from)
                .arg(qstr(x.acknowledged_id))
                .arg(qstr(x.new_capacity)));
  }

  template <class T>
  void operator()(const T& x) {
    lw->addItem(qstr("From: %1 -> %2").arg(from).arg(qstr(to_string(x))));
  }
};

} // namespace <anonymous>

void entity::render_mailbox() {
  using namespace caf;
  QListWidget* lw = nullptr;
  init(lw, name_ + "Mailbox");
  lw->setUpdatesEnabled(false);
  lw->clear();
  simulant_->iterate_mailbox([&](mailbox_element& x) {
    if (x.content().match_elements<stream_msg>()) {
      auto& sm = x.content().get_as<stream_msg>(0);
      render_mailbox_visitor f{lw, env_->entity_by_handle(sm.sender)->id()};
      visit(f, sm.content);
    } else {
      lw->addItem(qstr(to_string(x.content())));
    }
  });
  lw->setUpdatesEnabled(true);
}

caf::actor entity::handle() {
  return caf::actor_cast<caf::actor>(simulant_->ctrl());
}

bool entity::mailbox_ready() {
  return !simulant_->mailbox().closed() && !simulant_->mailbox().empty();
}

void entity::resume_simulant() {
  if (!mailbox_ready())
    return;
  simulant_->resume(env_->sys().dummy_execution_unit(), 1);
  render_mailbox();
}

namespace {

static const char* state_strings[] = {
  "idle",
  "read_mailbox",
  "consume_batch",
  "produce_batch"
};

} // namespace <anonymous>

const char* to_string(entity::state_t x) {
  return state_strings[static_cast<size_t>(x)];
}
