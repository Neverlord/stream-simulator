#include <cmath>

#include <QPainter>

#include "edge.hpp"
#include "node.hpp"

namespace {

const double Pi = 3.14159265358979323846264338327950288419717;
const double TwoPi = 2.0 * Pi;

} // namespace <anonymous>

edge::edge(node* sourcenode, node* destnode) : arrow_size_(10) {
  setAcceptedMouseButtons(0);
  source_ = sourcenode;
  dest_ = destnode;
  source_->add(this);
  dest_->add(this);
  adjust();
}

int edge::type() const {
  return Type;
}

void edge::adjust() {
  if (!source_ || !dest_)
    return;
  QLineF line(mapFromItem(source_, 0, 0), mapFromItem(dest_, 0, 0));
  prepareGeometryChange();
  qreal length = line.length();
  if (length > qreal(20.)) {
    auto r = source_->radius() + 0.2;
    QPointF edgeOffset{(line.dx() * r) / length, (line.dy() * r) / length};
    source_point_ = line.p1() + edgeOffset;
    dest_point_ = line.p2() - edgeOffset;
  } else {
    source_point_ = dest_point_ = line.p1();
  }
}

QRectF edge::boundingRect() const {
  if (!source_ || !dest_)
    return {};
  qreal penWidth = 1;
  qreal extra = (penWidth + arrow_size_) / 2.0;
  return QRectF(source_point_, QSizeF(dest_point_.x() - source_point_.x(),
                                    dest_point_.y() - source_point_.y()))
    .normalized()
    .adjusted(-extra, -extra, extra, extra);
}

void edge::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
  if (!source_ || !dest_)
    return;
  QLineF line(source_point_, dest_point_);
  if (qFuzzyCompare(line.length(), qreal(0.)))
    return;
  // Draw the line.
  painter->setPen(QPen(Qt::black, 1, Qt::SolidLine,
                       Qt::RoundCap, Qt::RoundJoin));
  painter->drawLine(line);
  // Draw the arrow.
  double angle = ::acos(line.dx() / line.length());
  if (line.dy() >= 0)
    angle = TwoPi - angle;
  QPointF destArrowP1 =
    dest_point_
    + QPointF(sin(angle - Pi / 3) * arrow_size_, cos(angle - Pi / 3) * arrow_size_);
  QPointF destArrowP2 = dest_point_
                        + QPointF(sin(angle - Pi + Pi / 3) * arrow_size_,
                                  cos(angle - Pi + Pi / 3) * arrow_size_);
  painter->setBrush(Qt::black);
  painter->drawPolygon(QPolygonF() << line.p2() << destArrowP1 << destArrowP2);
}
