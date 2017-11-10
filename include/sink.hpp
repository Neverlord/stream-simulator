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

#include "caf/stream_manager.hpp"

#include "entity.hpp"
#include "tick_time.hpp"

class sink : virtual public entity {
public:
  sink(environment* env, QWidget* parent, QString name);

  ~sink() override;

  void start() override;

private:
  tick_time last_batch_start_;
  caf::stream_manager_ptr smp;
};

#endif // SINK_HPP
