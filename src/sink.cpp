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

#include "sink.hpp"

#include "caf/stream.hpp"

#include "environment.hpp"

sink::sink(environment* env, QObject* parent, QString name)
    : entity(env, parent, name) {
  init(credit_per_interval_, name + "CreditPerInterval");
  init(ticks_per_item_, name + "TicksPerItem");
  init(batch_size_, name + "BatchSize");
  init(item_progress_, name + "ItemProgress");
  init(batch_progress_, name + "BatchProgress");
  init(mailbox_, name + "Mailbox");
  init(current_sender_, name + "CurrentSender");
  init(pending_, name + "Pending");
  init(pending_avg_, name + "PendingAVG");
  init(latency_, name + "Latency");
  init(latency_avg_, name + "LatencyAVG");
}

sink::~sink() {
  // nop
}

void sink::start() {
  simulant_->become(
    [=](const caf::stream<int>& in) {
      if (!simulant_->streams().empty()) {
        auto& sm = simulant_->current_mailbox_element()->content().get_as<caf::stream_msg>(0);
        auto& op = caf::get<caf::stream_msg::open>(sm.content);
        auto ptr = simulant_->streams().begin()->second;
        ptr->add_source(in.id(), op.prev_stage, op.original_stage, op.priority, op.redeployable, caf::response_promise{});
        simulant_->streams().emplace(in.id(), ptr);
        return;
      }
      simulant_->make_sink(
        in,
        [](caf::unit_t&) {
          // nop
        },
        [=](caf::unit_t&, int) {
          if (state_ != consume_batch) {
            state_ = consume_batch;
            text(current_sender_,
                 env_->id_by_handle(simulant_->current_sender()));
            reset(batch_progress_, 0, 1);
          } else {
            inc_max(batch_progress_);
          }
        },
        [](caf::unit_t&) {
          // nop
        }
      );
    }
  );
  simulant_->model()->update();
}

void sink::tick() {
  switch (state_) {
    default:
      break;
    case read_mailbox:
      state_ = idle;
      resume_simulant();
      break;
    case consume_batch:
      consume_batch_impl();
      break;
  }
  if (last_state_ != state_ && state_ == consume_batch) {
    val(batch_size_, max(batch_progress_));
  } else if (last_state_ == idle && state_ == idle && at_max(batch_progress_)) {
    val(batch_progress_, 0);
    val(batch_size_, 0);
  }
}

bool sink::consume_batch_impl() {
  inc(item_progress_);
  if (at_max(item_progress_)) {
    //upstream_[text(current_sender_)].rate += 1;
    reset(item_progress_, 0, val(ticks_per_item_));
    inc(batch_progress_);
    if (at_max(batch_progress_)) {
      state_ = idle;
    }
    return true;
  }
  return false;
}
