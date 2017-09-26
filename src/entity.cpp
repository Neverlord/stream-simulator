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
#include "entity_details.hpp"

entity::entity(environment* env, QWidget* parent, QString name)
  : env_(env),
    parent_(parent),
    name_(name),
    simulant_thread_state_(sts_none),
    state_(idle),
    last_state_(idle) {
  using storage = caf::actor_storage<simulant>;
  auto& sys = env->sys();
  caf::actor_config cfg;
  auto ptr = new storage(sys.next_actor_id(), sys.node(), &sys, cfg, this);
  simulant_.reset(&ptr->data, false);
  // Parent takes ownership of dialog_.
  dialog_ = new entity_details(parent);
  dialog_->id->setText(name);
  dialog_->state->setModel(simulant_->model());
}

entity::~entity() {
  if (simulant_thread_state_ != sts_none) {
    { // lifetime scope of guard
      std::unique_lock<std::mutex> guard{simulant_mx_};
      simulant_thread_state_ = sts_abort;
      simulant_resume_cv_.notify_one();
    }
    simulant_thread_.join();
  }
}

void entity::before_tick() {
  if (state_ == idle && mailbox_ready())
    state_ = read_mailbox;
}

void entity::tick() {
  switch (state_) {
    default:
      break;
    case read_mailbox:
      state_ = idle;
      start_handling_next_message();
      break;
    case resume_simulant:
      resume();
  }
}

void entity::after_tick() {
  simulant_->model()->update();
  last_state_ = state_;
  if (refresh_mailbox_) {
    refresh_mailbox_ = false;
    render_mailbox();
  }
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
  auto lw = dialog_->mailbox;
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

void entity::show_dialog() {
  if (!dialog_->isVisible()) {
    dialog_->show();
    dialog_->raise();
    dialog_->activateWindow();
  }
}

void entity::progress(QProgressBar* bar, int first, int last) {
  progress(bar, first, last, [](int) {});
}

void entity::yield() {
  simulant_thread_state_ = sts_yield;
  simulant_yield_cv_.notify_one();
  simulant_thread_state st;
  do {
    simulant_resume_cv_.wait(*simulant_resume_guard_);
    st = simulant_thread_state_.load();
  } while (st == sts_yield);
  if (st == sts_abort)
    throw std::runtime_error("aborted");
}

void entity::resume() {
  std::unique_lock<std::mutex> guard{simulant_mx_};
  // Wait for the simulant in case it is still running.
  while (simulant_thread_state_ == sts_resume)
    simulant_yield_cv_.wait(guard);
  simulant_thread_state_ = sts_resume;
  simulant_resume_cv_.notify_one();
  simulant_yield_cv_.wait(guard);
  if (simulant_thread_state_ == sts_finalize) {
    simulant_thread_.join();
    simulant_thread_ = std::thread{};
    simulant_thread_state_ = sts_none;
    state_ = idle;
  } else if (state_ != resume_simulant) {
    state_ = resume_simulant;
  }
  //dialog_->update();
}

void entity::start_handling_next_message() {
  if (!mailbox_ready())
    return;
  delete dialog_->mailbox->takeItem(0);
  assert(simulant_thread_state_ == sts_none);
  simulant_thread_ = std::thread{[=] {
    std::unique_lock<std::mutex> guard{simulant_mx_};
    while (simulant_thread_state_ != sts_resume)
      simulant_resume_cv_.wait(guard);
    simulant_resume_guard_ = &guard;
    simulant_->resume(env_->sys().dummy_execution_unit(), 1);
    simulant_thread_state_ = sts_finalize;
    simulant_yield_cv_.notify_one();
  }};
  resume();
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
