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
#include "environment.hpp"

MainWindow::MainWindow(environment* env, QWidget *parent) :
    QMainWindow(parent),
    entity(env, parent, "MainWindow"),
    timer(new QTimer(this)) {
  // UI and signal/slot setup
  setupUi(this);
  connect(ticksPerSecond, SIGNAL(valueChanged(int)),
          this, SLOT(manual_tick_count_changed(int)));
  connect(manualTick, SIGNAL(pressed()),
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
  interval->setValue(interval->value() + 1);
  ticks->setValue(ticks->value() + 1);
}

void MainWindow::after_tick() {
  // nop
}

void MainWindow::tock() {
  interval->setValue(0);
  interval->setMaximum(ticksPerInterval->value());
}

void MainWindow::manual_tick_count_changed(int x) {
  if (x == 0) {
    manualTick->setEnabled(true);
    timer->stop();
  } else {
    manualTick->setEnabled(false);
    timer->start(1000 / x);
  }
}

namespace {

std::pair<QGroupBox*, QGridLayout*>
get_group_box(QWidget* parent, QGridLayout* parent_layout, QString name) {
  if (parent_layout == nullptr) {
    auto box = new QGroupBox(parent);
    box->setObjectName(name);
    box->setTitle(name);
    auto layout = new QGridLayout(box);
    layout->setObjectName(name + "Layout");
    layout->setSpacing(6);
    layout->setContentsMargins(11, 11, 11, 11);
    return {box, layout};
  }
  return {dynamic_cast<QGroupBox*>(parent->parentWidget()), parent_layout};
}

void new_label(QGridLayout* layout, QString name, QString text,
               int row, int column) {
  auto label = new QLabel(text, layout->parentWidget());
  label->setObjectName(name);
  layout->addWidget(label, row, column);
}

void new_text_display(QGridLayout* layout, QString parent_name, QString name,
                      QString text, int row, int start_column) {
  auto full_name = parent_name + name;
  new_label(layout, full_name + "Label", text, row, start_column);
  new_label(layout, full_name, "", row, start_column + 1);
}

void new_spin_box(QGridLayout* layout, QString parent_name, QString name,
                  QString text, int min_val, int max_val, int val,
                  bool interactive, int row, int start_column) {
  auto full_name = parent_name + name;
  new_label(layout, full_name + "Label", text, row, start_column);
  auto box = new QSpinBox(layout->parentWidget());
  box->setObjectName(full_name);
  box->setMinimum(min_val);
  box->setMaximum(max_val);
  box->setValue(val);
  if (!interactive) {
    box->setEnabled(false);
    box->setFrame(false);
    box->setReadOnly(true);
    box->setButtonSymbols(QAbstractSpinBox::NoButtons);
    box->setKeyboardTracking(false);
    box->setStyleSheet("background-color: transparent");
  }
  layout->addWidget(box, row, start_column + 1);
}

void new_progress_bar(QGridLayout* layout, QString parent_name, QString name,
                  QString text, int row, int start_column) {
  auto full_name = parent_name + name;
  new_label(layout, full_name + "Label", text, row, start_column);
  auto ptr = new QProgressBar(layout->parentWidget());
  ptr->setObjectName(parent_name + name);
  ptr->setMaximum(1);
  ptr->setValue(0);
  ptr->setTextVisible(false);
  layout->addWidget(ptr, row, start_column + 1);
}

void new_mailbox(QGridLayout* layout, QString parent_name,
                 int row, int start_column) {
  auto box = new QGroupBox(layout->parentWidget());
  auto mailbox_layout = new QGridLayout(box);
  mailbox_layout->setSpacing(5);
  mailbox_layout->setContentsMargins(5, 5, 5, 5);
  auto mailbox = new QListWidget(box);
  mailbox->setWordWrap(true);
  mailbox->setTextElideMode(Qt::ElideNone);
  mailbox->setAlternatingRowColors(true);
  mailbox->setObjectName(parent_name + "Mailbox");
  mailbox->setEnabled(false);
  mailbox->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  mailbox->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  mailbox->setAutoScroll(false);
  mailbox_layout->addWidget(mailbox, 0, 0, 1, 1);
  layout->addWidget(box, row, start_column, 1, 2);
}

void new_state_tree(QGridLayout* layout, QString parent_name,
                   int row, int start_column) {
  auto view = new QTreeView(layout->parentWidget());
  view->setObjectName(parent_name + "StateTree");
  layout->addWidget(view, row, start_column, 1, 2);
}

void new_hline(QGridLayout* layout, int width, QFrame::Shadow shadow,
               int row, int column, int column_span = 1) {
  auto hline = new QFrame(layout->parentWidget());
  hline->setMinimumSize(QSize(50, 0));
  hline->setLineWidth(width);
  hline->setFrameShadow(shadow);
  hline->setFrameShape(QFrame::HLine);
  layout->addWidget(hline, row, column, 1, column_span);
}

} // namespace <anonymous>

QGroupBox* MainWindow::make_source(QString name, QGridLayout* parent,
                                   int start_row, int start_column,
                                   bool add_mailbox) {
  // Create a group box as container for all further widgets.
  QGroupBox* box;
  QGridLayout* layout;
  std::tie(box, layout) = get_group_box(mainFrame, parent, name);
  // Factory functions for adding box children.
  using namespace std::placeholders;
  auto add_spin_box = std::bind(new_spin_box, layout, name,
                                _1, _2, _3, _4, _5, _6, _7, _8);
  auto add_progress_bar = std::bind(new_progress_bar, layout, name,
                                    _1, _2, _3, _4);
  add_spin_box("Weight", "Weight", 1, 9999, 1, true,
               start_row, start_column);
  add_spin_box("Rate", "Rate", 1, 9999, 1, true,
               start_row + 1, start_column);
  add_spin_box("Credit", "Credit", 0, 9999, 0, false,
               start_row + 2, start_column);
  add_progress_bar("ItemGeneration", "Item Generation",
                   start_row + 3, start_column);
  add_progress_bar("SendProgress", "Send Progress",
                   start_row + 4, start_column);
  add_progress_bar("SendTimeout", "Send Timeout",
                   start_row + 5, start_column);
  if (add_mailbox) {
    new_hline(layout, 1, QFrame::Sunken, start_row + 6, start_column, 2);
    new_state_tree(layout, name, start_row + 7, start_column);
    new_mailbox(layout, name, start_row + 8, start_column);
  }
  return box;
}

QGroupBox* MainWindow::make_sink(QString name, QGridLayout* parent,
                                 int start_row, int start_column) {
  QGroupBox* box;
  QGridLayout* layout;
  std::tie(box, layout) = get_group_box(mainFrame, parent, name);
  // Factory functions for adding box children.
  using namespace std::placeholders;
  auto add_spin_box = std::bind(new_spin_box, layout, name,
                                _1, _2, _3, _4, _5, _6, _7, _8);
  auto add_progress_bar = std::bind(new_progress_bar, layout, name,
                                    _1, _2, _3, _4);
  auto add_hline = std::bind(new_hline, layout, 1, QFrame::Sunken, _1, _2, _3);
  add_spin_box("CreditPerInterval", "Credit per Interval", 1, 9999, 100, true,
               start_row, start_column);
  add_spin_box("TicksPerItem", "Ticks per Item", 1, 9999, 1, true,
               start_row + 1, start_column);
  add_progress_bar("ItemProgress", "Item Progress",
                   start_row + 2, start_column);
  add_progress_bar("BatchProgress", "Batch Progress",
                   start_row + 3, start_column);
  // Add a frame for holding statistics.
  add_hline(start_row + 4, start_column, 2);
  auto stats_frame = new QFrame(box);
  stats_frame->setObjectName(name + "Stats");
  stats_frame->setFrameShape(QFrame::NoFrame);
  auto stats_layout = new QGridLayout(stats_frame);
  stats_layout->setObjectName(name + "StatsLayout");
  stats_layout->setSpacing(0);
  stats_layout->setContentsMargins(0, 0, 0, 0);
  auto add_stats_spin_box = std::bind(new_spin_box, stats_layout, name,
                                      _1, _2, 0, 9999, 0, false, _3, _4);
  auto add_stats_text_display = std::bind(new_text_display, stats_layout, name,
                                          _1, _2, _3, _4);
  add_stats_spin_box("BatchSize", "BatchSize", 0, 0);
  add_stats_text_display("CurrentSender", "Current Sender", 0, 2);
  add_stats_spin_box("Unassigned", "Unassigned", 1, 0);
  add_stats_spin_box("UnassignedAVG", "Unassigned AVG", 1, 2);
  add_stats_spin_box("Pending", "Pending", 2, 0);
  add_stats_spin_box("PendingAVG", "Pending AVG", 2, 2);
  add_stats_spin_box("Latency", "Latency", 3, 0);
  add_stats_spin_box("LatencyAVG", "Latency AVG", 3, 2);
  layout->addWidget(stats_frame, start_row + 5, start_column, 1, 2);
  // Add the mailbox.
  add_hline(start_row + 6, start_column, 2);
  new_state_tree(layout, name, start_row + 7, start_column);
  new_mailbox(layout, name, start_row + 8, start_column);
  return box;
}

QGroupBox* MainWindow::make_stage(QString name, QGridLayout* parent,
                                  int start_row, int start_column) {
  QGroupBox* box;
  QGridLayout* layout;
  std::tie(box, layout) = get_group_box(mainFrame, parent, name);
  // Add In/Out Ratio display.
  layout->addWidget(new QLabel("In/Out Ratio", box), 0, 0);
  auto ratio_in = new QSpinBox(box);
  ratio_in->setObjectName(name + "RatioIn");
  ratio_in->setMinimumSize(QSize(60, 0));
  ratio_in->setMinimum(1);
  ratio_in->setMaximum(9999);
  ratio_in->setValue(1);
  auto ratio_separator = new QLabel("/", box);
  ratio_separator->setAlignment(Qt::AlignCenter);
  auto ratio_out = new QSpinBox(box);
  ratio_out->setObjectName(name + "RatioOut");
  ratio_out->setMinimumSize(QSize(60, 0));
  ratio_out->setMinimum(1);
  ratio_out->setMaximum(9999);
  ratio_out->setValue(1);
  auto ratio_layout = new QHBoxLayout();
  ratio_layout->setSpacing(0);
  ratio_layout->addWidget(ratio_in);
  ratio_layout->addWidget(ratio_separator);
  ratio_layout->addWidget(ratio_out);
  layout->addLayout(ratio_layout, 0, 1);
  // Add output buffer size.
  new_spin_box(layout, name, "OutputBuffer", "Output Buffer",
               0, 9999, 0, false, 1, 0);
  new_hline(layout, 1, QFrame::Sunken, 2, 1, 2);
  // Add source and sink widgets.
  make_source(name, layout, start_row + 3, start_column, false);
  make_sink(name, layout, start_row + 9, start_column);
  return box;
}

QFrame* MainWindow::make_connection(QString from, QString to) {
  // Create a frame as container for all further widgets.
  auto prefix = from + to;
  auto frame = new QFrame(mainFrame);
  frame->setObjectName(prefix + "Connection");
  frame->setFrameShape(QFrame::NoFrame);
  auto layout = new QGridLayout(frame);
  // Factory functions.
  using namespace std::placeholders;
  auto add_spin_box = std::bind(new_spin_box, layout, prefix,
                                _1, _2, 0, 9999, 0, false, _3, _4);
  auto add_hline = std::bind(new_hline, layout, 1, QFrame::Plain, _1, _2, _3);
  layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum,
                                  QSizePolicy::Expanding),
                  0, 0);
  add_spin_box("Rate", "Rate", 1, 0);
  add_spin_box("Weight", "Weight", 2, 0);
  add_spin_box("Credit", "Credit", 3, 0);
  add_spin_box("PrevCredit", "Prev Credit", 4, 0);
  add_spin_box("Cap", "Cap", 5, 0);
  add_hline(6, 0, 2);
  layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum,
                                  QSizePolicy::Expanding),
                  7, 0);
  // Done.
  return frame;
}

