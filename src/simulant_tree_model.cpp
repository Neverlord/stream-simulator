#include "simulant_tree_model.hpp"

#include "qstr.hpp"
#include "entity.hpp"
#include "simulant.hpp"

simulant_tree_model::simulant_tree_model(simulant* parent)
  : parent_(parent),
    root_(this, nullptr, parent->parent()->id(), QString::fromUtf8("<actor>")) {
  // nop
}

QVariant simulant_tree_model::data(const QModelIndex& index, int role) const {
  if (role != Qt::DisplayRole)
    return {};
  return ptr(index)->data(index.column());
}

Qt::ItemFlags simulant_tree_model::flags(const QModelIndex&) const {
  return Qt::ItemIsSelectable;
}

QVariant simulant_tree_model::headerData(int section, Qt::Orientation,
                                         int role) const {
  if (role != Qt::DisplayRole)
    return {};
  if (section == 0)
    return qstr("Field");
  else if (section == 1)
    return qstr("Value");
  return {};
}

QModelIndex simulant_tree_model::index(int row, int column,
                                       const QModelIndex& parent) const {
  auto item = ptr(parent);
  return createIndex(row, column, item->child(row));
}

QModelIndex simulant_tree_model::parent(const QModelIndex& index) const {
  auto item = ptr(index);
  if (item == &root_ || item->parent() == &root_)
    return {};
  return createIndex(item->index_at_parent(), index.column(), item->parent());
}

int simulant_tree_model::rowCount(const QModelIndex& index) const {
  return ptr(index)->child_count();
}

int simulant_tree_model::columnCount(const QModelIndex&) const {
  return 2;
}

simulant_tree_item* simulant_tree_model::ptr(const QModelIndex& x) const {
  auto vptr = x.internalPointer();
  return vptr == nullptr ? &root_ : reinterpret_cast<simulant_tree_item*>(vptr);
}

void simulant_tree_model::update() {
  parent_->serialize_state(root_);
}
