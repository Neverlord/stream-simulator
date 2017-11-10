#ifndef SCATTERER_HPP
#define SCATTERER_HPP

#include "caf/broadcast_scatterer.hpp"

#include "entity.hpp"
#include "tick_time.hpp"

template <class T>
class scatterer : public caf::broadcast_scatterer<T> {
public:
  using super = caf::broadcast_scatterer<T>;

  using path_ptr = typename super::path_ptr;

  scatterer(caf::local_actor* self)
      : super(self),
        parent_(static_cast<simulant*>(self)->parent()) {
    // nop
  }

  void batch_sent(tick_time tstamp, size_t num_elements) {
    CAF_ASSERT(samples_.empty() || samples_.back().first <= tstamp);
    if (samples_.size() > 1000)
      samples_.erase(samples_.begin());
    samples_.emplace_back(tstamp, num_elements);
  }

  double last_rate(tick_time tstamp) {
    // Select samples for the last second.
    auto t0 = std::min(tstamp - 1000000, 0);
    struct {
      bool operator()(tick_time x, const sample& y) const {
        return x < y.first;
      }
      bool operator()(const sample& x, tick_time y) const {
        return x.first < y;
      }
    } pred;
    auto i = std::lower_bound(samples_.begin(), samples_.end(), t0, pred);
    if (i == samples_.end())
      return 0.;
    auto e = std::upper_bound(samples_.begin(), samples_.end(), tstamp, pred);
    auto add = [](size_t x, const sample& y) { return x + y.second; };
    return std::accumulate(i, e, 0ul, add);
  }

private:
  using sample = std::pair<tick_time, size_t>;

  std::vector<sample> samples_;

  struct rate_state {
  };

  std::unordered_map<caf::inbound_path*, rate_state> rate_states_;

  entity* parent_;
};

#endif // SCATTERER_HPP
