#ifndef TERM_SCATTERER_HPP
#define TERM_SCATTERER_HPP

#include "caf/terminal_stream_scatterer.hpp"

#include "entity.hpp"
#include "tick_time.hpp"
#include "rate_controlled_sink.hpp"

class term_scatterer : public caf::terminal_stream_scatterer,
                       public rate_controlled_sink {
public:
  using super = caf::terminal_stream_scatterer;

  using path_ptr = typename super::path_ptr;

  term_scatterer(caf::local_actor* self)
      : super(self),
        parent_(static_cast<simulant*>(self)->parent()) {
    // nop
  }

  void batch_sent(tick_time tstamp, size_t num_elements) override;

  double last_rate(tick_time tstamp) override;

private:
  using sample = std::pair<tick_time, size_t>;

  std::vector<sample> samples_;

  struct rate_state {
  };

  std::unordered_map<caf::inbound_path*, rate_state> rate_states_;

  entity* parent_;
};

#endif // TERM_SCATTERER_HPP
