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
#include "entity_details.hpp"
#include "qstr.hpp"

sink::sink(environment* env, QWidget* parent, QString name)
    : entity(env, parent, name) {
  // nop
}

sink::~sink() {
  // nop
}

void sink::start() {
  dialog_->drop_stage_widgets();
  dialog_->drop_source_widgets();
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
          if (text(dialog_->current_sender).isEmpty()) {
            auto me = simulant_->current_mailbox_element();
            text(dialog_->current_sender, env_->id_by_handle(me->sender));
            auto& sm = me->content().get_as<caf::stream_msg>(0);
            auto& op = caf::get<caf::stream_msg::batch>(sm.content);
            max(dialog_->batch_progress, static_cast<int>(op.xs_size));
            yield();
          }
          progress(dialog_->item_progress, 0, val(dialog_->ticks_per_item));
          inc(dialog_->batch_progress);
          if (at_max(dialog_->batch_progress)) {
            yield();
            text(dialog_->current_sender, qstr(""));
            reset(dialog_->batch_progress, 0, 1);
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
