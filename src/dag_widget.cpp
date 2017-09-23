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
  QGraphicsScene* scene = new QGraphicsScene(this);
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
    case Qt::Key_Plus:
      zoomIn();
      break;
    case Qt::Key_Minus:
      zoomOut();
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
  foreach (QGraphicsItem* item, scene()->items()) {
    auto ptr = qgraphicsitem_cast<node*>(item);
    if (ptr)
      nodes.emplace_back(ptr);
  }
  for (auto x : nodes)
    x->calculateForces();
  bool itemsMoved = false;
  for (auto x : nodes)
    if (x->advance())
      itemsMoved = true;
  if (!itemsMoved) {
    killTimer(timerId);
    timerId = 0;
  }
}

void dag_widget::wheelEvent(QWheelEvent* event) {
  scaleView(pow((double)2, -event->delta() / 240.0));
}

void dag_widget::drawBackground(QPainter*, const QRectF&) {
  /*
  // Shadow
  QRectF sceneRect = this->sceneRect();
  QRectF rightShadow(sceneRect.right(), sceneRect.top() + 5, 5,
                     sceneRect.height());
  QRectF bottomShadow(sceneRect.left() + 5, sceneRect.bottom(),
                      sceneRect.width(), 5);
  if (rightShadow.intersects(rect) || rightShadow.contains(rect))
    painter->fillRect(rightShadow, Qt::darkGray);
  if (bottomShadow.intersects(rect) || bottomShadow.contains(rect))
    painter->fillRect(bottomShadow, Qt::darkGray);
  // Fill
  QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
  gradient.setColorAt(0, Qt::white);
  gradient.setColorAt(1, Qt::lightGray);
  painter->fillRect(rect.intersected(sceneRect), gradient);
  painter->setBrush(Qt::NoBrush);
  painter->drawRect(sceneRect);
  // Text
  QRectF textRect(sceneRect.left() + 4, sceneRect.top() + 4,
                  sceneRect.width() - 4, sceneRect.height() - 4);
  QString message(
    tr("Click and drag the nodes around, and zoom with the mouse "
       "wheel or the '+' and '-' keys"));

  QFont font = painter->font();
  font.setBold(true);
  font.setPointSize(14);
  painter->setFont(font);
  painter->setPen(Qt::lightGray);
  painter->drawText(textRect.translated(2, 2), message);
  painter->setPen(Qt::black);
  painter->drawText(textRect, message);
  */
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
  foreach (QGraphicsItem* item, scene()->items()) {
    if (qgraphicsitem_cast<node*>(item))
      item->setPos(-150 + qrand() % 300, -150 + qrand() % 300);
  }
}

void dag_widget::zoomIn() {
  scaleView(qreal(1.2));
}

void dag_widget::zoomOut() {
  scaleView(1 / qreal(1.2));
}
