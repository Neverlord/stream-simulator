#ifndef GATHERER_HPP
#define GATHERER_HPP

#include "caf/random_gatherer.hpp"

#include "fwd.hpp"
#include "tick_time.hpp"
#include "rate_controlled_sink.hpp"

class gatherer : public caf::random_gatherer {
public:
  using super = caf::random_gatherer;

  template <class Scatterer>
  gatherer(caf::local_actor* self, Scatterer& out)
      : super(self, out),
        parent_(static_cast<simulant*>(self)->parent()) {
    // nop
  }

  ~gatherer() override;

  void assign_credit(long downstream_capacity) override;

  long initial_credit(long downstream_capacity, path_ptr x) override;

  void batch_completed(caf::inbound_path* from, size_t xs_size, int64_t id);

private:
  entity* parent_;

  double proportional_ = 1;
  double integral_ = .2;
  double derivative_ = 0;
  double min_rate_ = 100;

  /// State for computing the rate using a PID controller.
  struct rate_state {
    tick_duration last_time_;
    tick_duration batch_interval_;
    double last_rate_;
    double last_err_;
  };

  std::unordered_map<caf::inbound_path*, rate_state> rate_states_;

  void update_rate(caf::inbound_path* from, tick_duration time,
                   size_t num_elements, tick_duration processing_delay,
                   tick_duration scheduling_delay);
};

#endif // GATHERER_HPP
