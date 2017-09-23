#ifndef NODE_HPP
#define NODE_HPP

#include <vector>

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include "fwd.hpp"

/// A node is represented by a circle.
class node : public QGraphicsItem {
public:
  node(entity* parent_entity, dag_widget* parent_widget);

  entity* entity() {
    return entity_;
  }

  void add(edge* x);

  inline const std::vector<edge*> edges() const {
    return edges_;
  }

  inline qreal radius() const {
    return radius_;
  }

  inline qreal diameter() const {
    return radius_ * 2;
  }

  inline qreal x() const {
    return -radius_;
  }

  inline qreal y() const {
    return -radius_;
  }

  inline qreal width() const {
    return diameter();
  }

  inline qreal height() const {
    return diameter();
  }

  enum { Type = UserType + 1 };

  int type() const override;

  using QGraphicsItem::advance;

  bool advance();

  void calculateForces();

  QRectF boundingRect() const override;

  QPainterPath shape() const override;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

protected:
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) override;

  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
  class entity* entity_;
  dag_widget* widget_;
  std::vector<edge*> edges_;
  QPointF new_pos_;
  qreal radius_;
  qreal shadow_offset_;
};

#endif // NODE_HPP