void MainWindow::load_layout(QTextStream& in) {
  // Clean slate.
  setUpdatesEnabled(false);
  auto mf = mainFrame;
  qDeleteAll(mf->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
  qDeleteAll(mf->findChildren<QGridLayout*>("", Qt::FindDirectChildrenOnly));
  // Helper functions for error handling.
  auto warn = [&](QString text) {
    qDeleteAll(mf->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
    qDeleteAll(mf->findChildren<QGridLayout*>("", Qt::FindDirectChildrenOnly));
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
  QVector<QVector<QWidget*>> matrix;
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
  matrix.resize(height);
  for (auto& matrix_row : matrix)
    matrix_row.resize(width);
  // Keep track of which sources depend on which sinks.
  std::map<QString, QString> dependencies;
  // Singleton-like access (with lazy init) to loaded entites.
  std::map<QString, QWidget*> widgets;
  // Stores the last source (or stage) seen on a given row.
  QVector<QWidget*> row_backtrace;
  row_backtrace.resize(height);
  // Parse the input matrix to create all widgets and extract dependencies.
  for (auto row = 0; row < height; ++row) {
    for (auto col = 0; col < width; ++col) {
      auto& bt = row_backtrace[row];
      auto& cell_text = input_matrix[row][col];
      if (cell_text.size() < 4 && cell_text != "-") {
        warn_at_cell("Invalid text \"" + cell_text + "\"", row, col);
        return;
      } else if (cell_text.startsWith("src")) {
        if (bt != nullptr) {
          warn_at_cell("Misplaced source", row, col);
          return;
        }
        auto i = widgets.find(cell_text);
        QWidget* ptr = nullptr;
        if (i == widgets.end()) {
          ptr = make_source(cell_text);
          widgets.emplace(cell_text, ptr);
        } else {
          ptr = i->second;
        }
        assert(ptr->objectName() == cell_text);
        bt = ptr;
        matrix[row][col] = ptr;
      } else if (cell_text.startsWith("stg")) {
        if (bt == nullptr || bt->objectName().startsWith("snk")) {
          warn_at_cell("Misplaced stage", row, col);
          return;
        };
        QWidget* ptr = nullptr;
        auto i = widgets.find(cell_text);
        if (i == widgets.end()) {
          ptr = make_stage(cell_text);
          widgets.emplace(cell_text, ptr);
        } else {
          ptr = i->second;
        }
        assert(ptr->objectName() == cell_text);
        dependencies.emplace(bt->objectName(), cell_text);
        bt = ptr;
        matrix[row][col] = ptr;
      } else if (cell_text.startsWith("snk")) {
        if (bt == nullptr || bt->objectName().startsWith("snk")) {
          warn_at_cell("Misplaced sink", row, col);
          return;
        };
        QWidget* ptr = nullptr;
        auto i = widgets.find(cell_text);
        if (i == widgets.end()) {
          ptr = make_sink(cell_text);
          widgets.emplace(cell_text, ptr);
        } else {
          ptr = i->second;
        }
        assert(ptr->objectName() == cell_text);
        dependencies.emplace(bt->objectName(), cell_text);
        bt = ptr;
        matrix[row][col] = ptr;
      } else if (cell_text != "-") {
        warn_at_cell("Invalid text \"" + cell_text + "\"", row, col);
        return;
      }
    }
  }
  std::map<QString, entity*> receivers;
  // Create all entities, traversing columns in reverse order to create
  // dependencies first.
  for (auto row = 0; row < height; ++row) {
    for (auto col = width - 1; col >= 0; --col) {
      auto& cell_text = input_matrix[row][col];
      if (cell_text == "-") {
        // nop
      } else if (cell_text.startsWith("snk")) {
        if (receivers.count(cell_text) == 0) {
          auto ptr = env_->make_entity<sink>(this, cell_text);
          receivers.emplace(cell_text, ptr);
        }
      } else if (cell_text.startsWith("stg")) {
        auto i = receivers.find(dependencies[cell_text]);
        if (i == receivers.end()) {
          warn_at_cell(cell_text + " has no downstream", row, col);
          return;
        }
        if (receivers.count(cell_text) == 0) {
          auto ptr = env_->make_entity<stage>(this, cell_text,
                                              i->second->handle());
          receivers.emplace(cell_text, ptr);
        }
      } else {
        assert(cell_text.startsWith("src"));
        auto i = receivers.find(dependencies[cell_text]);
        if (i == receivers.end()) {
          warn_at_cell(cell_text + " has no downstream", row, col);
          return;
        }
        env_->make_entity<source>(this, cell_text, i->second->handle());
      }
    }
  }
  // Finally, layout all widgets. No need for any checks at this point.
  auto layout = new QGridLayout(mainFrame);
  auto layout_col = [](int col) -> int {
    // Computes the column in the final layout from the original position.
    return col * 2;
  };
  for (auto row = 0; row < height; ++row) {
    QString dep_name;
    auto dep_pos = -1; // Stores at which position our upstream is.
    for (auto col = 0; col < width; ++col) {
      auto ptr = matrix[row][col];
      if (ptr == nullptr) {
        // nop
      } else if (ptr->objectName().startsWith("src")) {
        dep_name = ptr->objectName();
        dep_pos = col;
        layout->addWidget(ptr, row, layout_col(col));
      } else {
        assert(dep_pos >= 0);
        assert(col > 0);
        auto lc = layout_col(col);
        auto dp = layout_col(dep_pos);
        // Add connection if it doesn't already exist.
        auto conn_name = matrix[row][dep_pos]->objectName();
        conn_name += matrix[row][col]->objectName();
        conn_name += "Connection";
        if (findChild<QObject*>(conn_name) == nullptr) {
          layout->addWidget(make_connection(dep_name, ptr->objectName()),
                            row, dp + 1, 1, lc - (dp + 1));
        };
        dep_name = ptr->objectName();
        dep_pos = col;
        if (row > 0 && matrix[row - 1][col] == ptr) {
          // The widget is already added to a previous row and spans down.
          continue;
        }
        // Check at which row this widget ends.
        auto end_row = row + 1;
        while (end_row < height && matrix[end_row][col] == ptr) {
          ++end_row;
        }
        layout->addWidget(ptr, row, lc, end_row - row, 1);
      }
    }
  }
  // Done.
  setUpdatesEnabled(true);
}

void MainWindow::load_default_view() {
  //QString txt = QStringLiteral("src1,src2,-;stg1,stg1,src3;snk1,snk1,snk1");
  QString txt = QStringLiteral("src1,stg1,snk1;src2,stg1,snk1;src3,-,snk1");
  QTextStream in(&txt);
  load_layout(in);
  /*
  // Clean up any current widgets and state.
  entities.clear();
  setUpdatesEnabled(false);
  auto ptr = mainFrame;
  qDeleteAll(ptr->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
  qDeleteAll(ptr->findChildren<QGridLayout*>("", Qt::FindDirectChildrenOnly));
  auto layout = new QGridLayout(ptr);
  layout->addWidget(make_source("src1"), 0, 0, 1, 1);
  layout->addWidget(make_source("src2"), 1, 0, 1, 1);
  layout->addWidget(make_source("src3"), 2, 0, 1, 1);
  layout->addWidget(make_connection("src1", "stg1"), 0, 1, 1, 1);
  layout->addWidget(make_connection("src2", "stg1"), 1, 1, 1, 1);
  layout->addWidget(make_connection("src3", "snk1"), 2, 1, 1, 3);
  layout->addWidget(make_stage("stg1"), 0, 2, 2, 1);
  layout->addWidget(make_connection("stg1", "snk1"), 0, 3, 1, 1);
  layout->addWidget(make_sink("snk1"), 0, 4, 3, 1);
  // Create state for the simulation itself.
  auto snk1 = new sink(this, "snk1");
  entities.emplace_back(snk1);
  auto stg1 = new stage(snk1, this, "stg1");
  entities.emplace_back(stg1);
  entities.emplace_back(new source(stg1, this, "src1"));
  entities.emplace_back(new source(stg1, this, "src2"));
  entities.emplace_back(new source(snk1, this, "src3"));
  // Initial tock to start things off.
  for (auto& x : entities)
    x->tock();
  setUpdatesEnabled(true);
  */
}
