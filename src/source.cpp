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

#include "source.hpp"

#include "caf/stream.hpp"

source::source(environment* env, QWidget* parent, QString name)
    : entity(env, parent, name) {
  init(weight_, name + "Weight");
  init(rate_, name + "Rate");
  init(credit_, name + "Credit");
  init(item_generation_, name + "ItemGeneration");
  init(send_progress_, name + "SendProgress");
  init(send_timeout_, name + "SendTimeout");
  init(max_batch_latency_, "maxBatchLatency");
}

source::~source() {
  // nop
}

void source::start() {
  stream_manager_ = simulant_->make_source(
    consumers_.front(),
    [](caf::unit_t&) {
      // nop
    },
    [=](caf::unit_t&, caf::downstream<int>& out, size_t n) {
      //val(credit_, static_cast<int>(out().credit()));
      if (at_max(send_progress_)) {
        auto batch_size = val(send_progress_);
        assert(batch_size > 0);
        assert(batch_size <= static_cast<int>(n));
        for (int i = 0; i < batch_size; ++i)
          out.push(i);
      }
    },
    [](const caf::unit_t&) -> bool {
      return false;
    },
    [](caf::expected<void>) {
      // nop
    }
  ).ptr();
  // TODO: add remaining consumers to the stream.
  simulant_->model()->update();
}

void source::tick() {
  switch (state_) {
    default:
      break;
    case idle: {
      auto credit = out().credit();
      auto buffered = out().buffered();
      auto diff = credit - buffered;
      if (diff >= out().min_batch_size()) {
        reset(item_generation_, 0, val(rate_));
        reset(send_progress_, 0, std::min(diff, out().max_batch_size()));
        state_ = produce_batch;
      }
      break;
    }
    case read_mailbox:
      state_ = idle;
      resume_simulant();
      break;
    case produce_batch:
      produce_batch_impl();
      break;
  }
}

void source::tock() {
  // nop
}

caf::stream_scatterer& source::out() {
  return stream_manager_->out();
}

void source::add_consumer(caf::actor consumer) {
  consumers_.emplace_back(std::move(consumer));
}

void source::produce_batch_impl() {
  inc(item_generation_);
  if (at_max(item_generation_)) {
    inc(send_progress_);
    if (at_max(send_progress_)) {
      stream_manager_->generate_messages();
      //out().emit_batches();
      val(send_progress_, 0);
      // Return to idle state to check in the next call to before_tick
      // whether we need to read from the mailbox.
      state_ = idle;
    }
    reset(item_generation_, 0, val(rate_));
  }
}
