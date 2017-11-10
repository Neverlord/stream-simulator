#include "gatherer.hpp"

#include "caf/all.hpp"

#include "entity.hpp"
#include "environment.hpp"
#include "scatterer.hpp"

using namespace caf;

gatherer::~gatherer() {
  // nop
}

void gatherer::assign_credit(long ) {
  // TODO: implement me
}

long gatherer::initial_credit(long downstream_capacity, path_ptr x) {
  // TODO: implement me
  CAF_IGNORE_UNUSED(downstream_capacity);
  CAF_IGNORE_UNUSED(x);
  return 0;
}

void gatherer::batch_completed(inbound_path* from, size_t num_elements, int64_t) {
  auto t = parent_->env()->timestamp();
  auto wait_delay = 5; // waitDelay <- batchCompleted.batchInfo.schedulingDelay
  auto work_delay = 5; // processingEndTime - processingStartTime;
  update_rate(from, t, num_elements, work_delay, wait_delay);
}

void gatherer::update_rate(inbound_path* from, tick_duration time,
                           size_t num_elements, tick_duration processing_delay,
                           tick_duration scheduling_delay) {
  // In elements/tick.
  auto processing_rate = static_cast<double>(num_elements) / processing_delay;
  auto i = rate_states_.find(from);
  if (i == rate_states_.end()) {
    rate_states_.emplace(
      from,
      rate_state{time, static_cast<tick_duration>(processing_rate), 0., 0});
    return;
  }
  auto& rs = i->second;
  if (time > rs.last_time_ && num_elements > 0 && processing_delay > 0) {
    // in seconds, should be close to batchDuration
    auto delay_since_update = time - rs.last_time_;


    // In our system `error` is the difference between the desired rate and the
    // measured rate based on the last batch information. We consider the
    // desired rate to be last rate, which is what this estimator calculated
    // for the previous batch. In elements/tick.
    auto err = rs.last_rate_ - processing_rate;
    // The error integral, based on schedulingDelay as an indicator for
    // accumulated errors. A scheduling delay s corresponds to s *
    // processingRate overflowing elements. Those are elements that couldn't
    // be processed in previous batches, leading to this delay. In the
    // following, we assume the processingRate didn't change too much. From
    // the number of overflowing elements we can calculate the rate at which
    // they would be processed by dividing it by the batch interval. This
    // rate is our "historical" error, or integral part, since if we
    // subtracted this rate from the previous "calculated rate", there
    // wouldn't have been any overflowing elements, and the scheduling delay
    // would have been zero. In elements/tick.
    auto historical_err = static_cast<double>(scheduling_delay)
                          * processing_rate / rs.batch_interval_;
    // In elements/(tick ^ 2).
    auto d_err = to_seconds((err - rs.last_err_) / delay_since_update);
    auto new_rate =
      std::max(rs.last_rate_ - proportional_ * err - integral_ * historical_err
                 - derivative_ * d_err,
               min_rate_);
    rs.last_time_ = time;
    rs.last_rate_ = new_rate;
    rs.last_err_ = err;
  }
}

