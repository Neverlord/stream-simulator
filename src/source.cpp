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

#include "entity_details.hpp"

source::source(environment* env, QWidget* parent, QString name)
    : entity(env, parent, name) {
  // nop
}

source::~source() {
  // nop
}

void source::start() {
  dialog_->drop_sink_widgets();
  dialog_->drop_stage_widgets();
  stream_manager_ = simulant_->make_source(
    consumers_.front(),
    [](caf::unit_t&) {
      // nop
    },
    [=](caf::unit_t&, caf::downstream<int>& out, size_t n) {
      progress(dialog_->batch_generation, 0, static_cast<int>(n), [&](int i) {
        progress(dialog_->item_generation, 1, val(dialog_->rate));
        out.push(i);
      });
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

void source::add_consumer(caf::actor consumer) {
  consumers_.emplace_back(std::move(consumer));
}
