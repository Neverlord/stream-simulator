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

  caf::invoke_message_result consume(caf::mailbox_element& x) override;

  template <class F>
  void iterate_mailbox(F f) {
    if (!mailbox().closed() && !mailbox().empty())
      mailbox().peek_all(f);
  }

  template <class... Ts>
  void become(Ts&&... xs) {
    do_become(caf::behavior{std::forward<Ts>(xs)...}, true);
  }

  inline simulant_tree_model* model() {
    return &model_;
  }

  void serialize_state(simulant_tree_item& root);

  void detach_from_parent();

private:
  // Adds `ptr` to `pending_messages_` and returns its ID.
  int push_pending_message(caf::mailbox_element* ptr);

  // Removes `ptr` from `pending_messages_` and returns its ID.
  int pop_pending_message(caf::mailbox_element* ptr);

  // Returns the ID of `ptr` or 0 if it isn't in `pending_messages_`.
  int peek_pending_message(caf::mailbox_element* ptr);

  template <class F>
  void for_all_model_items(F f) {
    model_.root()->for_all_children(f);
  }

  // Points to the simulator environment. The environment hosts the actor
  // system and is thus guaranteed to outlive the simulant.
  environment* env_;

  // Points to the entity that created this simulant. The parent can be
  // destroyed before the simulant. This pointer is nulled in the parent's
  // destructor via `detach_from_parent`.
  std::atomic<entity*> parent_;

  // Protects access to `parent_`.
  std::mutex parent_mtx_;

  // Allows Qt to render the state of this simulant in a tree.
  simulant_tree_model model_;

  // Consecutively numbers incoming messages with an actor-unique ID. This
  // allows the simulation to keep track of messages in the system.
  std::atomic<int> msg_ids_;

  // Associates pending messages with their assigned ID.
  std::map<caf::mailbox_element*, int> pending_messages_;

  // Protects access to `pending_messages_`.
  std::mutex pending_messages_mtx_;
};

void intrusive_ptr_add_ref(simulant*);

void intrusive_ptr_release(simulant*);

#endif // SIMULANT_HPP
