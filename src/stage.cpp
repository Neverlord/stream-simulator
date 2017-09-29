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
#include "entity_details.hpp"
#include "qstr.hpp"

stage::stage(environment* env, QWidget* parent, QString name)
    : entity(env, parent, name),
      source(env, parent, name),
      sink(env, parent, name),
      completed_items_(0) {
  // nop
}

stage::~stage() {
  // nop
}

void stage::start() {
  dialog_->drop_source_only_widgets();
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
          if (!started_)
            started_ = true;
          if (text(dialog_->sink_current_sender).isEmpty()) {
            auto me = simulant_->current_mailbox_element();
            text(dialog_->sink_current_sender, env_->id_by_handle(me->sender));
            auto& sm = me->content().get_as<caf::stream_msg>(0);
            auto& op = caf::get<caf::stream_msg::batch>(sm.content);
            max(dialog_->sink_batch_progress, static_cast<int>(op.xs_size));
            yield();
          }
          progress(dialog_->sink_item_progress, 0, val(dialog_->sink_ticks_per_item));
          inc(dialog_->sink_batch_progress);
          if (++completed_items_ >= val(dialog_->ratio_in)) {
            completed_items_ = 0;
            for (int i = 0; i < val(dialog_->ratio_out); ++i)
              out.push(i);
          }
          if (at_max(dialog_->sink_batch_progress)) {
            text(dialog_->sink_current_sender, qstr(""));
            reset(dialog_->sink_batch_progress, 0, 1);
          }
        },
        [](caf::unit_t&) {
          // nop
        }
      ).ptr();
    }
  );
  // Run initialization code of the simulant and update state model.
  simulant_->activate(env_->sys().dummy_execution_unit());
  simulant_->model()->update();
}
