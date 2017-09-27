#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <memory>
#include <vector>
#include <cassert>
#include <unordered_map>

#include <QApplication>

#include "caf/fwd.hpp"

#include "entity.hpp"
#include "mainwindow.hpp"
#include "tick_time.hpp"

/// Top-level Qt object for the simulation. Contains the CAF actor system.
class environment : public QObject {
public:
  // -- Nested types -----------------------------------------------------------

  using scheduler_type = caf::scheduler::test_coordinator;

  using entity_ptr = std::unique_ptr<entity>;

  using entity_ptrs = std::vector<entity_ptr>;

  using tick_durations = std::vector<tick_duration>;

  using duration_samples = std::unordered_map<entity*, tick_durations>;

  using entity_message_pair = std::pair<entity*, caf::mailbox_element*>;

  using timestamped_messages = std::map<entity_message_pair, tick_time>;

  // -- Construction, destruction, and assignment ------------------------------

  static std::unique_ptr<environment> make(caf::actor_system& sys);

  environment(const environment&) = delete;

  environment& operator=(const environment&) = delete;

  // -- Simulation control functions -------------------------------------------

  /// Runs the simulation.
  void run();

  // -- Setup functions --------------------------------------------------------

  /// Adds a new entity to the simulation.
  /// @warning Illegal to call while running the simulation.
  template <class T, class... Ts>
  T* make_entity(Ts&&... xs) {
    assert(!running_);
    auto ptr = new T(this, std::forward<Ts>(xs)...);
    entities_.emplace_back(ptr);
    return ptr;
  }

  /// Returns an existing entity or creates a new entity if necessary.
  template <class T, class... Ts>
  T* get_entity(QWidget* parent, const QString& id, Ts&&... xs) {
    assert(!running_);
    T* ptr = nullptr;
    auto base_ptr = entity_by_id(id);
    if (base_ptr == nullptr) {
      ptr = new T(this, parent, id, std::forward<Ts>(xs)...);
      entities_.emplace_back(ptr);
    } else {
      ptr = dynamic_cast<T*>(base_ptr);
      if (!ptr)
        throw std::logic_error("Cannot create entity twice "
                               "with different type.");
    }
    return ptr;
  }

  // -- accessors and mutators -------------------------------------------------

  /// Returns the list of simulated entities.
  inline const entity_ptrs& entities() const {
    return entities_;
  }

  /// Returns the host actor system.
  inline caf::actor_system& sys() {
    return sys_;
  }

  /// Returns the entity associated with `x`.
  entity* entity_by_id(const QString& x) const;

  /// Returns the entity associated with `x`.
  entity* entity_by_handle(const caf::actor_addr& x) const;

  /// Returns the ID of the entity associated with `x`.
  QString id_by_handle(const caf::actor_addr& x) const;

  /// Returns the ID of the entity associated with `x`.
  inline QString id_by_handle(const caf::strong_actor_ptr& x) const {
    return id_by_handle(caf::actor_cast<caf::actor_addr>(x));
  }

  /// Returns whether `id` refers to a known entity.
  inline bool has_entity(const QString& id) const {
    return entity_by_id(id) != nullptr;
  }

  /// Returns the current simulation time.
  inline tick_time timestamp() const {
    return time_;
  }

  /// Registers `(receiver, msg)` as in-flight message with the current time as
  /// delivery timestamp.
  inline void register_in_flight_message(entity* receiver,
                                         caf::mailbox_element* msg) {
    in_flight_messages_.emplace(std::make_pair(receiver, msg), time_);
  }

  /// Deregisters `(receiver, msg)` from in-flight messages and returns the
  /// stored delivery timestamp. The difference between the delivery timestamp
  /// and the current timestamp is stored automatically in `latency_samples`.
  /// @throws std::runtime_error if `(receiver, msg)` is an invalid pair.
  tick_time deregister_in_flight_message(entity* receiver,
                                         caf::mailbox_element* msg);

  /// Increases the recorded idle time for `x` if `x->started() == true`.
  void inc_idle_time(entity* x);

  // -- statistics of simulation metrics ---------------------------------------

  /// Returns the average latency for `x`.
  tick_duration average_latency(entity* x);

  /// Returns the average latency for all entites.
  tick_duration average_global_latency();

  /// Returns what percentage of time `x` is idle.
  double idle_percentage(entity* x);

  /// Returns what percentage of time sinks are idle.
  double average_global_idle_percentage();

public slots:
  /// Triggers a single computation step.
  void tick();

  /// Triggers `manual_tick_count` computation steps.
  void manual_tick();

signals:
  /// Emitted when a new latency sample is added and the average is recomputed.
  void average_global_latency_changed(int value);

  /// Emitted when a new latency sample is added and the average is recomputed.
  void average_latency_changed(entity* receiver, int value);

  /// Emitted when tick time changes and the percentage of reported idle times
  /// is recomputed.
  void average_global_idle_percentage_changed(double percentage);

  /// Emitted when tick time changes and the percentage of reported idle times
  /// is recomputed.
  void idle_percentage_changed(entity* receiver, double percentage);

private:
  environment(caf::actor_system &sys, scheduler_type* sched);

  double idle_percentage(tick_duration x);

  void tick(bool silent);

  caf::actor_system& sys_;
  scheduler_type* sched_;
  entity_ptrs entities_;
  std::unique_ptr<MainWindow> main_window_;
  bool running_;

  /// Keeps track of the current tick count,  i.e., the current timestamp.
  tick_time time_;

  /// Stores in-flight messages with delivery timestamp.
  timestamped_messages in_flight_messages_;

  /// Keeps track of reported latency times.
  duration_samples latency_samples_;

  /// Keeps track of reported idle times.
  std::unordered_map<entity*, tick_duration> idle_times_;

  Q_OBJECT
};

#endif // ENVIRONMENT_HPP
