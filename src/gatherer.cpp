#include "gatherer.hpp"

#include "caf/all.hpp"

#include "entity.hpp"

using namespace caf;

gatherer::gatherer(entity* self) : parent_(self->parent()) {

}

gatherer::~gatherer() {

}

gatherer::path_ptr gatherer::add_path(const stream_id& sid, strong_actor_ptr x,
                                      strong_actor_ptr original_stage,
                                      stream_priority prio, long available_credit,
                                      bool redeployable, response_promise result_cb) {
  return nullptr;
}

bool gatherer::remove_path(const stream_id& sid, const actor_addr& x,
                           error reason, bool silent) {
  return false;
}

void gatherer::close(message result) {}

void gatherer::abort(error reason) {}

long gatherer::num_paths() const {}

bool gatherer::closed() const {}

bool gatherer::continuous() const {}

void gatherer::continuous(bool value) {}

gatherer::path_ptr gatherer::find(const stream_id& sid, const actor_addr& x) {}

gatherer::path_ptr gatherer::path_at(size_t index) {}

long gatherer::high_watermark() const {}

long gatherer::min_credit_assignment() const {}

long gatherer::max_credit() const {}

void gatherer::high_watermark(long x) {}

void gatherer::min_credit_assignment(long x) {}

void gatherer::max_credit(long x) {}

void gatherer::assign_credit(long downstream_capacity) {}

long gatherer::initial_credit(long downstream_capacity, path_ptr x) {}
