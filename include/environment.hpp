#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <memory>
#include <vector>
#include <cassert>

#include <QApplication>

#include "caf/fwd.hpp"

#include "entity.hpp"
#include "mainwindow.hpp"

/// Top-level Qt object for the simulation. Contains the CAF actor system.
class environment : public QObject {
public:
  using scheduler_type = caf::scheduler::test_coordinator;

  using entity_ptr = std::unique_ptr<entity>;

  using entities_vec = std::vector<entity_ptr>;

  static std::unique_ptr<environment> make(caf::actor_system& sys);

  /// Run the simulation.
  void run();

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

  inline const entities_vec& entities() const {
    return entities_;
  }

  caf::actor_system& sys() {
    return sys_;
  }

  entity* entity_by_id(const QString& x) const;

  entity* entity_by_handle(const caf::actor_addr& x) const;

  QString id_by_handle(const caf::actor_addr& x) const;

  inline QString id_by_handle(const caf::strong_actor_ptr& x) const {
    return id_by_handle(caf::actor_cast<caf::actor_addr>(x));
  }

  inline bool has_entity(const QString& id) const {
    return entity_by_id(id) != nullptr;
  }

public slots:
  /// Triggers a single computation step.
  void tick();

  /// Triggers `manual_tick_count` computation steps.
  void manual_tick();

private:
  environment(caf::actor_system &sys, scheduler_type* sched);

  caf::actor_system& sys_;
  scheduler_type* sched_;
  entities_vec entities_;
  std::unique_ptr<MainWindow> main_window_;
  bool running_;

  Q_OBJECT
};

#endif // ENVIRONMENT_HPP
