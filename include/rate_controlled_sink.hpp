#ifndef RATE_CONTROLLED_SINK_HPP
#define RATE_CONTROLLED_SINK_HPP

#include "entity.hpp"
#include "tick_time.hpp"

class rate_controlled_sink {
public:
  virtual ~rate_controlled_sink();

  virtual void batch_sent(tick_time tstamp, size_t num_elements) = 0;

  virtual double last_rate(tick_time tstamp) = 0;
};

#endif // RATE_CONTROLLED_SINK_HPP
