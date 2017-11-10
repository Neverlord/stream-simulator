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

#include "mainwindow.hpp"

#include <algorithm>
#include <functional>

#include <QGroupBox>
#include <QTreeView>
#include <QMessageBox>

#include "sink.hpp"
#include "source.hpp"
#include "stage.hpp"
#include "node.hpp"
#include "edge.hpp"
#include "environment.hpp"

MainWindow::MainWindow(environment* env, QWidget *parent) :
    QMainWindow(parent),
    env_(env),
    timer(new QTimer(this)) {
  // UI and signal/slot setup
  setupUi(this);
  connect(ticks_per_second, SIGNAL(valueChanged(int)),
          this, SLOT(manual_tick_count_changed(int)));
  connect(manual_tick, SIGNAL(pressed()),
          this, SIGNAL(manual_tick_triggered()));
  connect(timer, SIGNAL(timeout()),
          this, SIGNAL(tick_triggered()));
  load_default_view();
}

MainWindow::~MainWindow() {
  // nop
}

void MainWindow::start() {
  // nop
}

void MainWindow::before_tick() {
  // nop
}

void MainWindow::tick() {
  ticks->setValue(ticks->value() + 1);
}

void MainWindow::after_tick() {
  // nop
}

void MainWindow::tock() {
  // nop
}

void MainWindow::manual_tick_count_changed(int x) {
  if (x == 0) {
    manual_tick->setEnabled(true);
    timer->stop();
  } else {
    manual_tick->setEnabled(false);
    timer->start(1000 / x);
  }
}

void MainWindow::load_layout(QTextStream& in) {
  // Clean slate.
  setUpdatesEnabled(false);
  qDeleteAll(dag->items());
  // Helper functions for error handling.
  auto warn = [&](QString text) {
    setUpdatesEnabled(true);
    QMessageBox::warning(this, "Cannot load layout", text);
  };
  auto warn_at_cell = [&](QString text, int row, int col) {
    text += " in cell (";
    text += QString::number(row);
    text += ", ";
    text += QString::number(col);
    text += ")";
    warn(text);
  };
  auto line = in.readLine();
  // We expect a line like "src1,src2;snk1,snk1".
  // ',' separates columns and ';' separates rows.
  auto rows = line.split(";");
  if (rows.empty()) {
    warn("No scheme to load in first line");
    return;
  }
  QVector<QStringList> input_matrix;
  for (auto& row : rows)
    input_matrix.append(row.split(","));
  // Make sure all columns are of equal size.
  auto height = input_matrix.size();
  auto width = input_matrix[0].size();
  if (width < 2) {
      warn("Layout contains only a single column");
      return;
  }
  for (auto& row : input_matrix)
    if (row.size() != width) {
      warn("Columns are not of equal size");
      return;
    }
  std::vector<std::vector<entity*>> matrix;
  matrix.resize(height);
  for (auto& matrix_row : matrix)
    matrix_row.resize(width);
  // Maps source IDs to sink IDs.
  std::map<source*, entity*> edges;
  // Singleton-like access (with lazy init) to loaded entites.
  // Stores the last source (or stage) seen on a given row.
  std::vector<entity*> row_backtrace;
  row_backtrace.resize(height);
  // Parse the input matrix to create all entities and to extract dependencies.
  for (auto row = 0; row < height; ++row) {
    // Keep track of last seen source.
    source* bt = nullptr;
    for (auto col = 0; col < width; ++col) {
      auto& cell_text = input_matrix[row][col];
      if (cell_text.size() < 4 && cell_text != "-") {
        warn_at_cell("Invalid text \"" + cell_text + "\"", row, col);
        return;
      } else if (cell_text.startsWith("src")) {
        if (bt != nullptr) {
          warn_at_cell("Misplaced source", row, col);
          return;
        }
        bt = env_->get_entity<source>(this, cell_text);
        matrix[row][col] = bt;
      } else if (cell_text.startsWith("stg")) {
        if (bt == nullptr) {
          warn_at_cell("Misplaced stage", row, col);
          return;
        };
        auto ptr = env_->get_entity<stage>(this, cell_text);
        edges.emplace(bt, ptr);
        bt = ptr;
        matrix[row][col] = ptr;
      } else if (cell_text.startsWith("snk")) {
        if (bt == nullptr) {
          warn_at_cell("Misplaced sink", row, col);
          return;
        };
        auto ptr = env_->get_entity<sink>(this, cell_text);
        edges.emplace(bt, ptr);
        bt = nullptr;
        matrix[row][col] = ptr;
      } else if (cell_text != "-") {
        warn_at_cell("Invalid text \"" + cell_text + "\"", row, col);
        return;
      }
    }
  }
  std::map<entity*, node*> nodes;
  auto scene = dag->scene();
  // Create graphics view nodes.
  for (auto& x : env_->entities()) {
    auto ptr = new node(x.get(), dag);
    nodes.emplace(x.get(), ptr);
    scene->addItem(ptr); // Scene takes ownership of ptr.
  }
  // Create graphics view edges and connect sources to sinks.
  for (auto& kvp : edges) {
    kvp.first->add_consumer(kvp.second->handle());
    scene->addItem(new edge(nodes[kvp.first], nodes[kvp.second]));
  }
  dag->shuffle();
  // Done.
  setUpdatesEnabled(true);
}

void MainWindow::load_default_view() {
  //QString txt = QStringLiteral("src1,stg1,snk1;src2,stg1,snk1;src3,-,snk1");
  QString txt = QStringLiteral("src1,snk1");
  QTextStream in(&txt);
  load_layout(in);
}
