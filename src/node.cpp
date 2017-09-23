#include "node.hpp"

#include <cassert>
#include <cmath>

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QTreeView>

#include "edge.hpp"
#include "qstr.hpp"
#include "entity.hpp"
#include "dag_widget.hpp"

node::node(class entity* parent_entity, dag_widget* parent_widget)
  : entity_(parent_entity),
    widget_(parent_widget),
    radius_(15.),
    shadow_offset_(2.) {
  setFlag(ItemIsMovable);
  setFlag(ItemSendsGeometryChanges);
  setCacheMode(DeviceCoordinateCache);
  setZValue(-1);
}

void node::add(edge* x) {
  assert(x != nullptr);
  assert(x->source() != nullptr);
  assert(x->dest() != nullptr);
  assert(x->source() == this || x->dest() == this);
  edges_.push_back(x);
  x->adjust();
}

void node::calculateForces() {
  if (!scene() || scene()->mouseGrabberItem() == this) {
    new_pos_ = pos();
    return;
  }
  // Sum up all forces pushing this item away.
  qreal xvel = 0;
  qreal yvel = 0;
  for (auto item : scene()->items()) {
    auto ptr = qgraphicsitem_cast<node*>(item);
    if (!ptr)
      continue;
    QPointF vec = mapToItem(ptr, 0, 0);
    qreal dx = vec.x();
    qreal dy = vec.y();
    double l = 2.0 * (dx * dx + dy * dy);
    if (l > 0) {
      xvel += (dx * 150.0) / l;
      yvel += (dy * 150.0) / l;
    }
  }
  // Subtract all forces pulling items together.
  double weight = (edges_.size() + 1) * 10.0;
  for (auto ptr : edges_) {
    auto vec = ptr->source() == this ? mapToItem(ptr->dest(), 0, 0)
                                     : mapToItem(ptr->source(), 0, 0);
    xvel -= vec.x() / weight;
    yvel -= vec.y() / weight;
  }
  // Ignore forces beyond 0.1 threshold.
  if (std::fabs(xvel) < 0.1 && std::fabs(yvel) < 0.1)
    xvel = yvel = 0;
  auto sceneRect = scene()->sceneRect();
  new_pos_ = pos() + QPointF(xvel, yvel);
  new_pos_.setX(
    qMin(qMax(new_pos_.x(), sceneRect.left() + 10), sceneRect.right() - 10));
  new_pos_.setY(
    qMin(qMax(new_pos_.y(), sceneRect.top() + 10), sceneRect.bottom() - 10));
}

int node::type() const {
  return Type;
}

bool node::advance() {
  if (new_pos_ == pos())
    return false;
  setPos(new_pos_);
  return true;
}

QRectF node::boundingRect() const {
  return {x(), y(), width() + shadow_offset_, height() + shadow_offset_};
}

QPainterPath node::shape() const {
  QPainterPath path;
  path.addEllipse(x(), y(), width(), height());
  return path;
}

void node::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                 QWidget*) {
  // Draw shadow.
  painter->setPen(Qt::NoPen);
  painter->setBrush(Qt::darkGray);
  painter->drawEllipse(x() + shadow_offset_, y() + shadow_offset_,
                       width(), height());
  // Calculate gradient coloring depending on whether the node is currently
  // clicked or otherwise selected.
  QRadialGradient gradient(-3, -3, 10);
  auto color1 = widget_->selected() == this ? Qt::green : Qt::yellow;
  auto color2 = widget_->selected() == this ? Qt::darkGreen : Qt::darkYellow;
  if (option->state & QStyle::State_Sunken) {
    gradient.setCenter(3, 3);
    gradient.setFocalPoint(3, 3);
    gradient.setColorAt(1, QColor(color1).light(120));
    gradient.setColorAt(0, QColor(color2).light(120));
  } else {
    gradient.setColorAt(0, color1);
    gradient.setColorAt(1, color2);
  }
  // Draw actual node.
  painter->setBrush(gradient);
  painter->setPen(QPen(Qt::black, 0));
  QRectF bound{x(), y(), width(), height()};
  painter->drawEllipse(bound);
  //painter->drawRect(bound);
  painter->drawText(bound, Qt::AlignCenter, entity_->id());
}

QVariant node::itemChange(GraphicsItemChange change, const QVariant& value) {
  switch (change) {
    case ItemPositionHasChanged:
      for (auto e : edges_)
        e->adjust();
      widget_->itemMoved();
      break;
    default:
      break;
  };
  return QGraphicsItem::itemChange(change, value);
}

void node::mousePressEvent(QGraphicsSceneMouseEvent* event) {
  widget_->selected(this);
  update();
  QGraphicsItem::mousePressEvent(event);
}

void node::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  update();
  QGraphicsItem::mouseReleaseEvent(event);
}
