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
  connect(this, SIGNAL(average_global_latency_changed(int)),
          main_window_->avg_latency, SLOT(setValue(int)));
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
  tick(false);
}

void environment::manual_tick() {
  for (int i = 0; i < main_window_->manual_tick_count->value(); ++i)
    tick(true);
}

tick_time environment::deregister_in_flight_message(entity* receiver,
                                                    caf::mailbox_element* msg) {
  auto i = in_flight_messages_.find(std::make_pair(receiver, msg));
  if (i == in_flight_messages_.end())
    throw std::runtime_error("message not registered as in-flight");
  auto t_0 = i->second;
  auto t_now = timestamp();
  in_flight_messages_.erase(i);
  latency_samples_[receiver].emplace_back(t_now - t_0);

  emit average_global_latency_changed(average_global_latency());
  return t_0;
}

void environment::inc_idle_time(entity* x) {
  ++idle_times_[x];
}

tick_duration environment::average_latency(entity* x) {
  auto& xs = latency_samples_[x];
  if (xs.empty())
    return 0;
  return std::accumulate(xs.begin(), xs.end(), tick_duration{0})
         / static_cast<tick_duration>(xs.size());
}

tick_duration environment::average_global_latency() {
  tick_duration sum = 0;
  size_t count = 0;
  for (auto& kvp : latency_samples_) {
    auto& xs = kvp.second;
    sum += std::accumulate(xs.begin(), xs.end(), tick_duration{0});
    count += xs.size();
  }
  if (sum == 0)
    return 0;
  return sum / static_cast<tick_duration>(count);
}

double environment::idle_percentage(entity* x) {
  return idle_percentage(idle_times_[x]);
}

double environment::average_global_idle_percentage() {
  std::vector<double> xs;
  for (auto& kvp : idle_times_)
    xs.emplace_back(idle_percentage(kvp.second));
  if (xs.empty())
    return 0.;
  return std::accumulate(xs.begin(), xs.end(), 0.) / xs.size();
}

environment::environment(caf::actor_system& sys, scheduler_type* sched)
  : sys_(sys),
    sched_(sched),
    main_window_(nullptr),
    running_(false),
    time_(0) {
    // nop
}

double environment::idle_percentage(tick_duration x) {
  return x == 0 ? 0. : (static_cast<double>(time_) / x) * 100.;
}

void environment::tick(bool silent) {
  main_window_->before_tick();
  for (auto& entity : entities_)
    entity->before_tick();
  main_window_->tick();
  for (auto& entity : entities_)
    entity->tick();
  main_window_->after_tick();
  for (auto& entity : entities_)
    entity->after_tick();
  ++time_;
  if (!silent) {
    for (auto& kvp : idle_times_)
      idle_percentage_changed(kvp.first, idle_percentage(kvp.second));
    average_global_idle_percentage_changed(average_global_idle_percentage());
  }
}
