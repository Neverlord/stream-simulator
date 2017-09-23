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

#ifndef STAGE_HPP
#define STAGE_HPP

#include "sink.hpp"
#include "source.hpp"

class stage : public source, public sink {
  public:
  stage(environment* env, QWidget* parent, QString name);

  ~stage();

  void before_tick() override;

  void tick() override;

  void after_tick() override;

  void tock() override;

  void start() override;

private:
  QSpinBox* ratio_in_;
  QSpinBox* ratio_out_;
  QSpinBox* output_buffer_;
  int completed_items_;
  std::unique_ptr<caf::downstream<int>> out_;
};

#endif // STAGE_HPP
