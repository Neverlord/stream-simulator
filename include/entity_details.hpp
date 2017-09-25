#ifndef ENTITY_DETAILS_HPP
#define ENTITY_DETAILS_HPP

#include <QDialog>

#include "ui_entity_details.h"

class entity_details : public QDialog, public Ui::entity_details {
  Q_OBJECT
public:
  explicit entity_details(QWidget *parent = 0);

  ~entity_details();

  void drop_sink_widgets();

  void drop_stage_widgets();

  void drop_source_widgets();

  void drop_source_only_widgets();

  using widget_pointer = QWidget*;

  inline void del() {
    // end of recursion
  }

  template <class T, class... Ts>
  void del(T& x, Ts&... xs) {
    delete x;
    x = nullptr;
    del(xs...);
  }
};

#endif // ENTITY_DETAILS_HPP
