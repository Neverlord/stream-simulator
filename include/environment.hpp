#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <map>
#include <memory>
#include <vector>
#include <cassert>
#include <random>
#include <unordered_map>

#include <QApplication>

#include "caf/fwd.hpp"
#include "caf/message.hpp"
#include "caf/actor_control_block.hpp"

#include "entity.hpp"
#include "mainwindow.hpp"
#include "tick_time.hpp"

/// Top-level Qt object for the simulation. Contains the CAF actor system.
class environment : public QObject {
public:
  // -- Nested types -----------------------------------------------------------

  struct enqueued_message {
    int id;
    tick_time timestamp;
    caf::strong_actor_ptr from;
    caf::message content;
    enqueued_message(int id, tick_time timestamp, caf::strong_actor_ptr from,
                     caf::message content);
  };

  struct in_flight_message {
    caf::strong_actor_ptr receiver;
    caf::mailbox_element_ptr content;
  };

  using scheduler_type = caf::scheduler::test_coordinator;

  using entity_ptr = std::unique_ptr<entity>;

  using entity_ptrs = std::vector<entity_ptr>;

  using tick_durations = std::vector<tick_duration>;

  using duration_samples = std::unordered_map<entity*, tick_durations>;

  using enqueued_messages = std::vector<enqueued_message>;

  using timestamped_messages = std::map<entity*, enqueued_messages>;

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
    connect_slots(ptr, std::is_same<T, sink>::value);
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
      ptr = make_entity<T>(parent, id, std::forward<Ts>(xs)...);
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

  // -- statistics of simulation metrics ---------------------------------------

  /// Returns the average latency for `x`.
  tick_duration average_latency(entity* x);

  /// Returns the average latency for all entites.
  tick_duration average_global_latency();

  /// Returns what percentage of time `x` is idle.
  double idle_percentage(entity* x);

  /// Returns what percentage of time sinks are idle.
  double average_global_idle_percentage();

  /// Transmits a new message over the "network".
  void transmit(caf::strong_actor_ptr receiver,
                caf::mailbox_element_ptr content);

public slots:
  /// Triggers a single computation step.
  void tick();

  /// Triggers `manual_tick_count` computation steps.
  void manual_tick();

  /// Handles idle entites.
  void sink_idling();

  /// Handles messages received by entites.
  void entity_received_message(int id, caf::strong_actor_ptr from,
                               caf::message content);

  /// Handles messages consumed by entites.
  void entity_consumed_message(int id);

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

  void connect_slots(MainWindow* x);

  void connect_slots(entity* x, bool is_sink);

  caf::actor_system& sys_;
  scheduler_type* sched_;
  entity_ptrs entities_;
  std::unique_ptr<MainWindow> main_window_;
  bool running_;

  /// Keeps track of the current tick count,  i.e., the current timestamp.
  tick_time time_;

  /// Stores unprocessed messages with delivery timestamp.
  timestamped_messages enqueued_messages_;

  /// Keeps track of reported latency times.
  duration_samples latency_samples_;

  /// Keeps track of reported idle times.
  std::unordered_map<entity*, tick_duration> idle_times_;

  /// Simulates a "network" by delaying messages.
  std::multimap<tick_time, in_flight_message> network_queue_;

  /// Protects access to `delivery_queue_`.
  std::mutex network_queue_mtx_;

  /// Generates a random seed.
  std::random_device rng_device_;

  /// Pseudo-random number generator.
  std::mt19937 rng_;

  Q_OBJECT
};

#endif // ENVIRONMENT_HPP
