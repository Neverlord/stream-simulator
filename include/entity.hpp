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

#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <vector>
#include <cassert>
#include <functional>

#include <QObject>
#include <QString>
#include <QProgressBar>

#include "caf/intrusive_ptr.hpp"

#include "fwd.hpp"
#include "simulant.hpp"
#include "critical_section.hpp"

/// An `entity` in a simulation.
class entity : public QObject {
public:
  friend class simulant;

  enum state_t {
    idle,
    read_mailbox,
    resume_simulant
  };

  entity(environment* env, QWidget* parent, QString name);

  virtual ~entity();

  /// Prepares the entity for the first tick.
  virtual void start() = 0;

  /// Decides what computations are triggered by the next tick.
  virtual void before_tick();

  /// Advances time by one step.
  virtual void tick();

  /// Cleans up any open state from the current tick.
  virtual void after_tick();

  /// Advances time by one interval (after X ticks have been emitted).
  virtual void tock();

  /// Returns a unique identifier for this entity.
  inline const QString& id() const {
    return name_;
  }

  inline QWidget* parent() const {
    return parent_;
  }

  inline environment* env() const {
    return env_;
  }

  simulant_tree_model* model();

  caf::actor handle();

  /// Returns `true` if at least one message is waiting in the mailbox.
  bool mailbox_ready();

  void show_dialog();

  inline void refresh_mailbox() {
    refresh_mailbox_ = true;
  }

  /// Returns whether this entity started its task. This indicates
  /// that batches are emitted for sources and received for sinks.
  inline bool started() const {
    return started_;
  }

  void run_posted_events();

signals:
  /// Signals that no operation was performed during a tick interval.
  void idling();

  /// Signals that a new message was received.
  /// @param id Ascending counter for unambiguous message identification.
  void message_received(int id, caf::strong_actor_ptr from,
                        caf::message content);

  /// Signals that a messages was consumed.
  /// @param id Ascending counter for unambiguous message identification.
  void message_consumed(int id);

protected:
  template <class T>
  static int val(const T* obj) {
    return obj->value();
  }

  template <class T>
  static void val(T* obj, int x) {
    obj->setValue(x);
  }

  template <class T>
  static int max(const T* obj) {
    return obj->maximum();
  }

  template <class T>
  static void max(T* obj, int x) {
    obj->setMaximum(x);
  }

  template <class T>
  static void inc_max(T* obj) {
    obj->setMaximum(obj->maximum() + 1);
  }
  template <class T>
  static inline bool at_max(const T* obj) {
    return val(obj) == max(obj);
  }

  template <class T>
  static void inc(T* obj, int value = 1) {
    obj->setValue(obj->value() + value);
  }

  template <class T>
  static void dec(T* obj, int value = 1) {
    obj->setValue(obj->value() - value);
  }

  template <class T>
  static int enabled(T* obj) {
    return obj->isEnabled();
  }

  template <class T>
  static void enabled(T* obj, bool x) {
    if (obj->isEnabled() != x)
      obj->setEnabled(x);
  }

  template <class T>
  static void enable(T* obj) {
    enabled(obj, true);
  }

  template <class T>
  static void disable(T* obj) {
    enabled(obj, false);
  }

  template <class T>
  static QString text(T* obj) {
    return obj->text();
  }

  template <class T>
  static void text(T* obj, QString x) {
    obj->setText(x);
  }

  template <class T>
  static void reset(T* obj, int value, int maximum) {
    max(obj, maximum);
    val(obj, value);
  }

  void progress(QProgressBar* bar, int first, int last);

  template <class F>
  void progress(QProgressBar* bar, int first, int last, F f) {
    if (first == last) {
      val(bar, 0);
      return;
    }
    max(bar, last);
    for (int i = first; i != last; ++i) {
      val(bar, i);
      f(i);
      yield();
    }
    val(bar, last);
  }

  template <class Int, class F>
  std::enable_if_t<std::is_integral<Int>::value>
  loop(Int first, Int last, F f) {
    for (Int i = first; i != last; ++i) {
      f(i);
      yield();
    }
  }

  template <class Iter, class F>
  std::enable_if_t<!std::is_integral<Iter>::value>
  loop(Iter first, Iter last, F f) {
    for (Iter i = first; i != last; ++i) {
      f(*i);
      yield();
    }
  }

  environment* env_;
  QWidget* parent_;
  entity_details* dialog_;
  QString name_;

  // Our actor under test.
  caf::intrusive_ptr<simulant> simulant_;

  std::mutex simulant_mx_;

  // Used to execute simulant_->resume in order to gain fine-grained control
  // over the execution of simulant_. This control is used to block-and-resume
  // the simulant's handling of stream messages.
  std::thread simulant_thread_;

  // Allows waiting for simulant_resume_.
  std::condition_variable simulant_resume_cv_;

  // Necessary guard for simulant_resume_cv_.
  std::unique_lock<std::mutex>* simulant_resume_guard_;

  enum simulant_thread_state {
    sts_none,     // Signals the entity that simulant_thread_ is invalid.
    sts_resume,   // Signals the simulant_thread_ to continue.
    sts_yield,    // Signals the entity to continue.
    sts_finalize, // Signals the entity to join and destroy simulant_thread_.
    sts_abort     // Signals the simulant_thread_ to throw.
  };

  // Signals the entity to resume the simulant next tick.
  std::atomic<simulant_thread_state> simulant_thread_state_;

  // Allows waiting for simulant_yield_.
  std::condition_variable simulant_yield_cv_;

  // Transfers control from the simulant thread back to the entity.
  void yield();

  // Transers control from the entity to the simulant thread.
  void resume();

  // Delegates processing of the next mailbox element via simulant_thread_.
  void start_handling_next_message();

  /// Controls what code is executed during a tick.
  state_t state_;

  /// Allows the entity to detect state transitions within one tick.
  state_t before_tick_state_;

  /// Informs the entity to redraw its mailbox.
  std::atomic<bool> refresh_mailbox_;

  /// Stores whether an entity started stream processing. A sink sets this flag
  /// to true when receiving the first batch.
  bool started_;

private:
  template <class F>
  void post(F f) {
    critical_section(simulant_events_mtx_, [&] {
      simulant_events_.emplace_back(std::move(f));
    });
  }

  /// Callbacks from the simulant's thread for running in the entity's thread
  /// after the simulant called `yield()`.
  std::vector<std::function<void(entity*)>> simulant_events_;

  std::mutex simulant_events_mtx_;

  Q_OBJECT
};

const char* to_string(entity::state_t x);

#endif // ENTITY_HPP
