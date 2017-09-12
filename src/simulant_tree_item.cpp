#include "include/simulant_tree_item.hpp"

#include <iostream>

#include "include/simulant_tree_model.hpp"

simulant_tree_item::simulant_tree_item(simulant_tree_model* model,
                                       simulant_tree_item* parent, QString id,
                                       QVariant value)
  : model_(model),
    parent_(parent),
    id_(std::move(id)),
    value_(std::move(value)),
    stale_(false) {
  // nop
}

simulant_tree_item* simulant_tree_item::child(int x) {
  return children_[static_cast<size_t>(x)].get();
}

void simulant_tree_item::remove_child(int x) {
  auto i = children_.begin() + x;
  model_->beginRemoveColumns(index(), x, x);
  children_.erase(i);
  model_->endRemoveColumns();
}

int simulant_tree_item::child_count() const {
  return static_cast<int>(children_.size());
}

int simulant_tree_item::index_of(const QString& x) const {
  auto pred = [=](const owning_pointer& y) {
    return x == y->id();
  };
  auto e = children_.end();
  auto b = children_.begin();
  auto i = std::find_if(b, e, pred);
  if (i != e)
    return std::distance(b, i);
  return -1;
}

int simulant_tree_item::index_at_parent() const {
  if (!parent_)
    return 0;
  for (auto i = 0; i < parent_->child_count(); ++i)
    if (parent_->child(i) == this)
      return i;
  throw std::logic_error("unable to compute index_at_parent");
}

QVariant simulant_tree_item::data(int x) const {
  return x == 0 ? id_ : value_;
}

int simulant_tree_item::column_count() const {
  return 2;
}

simulant_tree_item* simulant_tree_item::lookup(path::const_iterator child_id,
                                               path::const_iterator end) {
  if (child_id == end)
    return this;
  auto pred = [=](owning_pointer& x) {
    return x->id() == *child_id;
  };
  auto e = children_.end();
  auto i = std::find_if(children_.begin(), e, pred);
  return i != e ? (*i)->lookup(++child_id, end) : nullptr;
}

simulant_tree_item*
simulant_tree_item::insert_or_update(path::const_iterator child_id,
                                     path::const_iterator end,
                                     QVariant value) {
  if (child_id == end) {
    if (stale_)
      stale_ = false;
    if (value_ != value) {
      value_.swap(value);
      auto ix = index(1);
      model_->dataChanged(ix, ix);
    }
    return this;
  }
  auto pred = [=](owning_pointer& x) {
    return x->id() == *child_id;
  };
  auto e = children_.end();
  auto i = std::find_if(children_.begin(), e, pred);
  if (i != e)
    return (*i)->insert_or_update(++child_id, end, std::move(value));
  if (child_id + 1 != end)
    return nullptr;
  auto ix = index();
  auto row = static_cast<int>(children_.size());
  model_->beginInsertRows(ix, row, row);
  children_.emplace_back(new simulant_tree_item(model_, this, *child_id,
                                                std::move(value)));
  model_->endInsertRows();
  return children_.back().get();
}

QModelIndex simulant_tree_item::index(int column) {
  if (parent_ == nullptr)
    return {};
  return model_->createIndex(index_at_parent(), column, this);
}

void simulant_tree_item::mark_as_stale() {
  if (!stale_)
    stale_ = true;
  for (auto& x : children_)
    x->mark_as_stale();
}

void simulant_tree_item::purge() {
  auto is_stale = [](const owning_pointer& x) {
    return x->stale();
  };
  auto is_not_stale = [](const owning_pointer& x) {
    return !x->stale();
  };
  auto e = children_.end();
  auto b = children_.begin();
  auto i = std::find_if(b, e, is_stale);
  if (i != e) {
    // Take out consecutive stale children at once by computing iterator
    // to the first *not* deleted child.
    auto j = std::find_if(i + 1, e, is_not_stale);
    // Compute IDs of first and last (inclusive) deleted rows.
    auto row_first = std::distance(b, i);
    auto row_last = std::distance(b, j) - 1;
    // Remove from model and vector.
    model_->beginRemoveRows(index(), row_first, row_last);
    children_.erase(i, j);
    model_->endRemoveRows();
    // Recurse until all stale children have been removed.
    purge();
  }
}
