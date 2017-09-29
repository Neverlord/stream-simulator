#ifndef GRAPH_WIDGET_HPP
#define GRAPH_WIDGET_HPP

#include <QGraphicsView>

#include "fwd.hpp"

/// Models a widget for displaying directed acyclic graphs (DAGs).
class dag_widget : public QGraphicsView {
  Q_OBJECT

public:
  using super = QGraphicsView;

  dag_widget(QWidget* parent = 0);

  void itemMoved();

  inline node* selected() {
    return selected_;
  }

  void selected(node* x);

public slots:
  void shuffle();
  void zoomIn();
  void zoomOut();

protected:
  void resizeEvent(QResizeEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void timerEvent(QTimerEvent* event) override;

  void scaleView(qreal scaleFactor);

private:
  void centerize_dag();

  int timerId;
  node* centernode;
  node* selected_;
};

#endif // GRAPH_WIDGET_HPP
