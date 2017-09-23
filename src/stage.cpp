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

#include "stage.hpp"

#include <QLabel>
#include <QTreeView>

#include "caf/stream.hpp"

#include "environment.hpp"

stage::stage(environment* env, QWidget* parent, QString name)
    : entity(env, parent, name),
      source(env, parent, name),
      sink(env, parent, name),
      completed_items_(0) {
  init(ratio_in_, name + "RatioIn");
  init(ratio_out_, name + "RatioOut");
  init(output_buffer_, name + "OutputBuffer");
  // Hide widgets inherited from source that aren't used.
  item_generation_->hide();
  QLabel* lbl1 = nullptr;
  init(lbl1, name + "ItemGenerationLabel");
  lbl1->hide();
  rate_->hide();
  QLabel* lbl2 = nullptr;
  init(lbl2, name + "RateLabel");
  lbl2->hide();
}

stage::~stage() {
  // nop
}

void stage::start() {
  simulant_->become(
    [=](const caf::stream<int>& in) {
      auto& stages = simulant_->current_mailbox_element()->stages;
      stages.insert(stages.begin(),
                    caf::actor_cast<caf::strong_actor_ptr>(consumers_.front()));
      if (!simulant_->streams().empty()) {
        auto& sm = simulant_->current_mailbox_element()->content().get_as<caf::stream_msg>(0);
        auto& op = caf::get<caf::stream_msg::open>(sm.content);
        auto ptr = simulant_->streams().begin()->second;
        ptr->add_source(in.id(), op.prev_stage, op.original_stage, op.priority, op.redeployable, caf::response_promise{});
        simulant_->streams().emplace(in.id(), ptr);
        return;
      }
      stream_manager_ = simulant_->make_stage(
        in,
        [](caf::unit_t&) {
          // nop
        },
        [=](caf::unit_t&, caf::downstream<int>& out, int) {
          if (out_ == nullptr)
            out_.reset(new caf::downstream<int>(out.buf()));
          if (state_ != consume_batch) {
            state_ = consume_batch;
            text(current_sender_,
                 env_->id_by_handle(simulant_->current_sender()));
            enable(batch_progress_);
            reset(batch_progress_, 0, 1);
          } else {
            inc_max(batch_progress_);
          }
        },
        [](caf::unit_t&) {
          // nop
        }
      ).ptr();
      val(credit_, static_cast<int>(stream_manager_->out().credit()));
    }
  );
  simulant_->model()->update();
}

void stage::before_tick() {
  if (state_ == idle && mailbox_ready())
    state_ = read_mailbox;
}

void stage::tick() {
  switch (state_) {
    default:
      break;
    case read_mailbox:
      state_ = idle;
      resume_simulant();
      break;
    case consume_batch:
      if (consume_batch_impl()) {
        ++completed_items_;
        if (completed_items_ >= val(ratio_in_)) {
          completed_items_ = 0;
          for (auto i = 0; i < val(ratio_out_); ++i)
            out_->push(i);
        }
        // State transition only occurrs when a batch has been completed.
        if (state_ == idle)
          stream_manager_->out().emit_batches();
      }
      break;
  }
  if (last_state_ != state_ && state_ == consume_batch) {
    val(batch_size_, max(batch_progress_));
  } else if (last_state_ == idle && state_ == idle && at_max(batch_progress_)) {
    val(batch_progress_, 0);
    val(batch_size_, 0);
  }
}

void stage::after_tick() {
  simulant_->model()->update();
}

void stage::tock() {
  sink::tock();
  source::tock();
}
