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

#ifndef SOURCE_HPP
#define SOURCE_HPP

#include <QSpinBox>
#include <QProgressBar>

#include "entity.hpp"
#include "mainwindow.hpp"

class source : virtual public entity {
public:
  source(environment* env, QObject* parent, QString name, caf::actor consumer);

  ~source() override;

  void tick() override;

  void tock() override;

  inline int rate() const {
    return rate_->value();
  }

  inline bool item_complete() const {
    return at_max(item_generation_);
  }

  inline bool send_complete() const {
    return at_max(send_progress_) || val(send_progress_) == val(credit_);
  }

  inline bool send_timeout() const{
    return at_max(send_timeout_);
  }

  void start() override;

  caf::stream_scatterer& out();

protected:
  void produce_batch_impl();

  // Pointer to the next stage in the pipeline.
  caf::actor consumer_;

  // Pointer to the CAF stream handler to advance the stream manually.
  caf::stream_manager_ptr stream_manager_;

  // Widgets belonging to this source.
  QSpinBox* weight_;
  QSpinBox* rate_;
  QSpinBox* credit_;
  QProgressBar* item_generation_;
  QProgressBar* send_progress_;
  QProgressBar* send_timeout_;

  // Widgets for global state.
  QSpinBox* max_batch_latency_;
};

#endif // SOURCE_HPP
