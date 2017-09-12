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

#include <cassert>

#include <QObject>
#include <QString>
#include <QProgressBar>

#include "caf/intrusive_ptr.hpp"

#include "fwd.hpp"
#include "simulant.hpp"

/// An `entity` in a simulation.
class entity {
public:
  enum state_t {
    idle,
    read_mailbox,
    consume_batch,
    produce_batch
  };

  entity(environment* env, QObject* parent, QString name);

  virtual ~entity();

  /// Prepares the entity for the first tick.
  virtual void start() = 0;

  /// Decides what computations are triggered by the next tick.
  virtual void before_tick();

  /// Advances time by one step.
  virtual void tick() = 0;

  /// Cleans up any open state from the current tick.
  virtual void after_tick();

  /// Advances time by one interval (after X ticks have been emitted).
  virtual void tock();

  /// Returns a unique identifier for this entity.
  inline const QString& id() const {
    return name_;
  }

  inline QObject* parent() const {
    return parent_;
  }

  inline environment* env() const {
    return env_;
  }

  int tick_time() const;

  void render_mailbox();

  caf::actor handle();

  /// Returns `true` if at least one message is waiting in the mailbox.
  bool mailbox_ready();

  /// Lets the simulant read and process the next message in the mailbox.
  void resume_simulant();

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

  template <class T>
  void init(T& ptr, QString child_name) {
    ptr = parent_->findChild<T>(child_name);
    assert(ptr != nullptr);
  }

  environment* env_;
  QObject* parent_;
  QString name_;
  caf::intrusive_ptr<simulant> simulant_;

  /// Can be set by `before_tick` to cause `tick` to resume the simulant.
  state_t state_;

  /// Allows the entity to detect state transitions.
  state_t last_state_;
};

const char* to_string(entity::state_t x);

#endif // ENTITY_HPP
