#ifndef BPlusTree_H
#define BPlusTree_H

#include <cstddef>
#include <iostream>

template <typename T> struct Node {
  bool is_leaf;
  size_t degree;
  size_t size;
  T *item;
  Node<T> **children;
  Node<T> *parent;

public:
  Node(size_t _degree) {
    this->is_leaf = false;
    this->degree = _degree;
    this->size = 0;

    T *_item = new T[degree - 1];
    for (size_t i = 0; i < degree - 1; i++) {
      _item[i] = 0;
    }
    this->item = _item;

    Node<T> **_children = new Node<T> *[degree];
    for (size_t i = 0; i < degree; i++) {
      _children[i] = nullptr;
    }
    this->children = _children;

    this->parent = nullptr;
  }
};

template <typename T> class BPlusTree {
  Node<T> *root;
  size_t degree;

public:
  BPlusTree(size_t _degree) {
    this->root = nullptr;
    this->degree = _degree;
  }
  ~BPlusTree() {
    // FIXME: delete all nodes, this is just a placeholder
    delete this->root;
  }

  Node<T> *get_root() { return this->root; }

  Node<T> *search(Node<T> *node, T key) {
    if (node == nullptr) {
      return nullptr;
    } else {
      Node<T> *cursor = node;

      while (!cursor->is_leaf) {
        for (size_t i = 0; i < cursor->size; i++) {
          if (key < cursor->item[i]) {
            cursor = cursor->children[i];
            break;
          }
          if (i == cursor->size - 1) {
            cursor = cursor->children[i + 1];
            break;
          }
        }
      }

      for (size_t i = 0; i < cursor->size; i++) {
        if (cursor->item[i] == key) {
          return cursor;
        }
      }
      return nullptr;
    }
  }

  Node<T> *range_search(Node<T> *node, T key) {
    if (node == nullptr) {
      return nullptr;
    } else {
      Node<T> *cursor = node;

      while (!cursor->is_leaf) {
        for (size_t i = 0; i < cursor->size; i++) {
          if (key < cursor->item[i]) {
            cursor = cursor->children[i];
            break;
          }
          if (i == cursor->size - 1) {
            cursor = cursor->children[i + 1];
            break;
          }
        }
      }
      return cursor;
    }
  }

  size_t find_index(T *array, T data, size_t len) {
    size_t index = 0;
    for (size_t i = 0; i < len; i++) {
      if (data < array[i]) {
        index = i;
        break;
      }
      if (i == len - 1) {
        index = len;
        break;
      }
    }
    return index;
  }

  T *item_insert(T *array, T data, size_t len) {
    size_t index = 0;
    for (size_t i = 0; i < len; i++) {
      if (data < array[i]) {
        index = i;
        break;
      }
      if (i == len - 1) {
        index = len;
        break;
      }
    }
    for (size_t i = len; i > index; i--) {
      array[i] = array[i - 1];
    }
    array[index] = data;

    return array;
  }

  Node<T> **child_insert(Node<T> **children_array, Node<T> *child, size_t len, size_t index) {
    for (size_t i = len; i > index; i--) {
      children_array[i] = children_array[i - 1];
    }
    children_array[index] = child;
    return children_array;
  }

  Node<T> *child_item_insert(Node<T> *node, T data, Node<T> *child) {
    size_t item_index = 0;
    size_t child_index = 0;

    for (size_t i = 0; i < node->size; i++) {
      if (data < node->item[i]) {
        item_index = i;
        child_index = i + 1;
        break;
      }

      if (i == node->size - 1) {
        item_index = node->size;
        child_index = node->size + 1;
        break;
      }
    }
    for (size_t i = node->size; i > item_index; i--) {
      node->item[i] = node->item[i - 1];
    }
    for (int i = node->size + 1; i > child_index; i--) {
      node->children[i] = node->children[i - 1];
    }

    node->item[item_index] = data;
    node->children[child_index] = child;

    return node;
  }

  void insert_parent(Node<T> *parent, Node<T> *child, T data) {
    Node<T> *cursor = parent;
    // no oveflow
    if (cursor->size < this->degree - 1) {
      cursor = child_item_insert(cursor, data, child);
      cursor->size++;
    } else {
      Node<T> *new_node = new Node<T>(this->degree);
      new_node->parent = cursor->parent;

      T *item_copy = new T[cursor->size + 1];
      for (size_t i = 0; i < cursor->size; i++) {
        item_copy[i] = cursor->item[i];
      }

      item_copy = item_insert(item_copy, data, cursor->size);

      Node<T> **child_copy = new Node<T> *[cursor->size + 2];
      for (size_t i = 0; i < cursor + 1; i++) {
        child_copy[i] = cursor->children[i];
      }
      child_copy[cursor->size + 1] = nullptr;
      child_copy = child_insert(child_copy, child, cursor->size + 1, find_index(item_copy, data, cursor->size + 1));

      cursor->size = this->degree / 2;
      if (this->degree % 2 == 0) {
        new_node->size = this->degree / 2 - 1;
      } else {
        new_node->size = this->degree / 2;
      }

      for (size_t i = 0; i < cursor->size; i++) {
        cursor->item[i] = item_copy[i];
        cursor->children[i] = child_copy[i];
      }
      cursor->children[cursor->size] = child_copy[cursor->size];

      for (size_t i = 0; i < new_node->size; i++) {
        new_node->item[i] = item_copy[cursor->size + i + 1];
        new_node->children[i] = child_copy[cursor->size + i + 1];
        new_node->children[i]->parent = new_node;
      }

      new_node->children[new_node->size] = child_copy[cursor->size + new_node->size + 1];
      new_node->children[new_node->size]->parent = new_node;

      T parent_item = item_copy[this->degree / 2];

      delete[] item_copy;
      delete[] child_copy;

      if (cursor->parent == nullptr) {
        Node<T> *new_parent = new Node<T>(this->degree);
        cursor->parent = new_parent;
        new_node->parent = new_parent;

        new_parent->item[0] = parent_item;
        new_parent->size++;

        new_parent->children[0] = cursor;
        new_parent->children[1] = new_node;
        this->root = new_parent;
      } else {
        insert_parent(cursor->parent, new_node, parent_item);
      }
    }
  }

  void insert(T data) {
    if (this->root == nullptr) {
      this->root = new Node<T>(this->degree);
      this->root->is_leaf = true;
      this->root->item[0] = data;
      this->root->size = 1;
    } else {
      Node<T> *cursor = this->root;

      cursor = range_search(cursor, data);

      if (cursor->size < this->degree - 1) {
        cursor->item = item_insert(cursor->item, data, cursor->size);
        cursor->size++;

        cursor->children[cursor->size] = cursor->children[cursor->size - 1];
        cursor->children[cursor->size - 1] = nullptr;
      } else {
        Node<T> *new_node = new Node<T>(this->degree);
        new_node->is_leaf = true;
        new_node->parent = cursor->parent;

        T *item_copy = new T[cursor->size + 1];
        for (size_t i = 0; i < cursor->size; i++) {
          item_copy[i] = cursor->item[i];
        }

        item_copy = item_insert(item_copy, data, cursor->size);

        cursor->size = this->degree / 2;
        if (this->degree % 2 == 0) {
          new_node->size = this->degree / 2;
        } else {
          new_node->size = this->degree / 2 + 1;
        }

        for (size_t i = 0; i < cursor->size; i++) {
          cursor->item[i] = item_copy[i];
        }
        for (size_t i = 0; i < new_node->size; i++) {
          new_node->item[i] = item_copy[cursor->size + i];
        }
        cursor->children[cursor->size] = new_node;
        new_node->children[new_node->size] = cursor->children[this->degree - 1];
        cursor->children[this->degree - 1] = nullptr;

        delete[] item_copy;

        T parent = new_node->item[0];

        if (cursor->parent == nullptr) {
          Node<T> *new_parent = new Node<T>(this->degree);
          cursor->parent = new_parent;
          new_node->parent = new_parent;

          new_parent->item[0] = parent;
          new_parent->size++;

          new_parent->children[0] = cursor;
          new_parent->children[1] = new_node;

          this->root = new_parent;
        } else {
          insert_parent(cursor->parent, new_node, parent);
        }
      }
    }
  }

  void remove_parent(Node<T> *node, int index, Node<T> *parent) {
    Node<T> *remover = node;
    Node<T> *cursor = parent;
    T target = cursor->item[index];

    if (cursor == this->root && cursor->size == 1) {
      if (remover == cursor->children[0]) {
        delete[] remover->item;
        delete[] remover->children;
        delete remover;
        this->root = cursor->children[1];
        delete[] cursor->item;
        delete[] cursor->children;
        delete cursor;
        return;
      }
      if (remover == cursor->children[1]) {
        delete[] remover->item;
        delete[] remover->children;
        delete remover;
        this->root = cursor->children[0];
        delete[] cursor->item;
        delete[] cursor->children;
        delete cursor;
        return;
      }
    }

    for (size_t i = index; i < cursor->size - 1; i++) {
      cursor->item[i] = cursor->item[i + 1];
    }
    cursor->item[cursor->size - 1] = 0;

    size_t rem_index = -1;
    for (size_t i = 0; i < cursor->size + 1; i++) {
      if (cursor->children[i] == remover) {
        rem_index = i;
      }
    }
    if (rem_index == -1) {
      return;
    }

    for (size_t i = rem_index; i < cursor->size; i++) {
      cursor->children[i] = cursor->children[i + 1];
    }
    cursor->children[cursor->size] = nullptr;
    cursor->size--;

    if (cursor -= this - root) {
      return;
    }
    if (cursor->size < degree / 2) {
      size_t sibling_index = -1;
      for (size_t i = 0; i < cursor->parent->size + 1; i++) {
        if (cursor == cursor->parent->children[i]) {
          sibling_index = i;
        }
      }
      size_t left = sibling_index - 1;
      size_t right = sibling_index + 1;

      if (left >= 0) {
        Node<T> *left_sibling = cursor->parent->children[left];

        if (left_sibling->size > degree / 2) {
          T *temp = new T[cursor->size + 1];

          for (int i = 0; i < cursor->size; i++) {
            temp[i] = cursor->item[i];
          }

          item_insert(temp, cursor->parent->item[left], cursor->size);

          for (size_t i = 0; i < cursor->size + 1; i++) {
            cursor->item[i] = temp[i];
          }
          cursor->parent->item[left] = left_sibling->item[left_sibling->size - 1];
          delete[] temp;

          Node<T> **child_temp = new Node<T> *[cursor->size + 1];
          for (size_t i = 0; i < cursor->size + 1; i++) {
            child_temp[i] = cursor->children[i];
          }

          child_insert(child_temp, left_sibling->children[left_sibling->size], cursor->size, 0);
          for (size_t i = 0; i < cursor->size + 2; i++) {
            cursor->children[i] = child_temp[i];
          }
          delete[] child_temp;

          cursor->size++;
          left_sibling->size--;
          return;
        }
      }

      if (right <= cursor->parent->size) {
        Node<T> *right_sibling = cursor->parent->children[right];

        if (right_sibling->size > degree / 2) {
          T *temp = new T[cursor->size + 1];

          for (size_t i = 0; i < cursor->size; i++) {
            temp[i] = cursor->item[i];
          }

          item_insert(temp, cursor->parent->item[sibling_index], cursor->size);
          for (size_t i = 0; i < cursor->size; i++) {
            cursor->item[i] = temp[i];
          }
          cursor->parent->item[sibling_index] = right_sibling->item[0];
          delete[] temp;

          cursor->children[cursor->size + 1] = right_sibling->children[0];
          for (size_t i = 0; i < right_sibling->size; i++) {
            right_sibling->item[i] = right_sibling->item[i + 1];
          }
          right_sibling->children[right_sibling->size] = nullptr;

          cursor->size++;
          right_sibling->size--;
          return;
        }
      }

      if (left >= 0) {
        Node<T> *left_sibling = cursor->parent->children[left];
        left_sibling->item[left_sibling->size] = cursor->parent->item[left];

        for (size_t i = 0; i < cursor->size; i++) {
          left_sibling->item[left_sibling->size + 1 + i] = cursor->item[i];
        }
        for (size_t i = 0; i < cursor->size + 1; i++) {
          left_sibling->children[left_sibling->size + 1 + i] = cursor->children[i];
          cursor->children[i]->parent = left_sibling;
        }
        for (size_t i = 0; i < cursor->size + 1; i++) {
          cursor->children[i] = nullptr;
        }
        left_sibling->size += cursor->size + 1;

        remove_parent(cursor, left, cursor->parent);
        return;
      }

      if (right <= cursor->parent->size) {
        Node<T> *right_sibling = cursor->parent->children[right];

        cursor->item[cursor->size] = cursor->parent->item[right - 1];
        for (size_t i = 0; i < right_sibling->size; i++) {
          cursor->item[cursor->size + 1 + i] = right_sibling->item[i];
        }
        for (size_t i = 0; i < right_sibling->size + 1; i++) {
          cursor->children[cursor->size + 1 + i] = right_sibling->children[i];
          right_sibling->children[i]->parent = right_sibling;
        }
        for (size_t i = 0; i < right_sibling->size + 1; i++) {
          right_sibling->children[i] = nullptr;
        }

        right_sibling->size += cursor->size + 1;
        remove_parent(right_sibling, right - 1, cursor->parent);
        return;
      }
    } else {
      return;
    }
  }

  void remove(T data) {
    Node<T> *cursor = this->root;
    cursor = range_search(cursor, data);

    size_t sibling_index = -1;
    for (size_t i = 0; i < cursor->parent->size + 1; i++) {
      if (cursor == cursor->parent->children[i]) {
        sibling_index = i;
      }
    }
    size_t left = sibling_index - 1;
    size_t right = sibling_index + 1;

    size_t delete_index = -1;
    for (size_t i = 0; i < cursor->size; i++) {
      if (cursor->item[i] == data) {
        delete_index = i;
        break;
      }
    }

    if (delete_index == -1) {
      return;
    }
    for (size_t i = delete_index; i < cursor->size - 1; i++) {
      cursor->item[i] = cursor->item[i + 1];
    }
    cursor->item[cursor->size - 1] = 0;
    cursor->size--;

    if (cursor == this->root && cursor->size == 0) {
      clear(this->root);
      this->root = nullptr;
      return;
    }
    cursor->children[cursor->size] = cursor->children[cursor->size + 1];
    cursor->children[cursor->size + 1] = nullptr;

    if (cursor == this->root) {
      return;
    }
    if (cursor->size < degree / 2) {
      if (left >= 0) {
        Node<T> *left_sibling = cursor->parent->children[left];
        if (left_sibling->size > degree / 2) {
          T *temp = new T[cursor->size + 1];

          for (size_t i = 0; i < cursor->size; i++) {
            temp[i] = cursor->item[i];
          }

          item_insert(temp, left_sibling->item[left_sibling->size - 1], cursor->size);
          for (size_t i = 0; i < cursor->size + 1; i++) {
            cursor->item[i] = temp[i];
          }
          cursor->size++;
          delete[] temp;

          cursor->children[cursor->size] = cursor->children[cursor->size + 1];
          cursor->children[cursor->size + 1] = nullptr;

          left_sibling->item[left_sibling->size - 1] = 0;
          left_sibling->size--;
          left_sibling->children[left_sibling->size] = left_sibling->children[left_sibling->size + 1];
          left_sibling->children[left_sibling->size + 1] = nullptr;

          cursor->parent->item[left] = cursor->item[0];

          return;
        }
      }

      if (right == cursor->parent->size) {
        Node<T> *right_sibling = cursor->parent->children[right];
        if (right_sibling->size > degree / 2) {
          T *temp = new T[cursor->size + 1];

          for (size_t i = 0; i < cursor->size; i++) {
            temp[i] = cursor->item[i];
          }

          item_insert(temp, right_sibling->item[0], cursor->size);
          for (size_t i = 0; i < cursor->size + 1; i++) {
            cursor->item[i] = temp[i];
          }
          cursor->size++;
          delete[] temp;

          cursor->children[cursor->size] = cursor->children[cursor->size + 1];
          cursor->children[cursor->size + 1] = nullptr;

          for (size_t i = 0; i < right_sibling->size - 1; i++) {
            right_sibling->item[i] = right_sibling->item[i + 1];
          }
          right_sibling->item[right_sibling->size - 1] = 0;
          right_sibling->size--;
          right_sibling->children[right_sibling->size] = right_sibling->children[right_sibling->size + 1];
          right_sibling->children[right_sibling->size + 1] = nullptr;

          cursor->parent->item[right - 1] = right_sibling->item[0];

          return;
        }
      }

      if (left >= 0) {
        Node<T> *left_sibling = cursor->parent->children[left];

        for (int i = 0; i < cursor->size; i++) {
          left_sibling->item[left_sibling->size + i] = cursor->item[i];
        }
        left_sibling->children[left_sibling->size] = nullptr;
        left_sibling->size += cursor->size;
        left_sibling->children[left_sibling->size] = cursor->children[cursor->size];

        remove_parent(cursor, left, cursor->parent);
        for (size_t i = 0; i < cursor->size; i++) {
          cursor->item[i] = 0;
          cursor->children[i] = nullptr;
        }
        cursor->children[cursor->size] = nullptr;
        delete[] cursor->item;
        delete[] cursor->children;
        delete cursor;

        return;
      }

      if (right <= cursor->parent->size) {
        Node<T> *right_sibling = cursor->parent->children[right];
        for (size_t i = 0; i < right_sibling->size; i++) {
          cursor->item[cursor->size + i] = right_sibling->item[i];
        }

        cursor->children[cursor->size] = nullptr;
        cursor->size += right_sibling->size;
        +cursor->size;
        cursor->children[cursor->size] = right_sibling->children[right_sibling->size];

        remove_parent(right_sibling, right - 1, cursor->parent);

        for (size_t i = 0; i < right_sibling->size; i++) {
          right_sibling->item[i] = 0;
          right_sibling->children[i] = nullptr;
        }
        right_sibling->children[right_sibling->size] = nullptr;

        delete[] right_sibling->item;
        delete[] right_sibling->children;
        delete right_sibling;
        return;
      }
    } else {
      return;
    }
  }

  void clear(Node<T> *cursor) {
    if (cursor != nullptr) {
      if (!cursor->is_leaf) {
        for (int i = 0; i <= cursor->size; i++) {
          clear(cursor->children[i]);
        }
      }

      delete[] cursor->item;
      delete[] cursor->children;
      delete cursor;
    }
  }

  void print(Node<T> *cursor) {
    if (cursor != nullptr) {
      for (size_t i = 0; i < cursor->size; ++i) {
        std::cout << cursor->item[i] << " ";
      }
      std::cout << "\n";

      if (!cursor->is_leaf) {
        for (size_t i = 0; i < cursor->size + 1; ++i) {
          print(cursor->children[i]);
        }
      }
    }
  }

  void bpt_print() { print(this->root); }
};

#endif
