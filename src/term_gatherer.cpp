#include "term_gatherer.hpp"

#include "caf/all.hpp"

#include "entity.hpp"
#include "environment.hpp"
#include "scatterer.hpp"

using namespace caf;

term_gatherer::~term_gatherer() {
  // nop
}

void term_gatherer::assign_credit(long available) {
  // TODO: use path weights
  printf("assign %ld credit\n", available);
  CAF_LOG_TRACE(CAF_ARG(available));
  if (assignment_vec_.empty())
    return;
  auto credit_per_path = available / static_cast<long>(assignment_vec_.size());
  for (auto& kvp : assignment_vec_)
    if (credit_per_path > kvp.first->assigned_credit)
      kvp.second = credit_per_path - kvp.first->assigned_credit;
    else
      kvp.second = 0;
  emit_credits();
}

long term_gatherer::initial_credit(long downstream_capacity, path_ptr x) {
  // TODO: implement me
  CAF_IGNORE_UNUSED(downstream_capacity);
  CAF_IGNORE_UNUSED(x);
  return min_tokens_;
}

void term_gatherer::batch_completed(long xs_size, tick_time,
                                    tick_time start_time, tick_time end_time) {
  printf("batch completed; xs_size : %ld, started: %d, ended: %d\n", xs_size,
         start_time, end_time);
  CAF_ASSERT(xs_size > 0);
  CAF_ASSERT(end_time >= start_time);
  processed_items_ += xs_size;
  processing_time_ += end_time - start_time;
  // Trigger cycle timeout if necessary.
  if (end_time >= last_cycle_ + cycle_duration) {
    generate_tokens(parent_->env()->timestamp());
  }
}

long term_gatherer::generate_tokens(tick_time now) {
  printf("generate tokens, tstamp: %d\n", now);
  printf(
    "last token count: %f, processed items: %ld, last_cycle: %d, now: %d\n",
    last_token_count_, processed_items_, last_cycle_, now);
  long result;
  // Stick to the last processed token count if no batch was processed during
  // the last cycle.
  if (processed_items_ == 0 || now <= last_cycle_ || last_cycle_ == 0) {
    result = static_cast<long>(last_token_count_);
  } else {
    auto time_per_item = processing_time_ / static_cast<double>(processed_items_);
    printf("time per item: %f\n", time_per_item);
    if (time_per_item == 0) {
      result = static_cast<long>(last_token_count_);
    } else {
      // Theoretical maximum when processing batches nonstop.
      auto upper_bound = cycle_duration / time_per_item;
      auto hint = std::max(min_batch_size,
                           lround(desired_batch_complexity_ / time_per_item));
      if (hint != batch_size_hint) {
        batch_size_hint = hint;
        for (auto& x : paths_)
          x->desired_batch_size = hint;
      }
      printf("upper bound: %f\n", upper_bound);
      result = static_cast<long>(std::max(upper_bound, min_tokens_));
    }
  }
  last_cycle_ = now;
  last_token_count_ = result;
  assign_credit(result);
  set_cycle_timeout();
  // reset measurement variables
  processing_time_ = 0;
  processed_items_ = 0;
  return result;
}

void term_gatherer::set_cycle_timeout() {
  auto ptr = make_mailbox_element(nullptr, message_id::make(), {},
                                  tick_atom::value, ++cycle_timeout);
  auto env = parent_->env();
  auto sim = parent_->sim();
  sim->push_pending_message(ptr.get());
  sim->update_model();
  env->post_f(cycle_duration, [=, me = std::move(ptr)](tick_time) mutable {
    sim->enqueue(std::move(me), nullptr);
  });
}
