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
  source(environment* env, QWidget* parent, QString name);

  ~source() override;

  void start() override;

  void add_consumer(caf::actor consumer);

protected:
  // Pointer to the next stage in the pipeline.
  std::vector<caf::actor> consumers_;

  // Pointer to the CAF stream handler to advance the stream manually.
  caf::stream_manager_ptr stream_manager_;
};

#endif // SOURCE_HPP
