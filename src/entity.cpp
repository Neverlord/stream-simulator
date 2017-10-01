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
#include "critical_section.hpp"

namespace {

class cancel_entity_thread : public std::exception {
public:
  const char* what() const noexcept override {
    return "entity aborted thread execution";
  }
};

} // namespace <anonymous>

entity::entity(environment* env, QWidget* parent, QString name)
  : env_(env),
    parent_(parent),
    name_(name),
    simulant_thread_state_(sts_none),
    state_(idle),
    before_tick_state_(idle) {
  using storage = caf::actor_storage<simulant>;
  auto& sys = env->sys();
  caf::actor_config cfg;
  auto ptr = new storage(sys.next_actor_id(), sys.node(), &sys, cfg, this);
  simulant_.reset(&ptr->data, false);
  // Parent takes ownership of dialog_.
  dialog_ = new entity_details(this);
  dialog_->setWindowTitle(name);
  dialog_->state->setModel(simulant_->model());
}

entity::~entity() {
  if (simulant_thread_state_ != sts_none) {
    critical_section(simulant_mx_, [&] {
      simulant_thread_state_ = sts_abort;
      simulant_resume_cv_.notify_one();
    });
    simulant_thread_.join();
  }
  simulant_->detach_from_parent();
}

void entity::before_tick() {
  if (state_ == idle && mailbox_ready())
    state_ = read_mailbox;
  before_tick_state_ = state_;
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
  if (started_ && before_tick_state_ == idle && state_ == idle)
    emit idling();
}

void entity::tock() {
  // nop
}

simulant_tree_model* entity::model() {
  return simulant_->model();
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
    throw cancel_entity_thread();
}

void entity::resume() {
  critical_section(simulant_mx_, [&](auto& guard) {
    // Wait for the simulant in case it is still running.
    while (simulant_thread_state_ == sts_resume)
      simulant_yield_cv_.wait(guard);
    simulant_thread_state_ = sts_resume;
    simulant_resume_cv_.notify_one();
    do {
      simulant_yield_cv_.wait(guard);
    } while (simulant_thread_state_ == sts_resume);
    if (simulant_thread_state_ == sts_finalize) {
      simulant_thread_.join();
      simulant_thread_ = std::thread{};
      simulant_thread_state_ = sts_none;
      state_ = idle;
    } else if (state_ != resume_simulant) {
      state_ = resume_simulant;
    }
  });
}

void entity::start_handling_next_message() {
  if (!mailbox_ready())
    return;
  delete dialog_->mailbox->takeItem(0);
  assert(simulant_thread_state_ == sts_none);
  simulant_thread_ = std::thread{[=] {
    critical_section(simulant_mx_, [&](auto& guard) {
      while (simulant_thread_state_ != sts_resume)
        simulant_resume_cv_.wait(guard);
      simulant_resume_guard_ = &guard;
      try {
        simulant_->resume(env_->sys().dummy_execution_unit(), 1);
        simulant_thread_state_ = sts_finalize;
        simulant_yield_cv_.notify_one();
      } catch (cancel_entity_thread&) {
        // nop
      }
    });
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
