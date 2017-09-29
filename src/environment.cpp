#include "environment.hpp"

#include <string>

#include <QDebug>

#include "caf/actor_system.hpp"
#include "caf/actor_system_config.hpp"
#include "caf/message.hpp"

#include "caf/scheduler/test_coordinator.hpp"

#include "sink.hpp"
#include "mainwindow.hpp"

environment::enqueued_message::enqueued_message(
  int id_arg, tick_time timestamp_arg, caf::strong_actor_ptr from_arg,
  caf::message content_arg)
    : id(id_arg),
      timestamp(timestamp_arg),
      from(std::move(from_arg)),
      content(std::move(content_arg)) {
  // nop
}

std::unique_ptr<environment> environment::make(caf::actor_system& sys) {
  std::unique_ptr<environment> result;
  auto sched = &dynamic_cast<scheduler_type&>(sys.scheduler());
  result.reset(new environment(sys, sched));
  return result;
}

void environment::run() {
  // Reset any state.
  time_ = 0;
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
  // Initialize main window and start all entities.
  connect_slots(main_window_.get());
  main_window_->show();
  main_window_->start();
  for (auto& e : entities_)
      e->start();
  for (auto& e : entities_)
      e->run_posted_events();
  // We start at tick count 1. Some entities send messages during
  // `start()`, i.e., "tick 0".
  time_ = 1;
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

void environment::sink_idling() {
  printf("sink_idling\n"); fflush(stdout);
  auto x = qobject_cast<entity*>(sender());
  if (x != nullptr && x->started())
    ++idle_times_[x];
}

void environment::entity_received_message(int id, caf::strong_actor_ptr from,
                                          caf::message content) {
  auto x = qobject_cast<entity*>(sender());
  if (x == nullptr)
    return;
  printf("%s received msg #%d (t %d): %s\n", x->id().toUtf8().constData(), id, (int) time_, to_string(content).c_str()); fflush(stdout);
  enqueued_messages_[x].emplace_back(id, timestamp(), std::move(from),
                                      std::move(content));
}

void environment::entity_consumed_message(int id) {
  auto pred = [id](const enqueued_message& y) {
    return y.id == id;
  };
  auto x = qobject_cast<entity*>(sender());
  if (x == nullptr)
    return;
  printf("%s consumed msg #%d (t %d)\n", x->id().toUtf8().constData(), id, (int) time_); fflush(stdout);
  auto& ifms = enqueued_messages_[x];
  auto e = ifms.end();
  auto i = std::find_if(ifms.begin(), e, pred);
  if (i == e) {
    qDebug() << "received entity_consumed_message twice or missing "
                "matching entity_received_message signal";
    return;
  }
  auto t_0 = i->timestamp;
  auto t_now = timestamp();
  assert(t_0 < t_now);
  ifms.erase(i);
  latency_samples_[x].emplace_back(t_now - t_0);
  //emit average_global_latency_changed(average_global_latency());
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

void environment::transmit(caf::strong_actor_ptr receiver,
                           caf::mailbox_element_ptr content) {
  auto t = timestamp();
  auto r_0 = main_window_->min_delay->value();
  auto r_n = std::max(main_window_->max_delay->value(), r_0);
  if (r_0 == r_n) {
    t += r_0;
  } else {
    std::uniform_int_distribution<int> f(r_0, r_n);
    t += f(rng_);
  }
  critical_section(network_queue_mtx_, [&] {
    network_queue_.emplace(t, in_flight_message{std::move(receiver),
                                                std::move(content)});
  });
}

environment::environment(caf::actor_system& sys, scheduler_type* sched)
  : sys_(sys),
    sched_(sched),
    main_window_(nullptr),
    running_(false),
    time_(0),
    rng_(rng_device_()) {
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
      emit idle_percentage_changed(kvp.first, idle_percentage(kvp.second));
    emit average_global_idle_percentage_changed(average_global_idle_percentage());
    emit average_global_latency_changed(average_global_latency());
  }
}

void environment::connect_slots(MainWindow* x) {
  // Connect main window events to environment slots.
  connect(x, SIGNAL(tick_triggered()), SLOT(tick()));
  connect(x, SIGNAL(manual_tick_triggered()), SLOT(manual_tick()));
  connect(this, SIGNAL(average_global_latency_changed(int)),
          x->avg_latency, SLOT(setValue(int)));
  connect(this, SIGNAL(average_global_idle_percentage_changed(double)),
          x->avg_sink_idle_time, SLOT(setValue(double)));
  connect(this, SIGNAL(average_global_latency_changed(int)),
          x->avg_latency, SLOT(setValue(int)));
}

void environment::connect_slots(entity* x, bool is_sink) {
  if (is_sink)
    connect(x, SIGNAL(idling()), SLOT(sink_idling()));
  connect(
    x, SIGNAL(message_received(int, caf::strong_actor_ptr, caf::message)),
    SLOT(entity_received_message(int, caf::strong_actor_ptr, caf::message)));
  connect(x, SIGNAL(message_consumed(int)), SLOT(entity_consumed_message(int)));
}
