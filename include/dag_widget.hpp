#ifndef GRAPH_WIDGET_HPP
#define GRAPH_WIDGET_HPP

#include <QGraphicsView>

#include "fwd.hpp"

/// Models a widget for displaying directed acyclic graphs (DAGs).
class dag_widget : public QGraphicsView {
  Q_OBJECT

public:
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
  void keyPressEvent(QKeyEvent* event) override;
  void timerEvent(QTimerEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void drawBackground(QPainter* painter, const QRectF& rect) override;

  void scaleView(qreal scaleFactor);

private:
  int timerId;
  node* centernode;
  node* selected_;
};

#endif // GRAPH_WIDGET_HPP
