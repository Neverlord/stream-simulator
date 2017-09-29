#ifndef ENTITY_DETAILS_HPP
#define ENTITY_DETAILS_HPP

#include <QDialog>

#include "caf/message.hpp"
#include "caf/actor_control_block.hpp"

#include "fwd.hpp"

#include "ui_entity_details.h"

class entity_details : public QDialog, public Ui::entity_details {
public:
  explicit entity_details(entity* ptr);

  ~entity_details();

  void drop_sink_widgets();

  void drop_stage_widgets();

  void drop_source_widgets();

  void drop_source_only_widgets();

public slots:
  /// Handles messages received by entites.
  void entity_received_message(int id, caf::strong_actor_ptr from,
                               caf::message content);

  /// Handles messages consumed by entites.
  void entity_consumed_message(int id);

private:
  void drop_by_prefix(const QString& prefix);

  environment* env_;

  Q_OBJECT
};

#endif // ENTITY_DETAILS_HPP
