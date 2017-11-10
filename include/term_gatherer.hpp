#ifndef TERM_GATHERER_HPP
#define TERM_GATHERER_HPP

#include "caf/fwd.hpp"
#include "caf/random_gatherer.hpp"

#include "fwd.hpp"
#include "tick_time.hpp"
#include "rate_controlled_source.hpp"

class term_gatherer : public caf::random_gatherer,
                      public rate_controlled_source {
public:
  using super = caf::random_gatherer;

  template <class Scatterer>
  term_gatherer(caf::local_actor* self, Scatterer& out)
      : super(self, out),
        parent_(static_cast<simulant*>(self)->parent()) {
    // nop
  }

  term_gatherer() = delete;
  term_gatherer(const term_gatherer&) = delete;
  term_gatherer& operator=(const term_gatherer&) = delete;

  ~term_gatherer() override;

  void assign_credit(long downstream_capacity) override;

  long initial_credit(long downstream_capacity, path_ptr x) override;

  void batch_completed(long xs_size, tick_time enqueue_time,
                       tick_time start_time, tick_time end_time) override;

  long generate_tokens(tick_time now) override;

  entity* parent_;

  // -- static configuration

  double proportional_ = 1;
  double integral_ = .2;
  double derivative_ = 0;
  tick_duration cycle_duration = 100;

  /// Minimum number of tokens per cycle.
  double min_tokens_ = 100;

  /// The desired processing time for a single batch in ticks.
  tick_duration desired_batch_complexity_ = 20;

  /// Minimum number of items per batch even if this puts the complexity above
  /// `desired_batch_complexity_`.
  long min_batch_size = 5;

  // -- dynamic configuration

  long batch_size_hint = 50;

  // -- historic data

  /// Timestamp of the last call to `generate_tokens`.
  tick_time last_cycle_ = 0;

  /// Generated tokens during the last cycle.
  double last_token_count_ = min_tokens_;

  /// Average time per processed item.
  double historic_time_per_item_ = 0;

  // -- sampling 

  /// Time spent processing batches during the last cycle.
  tick_duration processing_time_ = 0;

  /// Number of processed items in the last cycle.
  long processed_items_ = 0;

  // -- timeout management

  int64_t cycle_timeout = 0;

  void set_cycle_timeout();
};

#endif // TERM_GATHERER_HPP
