#ifndef EDGE_HPP
#define EDGE_HPP

#include <QGraphicsItem>

#include "fwd.hpp"

class edge : public QGraphicsItem {
public:
  edge(node* sourcenode, node* destnode);

  inline node* source() const {
    return source_;
  }

  inline node* dest() const {
    return dest_;
  }

  void adjust();

  enum { Type = UserType + 2 };

  int type() const override;

protected:
  QRectF boundingRect() const override;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

private:
  node *source_;
  node* dest_;
  QPointF source_point_;
  QPointF dest_point_;
  qreal arrow_size_;
};

#endif // EDGE_HPP
