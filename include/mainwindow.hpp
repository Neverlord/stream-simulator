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

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <vector>
#include <memory>

#include <QTimer>
#include <QGroupBox>
#include <QMainWindow>
#include <QGridLayout>
#include <QTextStream>

#include "entity.hpp"

#include "ui_mainwindow.h"

class MainWindow : public QMainWindow, public entity, public Ui::MainWindow {
  Q_OBJECT

public:
  explicit MainWindow(environment* env, QWidget *parent = 0);

  ~MainWindow();

  void start() override;

  void before_tick() override;

  void tick() override;

  void after_tick() override;

  void tock() override;

signals:

  void tick_triggered();

  void manual_tick_triggered();

public slots:

  void manual_tick_count_changed(int);

public:
  QGroupBox* make_source(QString name, QGridLayout* parent = nullptr,
                         int start_row = 0, int start_column = 0,
                         bool add_mailbox = true);

  QGroupBox* make_sink(QString name, QGridLayout* parent = nullptr,
                       int start_row = 0, int start_column = 0);

  QGroupBox* make_stage(QString name, QGridLayout* parent = nullptr,
                        int start_row = 0, int start_column = 0);

  QFrame* make_connection(QString from, QString to);

private:
  void load_layout(QTextStream& in);
  void load_default_view();

  QTimer* timer;
};

#endif // MAINWINDOW_HPP
