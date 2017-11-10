#ifndef RATE_CONTROLLED_SOURCE_HPP
#define RATE_CONTROLLED_SOURCE_HPP

#include "entity.hpp"
#include "tick_time.hpp"

class rate_controlled_source {
public:
  virtual ~rate_controlled_source();

  /// Records completion of a batch and updates the rate calculation based on
  /// the observed computation or wait time.
  virtual void batch_completed(long xs_size, tick_time enqueue_time,
                               tick_time start_time, tick_time end_time) = 0;

  /// Generates tokens for the next interval.
  virtual long generate_tokens(tick_time now) = 0;
};

#endif // RATE_CONTROLLED_SOURCE_HPP
