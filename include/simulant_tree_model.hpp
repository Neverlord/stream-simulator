#ifndef SIMULANT_TREE_MODEL_HPP
#define SIMULANT_TREE_MODEL_HPP

#include <memory>

#include <QAbstractItemModel>

#include "fwd.hpp"
#include "simulant_tree_item.hpp"

class simulant_tree_model : public QAbstractItemModel {
public:
  friend class simulant_tree_item;

  using pointer = simulant_tree_item*;

  simulant_tree_model(simulant* parent);

  QVariant data(const QModelIndex& index, int role) const override;

  Qt::ItemFlags flags(const QModelIndex& index) const override;

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;

  QModelIndex index(int row, int column,
                    const QModelIndex& parent) const override;

  QModelIndex parent(const QModelIndex& index) const override;

  int rowCount(const QModelIndex& parent) const override;

  int columnCount(const QModelIndex& parent) const override;

  inline pointer root() {
    return &root_;
  }

  /// Updates the model.
  void update();

private:
  simulant_tree_item* ptr(const QModelIndex& x) const;

  simulant* parent_;
  mutable simulant_tree_item root_;
};

#endif // SIMULANT_TREE_MODEL_HPP
