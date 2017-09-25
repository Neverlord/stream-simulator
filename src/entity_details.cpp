#include "include/entity_details.hpp"
#include "ui_entity_details.h"

entity_details::entity_details(QWidget* parent) : QDialog(parent) {
  setupUi(this);
}

entity_details::~entity_details() {
  // nop
}

void entity_details::drop_sink_widgets() {
  del(sink_line_1, sink_line_2, ticks_per_item_label, ticks_per_item,
      batch_size_label, batch_size, current_sender_label, current_sender,
      input_batch_size_label, input_batch_size, item_progress_label,
      item_progress, batch_progress_label, batch_progress);
}

void entity_details::drop_stage_widgets() {
  del(stage_line_1, stage_line_2, ratio_label, ratio_frame, output_buffer_label,
      output_buffer);
}

void entity_details::drop_source_widgets() {
  del(source_line_1, source_line_2, weight_label, weight, rate_label, rate,
      item_generation_label, item_generation, batch_generation_label,
      batch_generation);
}

void entity_details::drop_source_only_widgets() {
  del(rate_label, rate, item_generation_label, item_generation);
}
