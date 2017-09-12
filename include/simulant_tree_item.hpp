#ifndef SIMULANT_TREE_ITEM_HPP
#define SIMULANT_TREE_ITEM_HPP

#include <memory>
#include <vector>

#include <QString>
#include <QVariant>

class simulant_tree_model;

/// Tree items are organized with paths.
class simulant_tree_item {
public:
  /// A path is represented as an ordered list of strings.
  using path = std::vector<QString>;

  using owning_pointer = std::unique_ptr<simulant_tree_item>;

  using children_vec = std::vector<owning_pointer>;

  simulant_tree_item(simulant_tree_model* model, simulant_tree_item* parent,
                     QString id, QVariant value);

  /// Returns the child at row `x`.
  simulant_tree_item* child(int x);

  /// Removes the child at row `x`.
  void remove_child(int x);

  /// Returns the number of children.
  int child_count() const;

  /// Returns the index of the child `id` or -1 on error.
  int index_of(const QString& id) const;

  /// Returns whether a child with ID `id` exists on this item.
  inline bool has_child(const QString& id) const {
    return index_of(id) == -1;
  }

  /// Returns the row of this item relative to its parent.
  int index_at_parent() const;

  /// Returns the rendering data at column `x`.
  QVariant data(int x) const;

  /// Returns the number of columns for rendering.
  int column_count() const;

  /// Recursive lookup function for accessing particular items by name.
  simulant_tree_item* lookup(path::const_iterator child_id,
                             path::const_iterator end);

  /// Recursive lookup function for accessing particular items by name.
  inline simulant_tree_item* lookup(const path& x) {
    return lookup(x.begin(), x.end());
  }

  /// Inserts a new item by name or updates an existing item.
  /// This operation fails if the parent does not exist.
  simulant_tree_item* insert_or_update(path::const_iterator child_id,
                                       path::const_iterator end,
                                       QVariant value);

  /// Inserts a new item by name or updates an existing item.
  /// This operation fails if the parent does not exist.
  inline simulant_tree_item* insert_or_update(const path& x, QVariant value) {
    return insert_or_update(x.begin(), x.end(), std::move(value));
  }

  /// Returns a pointer to the parent item.
  inline simulant_tree_item* parent() {
    return parent_;
  }

  /// Returns a pointer to the parent item.
  inline const simulant_tree_item* parent() const {
    return parent_;
  }

  /// Returns the string identifier for this item.
  inline const QString& id() const {
    return id_;
  }

  template <class F>
  void for_all_children(F& f) {
    for (auto& child : children_) {
      f(*child);
      child->for_all_children(f);
    }
  }

  inline const QVariant& value() const {
    return value_;
  }

  inline void value(QVariant x) {
    value_ = std::move(x);
  }

  QModelIndex index(int column = 0);

  inline bool stale() const {
    return stale_;
  }

  /// Marks this item and all of its children as stale until it gets updated.
  void mark_as_stale();

  /// Removes all stale children.
  void purge();

private:
  simulant_tree_model* model_;
  simulant_tree_item* parent_;
  QString id_;
  QVariant value_;
  children_vec children_;
  bool stale_;
};

#endif // SIMULANT_TREE_ITEM_HPP
