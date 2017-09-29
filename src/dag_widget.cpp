#include "dag_widget.hpp"

#include <cmath>

#include <QKeyEvent>
#include <QTreeView>

#include "edge.hpp"
#include "entity.hpp"
#include "node.hpp"
#include "qstr.hpp"

dag_widget::dag_widget(QWidget* parent)
    : QGraphicsView(parent),
      timerId(0),
      selected_(nullptr) {
  auto scene = new QGraphicsScene(this);
  scene->setItemIndexMethod(QGraphicsScene::NoIndex);
  scene->setSceneRect(-200, -200, 400, 400);
  setScene(scene);
  setCacheMode(CacheBackground);
  setViewportUpdateMode(BoundingRectViewportUpdate);
  setRenderHint(QPainter::Antialiasing);
  setTransformationAnchor(AnchorUnderMouse);
  scale(qreal(0.8), qreal(0.8));
  setMinimumSize(400, 400);
  setWindowTitle(tr("Elastic nodes"));
}

void dag_widget::itemMoved() {
  if (!timerId)
    timerId = startTimer(1000 / 25);
}

void dag_widget::selected(node* x) {
  if (x == selected_)
    return;
  auto old = selected_;
  selected_ = x;
  if (old != nullptr)
    old->update();
  auto e = x->entity();
  auto tv = e->parent()->findChild<QTreeView*>(qstr("state"));
  tv->setModel(e->model());
  tv->expandAll();
}

void dag_widget::resizeEvent(QResizeEvent* event) {
  centerize_dag();
  super::resizeEvent(event);
}

void dag_widget::keyPressEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_Up:
      centernode->moveBy(0, -20);
      break;
    case Qt::Key_Down:
      centernode->moveBy(0, 20);
      break;
    case Qt::Key_Left:
      centernode->moveBy(-20, 0);
      break;
    case Qt::Key_Right:
      centernode->moveBy(20, 0);
      break;
    case Qt::Key_Space:
    case Qt::Key_Enter:
      shuffle();
      break;
    default:
      QGraphicsView::keyPressEvent(event);
  }
}

void dag_widget::timerEvent(QTimerEvent*) {
  std::vector<node*> nodes;
  for (auto item : scene()->items()) {
    auto ptr = qgraphicsitem_cast<node*>(item);
    if (ptr)
      nodes.emplace_back(ptr);
  }
  for (auto x : nodes)
    x->calculateForces();
  bool items_moved = false;
  for (auto x : nodes)
    if (x->advance())
      items_moved = true;
  if (items_moved) {
    centerize_dag();
  } else {
    killTimer(timerId);
    timerId = 0;
  }
}

void dag_widget::scaleView(qreal scaleFactor) {
  qreal factor = transform()
                   .scale(scaleFactor, scaleFactor)
                   .mapRect(QRectF(0, 0, 1, 1))
                   .width();
  if (factor < 0.07 || factor > 100)
    return;
  scale(scaleFactor, scaleFactor);
}

void dag_widget::shuffle() {
  // Collect nodes.
  std::vector<node*> nodes;
  for (auto item : scene()->items()) {
    auto ptr = qgraphicsitem_cast<node*>(item);
    if (ptr)
      nodes.emplace_back(ptr);
  }
  // Shuffle position.
  for (auto x : nodes)
    x->setPos(-150 + qrand() % 300, -150 + qrand() % 300);
  // Advance nodes until the scene stabilizes.
  bool items_moved = true;
  while (items_moved) {
    items_moved = false;
    for (auto x : nodes)
      x->calculateForces();
    for (auto x : nodes)
      if (x->advance())
        items_moved = true;
  }
  // Centerize scene.
  centerize_dag();
}

void dag_widget::zoomIn() {
  scaleView(qreal(1.2));
}

void dag_widget::zoomOut() {
  scaleView(1 / qreal(1.2));
}

void dag_widget::centerize_dag() {
  auto br = scene()->itemsBoundingRect();
  setSceneRect(br);
  fitInView(br.x() - 4, br.y() - 4, br.width() + 8, br.height() + 8,
            Qt::KeepAspectRatio);
}
