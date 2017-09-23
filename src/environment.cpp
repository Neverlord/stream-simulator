#include "environment.hpp"

#include <string>

#include "caf/actor_system.hpp"
#include "caf/actor_system_config.hpp"
#include "caf/message.hpp"

#include "caf/scheduler/test_coordinator.hpp"

#include "mainwindow.hpp"

std::unique_ptr<environment> environment::make(caf::actor_system& sys) {
  std::unique_ptr<environment> result;
  auto sched = &dynamic_cast<scheduler_type&>(sys.scheduler());
  result.reset(new environment(sys, sched));
  return result;
}

void environment::run() {
  // Get CLI arguments for Qt.
  int argc = 0;
  std::vector<char*> args;
  { // lifetime scope of ar
    auto& ar = sys_.config().args_remainder;
    argc = static_cast<int>(ar.size());
    for (size_t i = 0; i < ar.size(); ++i)
      args.emplace_back(const_cast<char*>(ar.get_as<std::string>(i).c_str()));
  }
  // Allocate Qt resources.
  QApplication app{argc, args.data()};
  main_window_ = std::make_unique<MainWindow>(this);
  // Connect main window events to environment slots.
  connect(main_window_.get(), SIGNAL(tick_triggered()),
          this, SLOT(tick()));
  connect(main_window_.get(), SIGNAL(manual_tick_triggered()),
          this, SLOT(manual_tick()));
  // Initialize main window and start all entities.
  main_window_->show();
  main_window_->start();
  for (auto& e : entities_)
    e->start();
  // Enter Qt's event loop.
  running_ = true;
  app.setQuitOnLastWindowClosed(true);
  app.exec();
  running_ = false;
  // Clean up all state except the CAF system.
  main_window_.reset();
  entities_.clear();
  disconnect();
}

entity* environment::entity_by_id(const QString& x) const {
  auto pred = [&](const entity_ptr& y) {
    assert(y != nullptr);
    return x == y->id();
  };
  auto e = entities_.end();
  auto i = std::find_if(entities_.begin(), e, pred);
  if (i != e)
    return i->get();
  return nullptr;
}

entity* environment::entity_by_handle(const caf::actor_addr& x) const {
  auto pred = [&](const entity_ptr& y) {
    assert(y != nullptr);
    return x == y->handle();
  };
  auto e = entities_.end();
  auto i = std::find_if(entities_.begin(), e, pred);
  if (i != e)
    return i->get();
  return nullptr;
}

QString environment::id_by_handle(const caf::actor_addr& x) const {
  auto ptr = entity_by_handle(x);
  return ptr == nullptr ? QString{} : ptr->id();
}

void environment::tick() {
  main_window_->before_tick();
  for (auto& entity : entities_)
    entity->before_tick();
  main_window_->tick();
  for (auto& entity : entities_)
    entity->tick();
  main_window_->after_tick();
  for (auto& entity : entities_)
    entity->after_tick();
}

void environment::manual_tick() {
  for (auto i = 0; i < main_window_->manualTickCount->value(); ++i)
    tick();
}

environment::environment(caf::actor_system& sys, scheduler_type* sched)
  : sys_(sys),
    sched_(sched),
    main_window_(nullptr),
    running_(false) {
  // nop
}
