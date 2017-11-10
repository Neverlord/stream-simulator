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

#include <QWidget>

#include "caf/stream.hpp"
#include "caf/terminal_stream_scatterer.hpp"

#include "caf/policy/arg.hpp"

#include "entity_details.hpp"
#include "environment.hpp"
#include "gatherer.hpp"
#include "qstr.hpp"
#include "scatterer.hpp"
#include "term_scatterer.hpp"
#include "term_gatherer.hpp"

using namespace caf;

sink::sink(environment* env, QWidget* parent, QString name)
    : entity(env, parent, name) {
  // nop
}

sink::~sink() {
  // nop
}

void sink::start() {
  CAF_LOG_TRACE("");
  dialog_->drop_stage_widgets();
  dialog_->drop_source_widgets();
  simulant_->become(
    [=](const stream<int>& in) {
      CAF_LOG_TRACE(CAF_ARG(in));
      if (smp != nullptr) {
        CAF_LOG_DEBUG("smp != nullptr");
        auto& sm = simulant_->current_mailbox_element()
                     ->content()
                     .get_as<stream_msg>(0);
        auto& op = get<stream_msg::open>(sm.content);
        smp->add_source(in.id(), op.prev_stage, op.original_stage, op.priority,
                        op.redeployable, response_promise{});
        simulant_->streams().emplace(in.id(), smp);
        return;
      }
      CAF_LOG_DEBUG("smp == nullptr");
      smp = simulant_->make_sink(
        in,
        [](unit_t&) {
          // nop
        },
        [=](unit_t&, int) {
          CAF_LOG_TRACE("");
          if (!started_) {
            CAF_LOG_DEBUG("first-time run, set started_ = true");
            started_ = true;
          }
          if (text(dialog_->sink_current_sender).isEmpty()) {
            auto me = simulant_->current_mailbox_element();
            text(dialog_->sink_current_sender, env_->id_by_handle(me->sender));
            auto& sm = me->content().get_as<stream_msg>(0);
            auto& op = get<stream_msg::batch>(sm.content);
            max(dialog_->sink_batch_progress, static_cast<int>(op.xs_size));
            last_batch_start_ = env_->timestamp();
            CAF_LOG_DEBUG("initialized batch processing, yield");
            yield();
          }
          CAF_LOG_DEBUG("enter progress loop, yield ticks_per_item times");
          progress(dialog_->sink_item_progress, 0,
                   val(dialog_->sink_ticks_per_item));
          inc(dialog_->sink_batch_progress);
          if (at_max(dialog_->sink_batch_progress)) {
            CAF_LOG_DEBUG("got last item in batch, record at gatherer");
            auto& sg = static_cast<term_gatherer&>(smp->in());
            sg.batch_completed(val(dialog_->sink_batch_progress), 0,
                               last_batch_start_, env_->timestamp());
            yield();
            text(dialog_->sink_current_sender, qstr(""));
            reset(dialog_->sink_batch_progress, 0, 1);
          }
        },
        [](unit_t&) {
          // nop
        },
        policy::arg<term_gatherer, terminal_stream_scatterer>::value
      ).ptr();
      static_cast<term_gatherer&>(smp->in()).set_cycle_timeout();
      simulant_->become(
        [=](tick_atom, int64_t id) {
          CAF_LOG_TRACE(CAF_ARG(id));
          auto& sg = static_cast<term_gatherer&>(smp->in());
          if (sg.cycle_timeout == id) {
            sg.generate_tokens(env_->timestamp());
          }
        }
      );
    }
  );
  // Run initialization code of the simulant and update state model.
  simulant_->activate(env_->sys().dummy_execution_unit());
  simulant_->model()->update();
}
