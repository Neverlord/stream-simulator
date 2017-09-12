#ifndef GATHERER_HPP
#define GATHERER_HPP

#include "caf/stream_gatherer.hpp"

#include <QObject>

#include "fwd.hpp"

class gatherer : public caf::stream_gatherer {
public:
  gatherer(entity* self);

  ~gatherer() override;

  path_ptr add_path(const caf::stream_id& sid, caf::strong_actor_ptr x,
                    caf::strong_actor_ptr original_stage,
                    caf::stream_priority prio, long available_credit,
                    bool redeployable, caf::response_promise result_cb) override;

  bool remove_path(const caf::stream_id& sid, const caf::actor_addr& x,
                   caf::error reason, bool silent) override;

  void close(caf::message result) override;

  void abort(caf::error reason) override;

  long num_paths() const override;

  bool closed() const override;

  bool continuous() const override;

  void continuous(bool value) override;

  path_ptr find(const caf::stream_id& sid, const caf::actor_addr& x) override;

  path_ptr path_at(size_t index) override;

  long high_watermark() const override;

  long min_credit_assignment() const override;

  long max_credit() const override;

  void high_watermark(long x) override;

  void min_credit_assignment(long x) override;

  void max_credit(long x) override;

  void assign_credit(long downstream_capacity) override;

  long initial_credit(long downstream_capacity, path_ptr x) override;

private:
  QObject* parent_;
};

#endif // GATHERER_HPP
