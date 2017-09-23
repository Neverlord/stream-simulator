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

#ifndef SINK_HPP
#define SINK_HPP

#include <cstdint>

#include <QSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>

#include "entity.hpp"

class sink : virtual public entity {
public:
  sink(environment* env, QWidget* parent, QString name);

  ~sink() override;

  void tick() override;

  void start() override;

protected:
  /// Returns `true` if an item was consumed. Additionally sets `state_` to
  /// `idle` when the batch was completed.
  bool consume_batch_impl();

  QSpinBox* credit_per_interval_;
  QSpinBox* ticks_per_item_;
  QSpinBox* batch_size_;
  QProgressBar* item_progress_;
  QProgressBar* batch_progress_;
  QListWidget* mailbox_;
  QLabel* current_sender_;
  QSpinBox* pending_;
  QSpinBox* pending_avg_;
  QSpinBox* latency_;
  QSpinBox* latency_avg_;
};

#endif // SINK_HPP
