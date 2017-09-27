#ifndef SIMULANT_HPP
#define SIMULANT_HPP

#include "caf/scheduled_actor.hpp"

#include "fwd.hpp"
#include "simulant_tree_model.hpp"

class simulant : public caf::scheduled_actor {
public:
  using super = caf::scheduled_actor;

  using behavior_type = caf::behavior;

  simulant(caf::actor_config& cfg, entity* parent);

  using caf::scheduled_actor::enqueue;

  void enqueue(caf::mailbox_element_ptr ptr, caf::execution_unit*) override;

  resume_result resume(caf::execution_unit*, size_t) override;

  template <class F>
  void iterate_mailbox(F f) {
    if (!mailbox().closed() && !mailbox().empty())
      mailbox().peek_all(f);
  }

  template <class... Ts>
  void become(Ts&&... xs) {
    do_become(caf::behavior{std::forward<Ts>(xs)...}, true);
  }

  inline entity* parent() {
    return parent_;
  }

  inline simulant_tree_model* model() {
    return &model_;
  }

  void serialize_state(simulant_tree_item& root);

private:
  template <class F>
  void for_all_model_items(F f) {
    model_.root()->for_all_children(f);
  }

  entity* parent_;
  simulant_tree_model model_;
};

void intrusive_ptr_add_ref(simulant*);

void intrusive_ptr_release(simulant*);

#endif // SIMULANT_HPP
