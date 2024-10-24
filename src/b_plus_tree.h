#ifndef BPlusTree_H
#define BPlusTree_H

// #include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <format>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ostream>
#include <ranges>
#include <sstream>
#include <string>
#include <strings.h>
#include <variant>

// helper type for the visitor
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

struct Order {
  int key;
  int price;
  int quantity;
};

std::string print_order(const Order *order, std::ostream &os) {
  return std::format("Key={}, Price={}, Quantity={}", order->key, order->price, order->quantity);
}

constexpr std::size_t D = 2;     // Global constant for the degree of the B+ tree
constexpr std::size_t M = D * 2; // Max number of keys in an internal node
constexpr std::size_t N = M + 1; // Max number of children in an internal node
constexpr std::size_t W = 3;     // Min number of keys in an internal node

struct Node {
  bool is_leaf;
  std::size_t size;                                                     // Current number of keys
  std::optional<Node *> parent;                                         // Pointer to the parent node
  std::array<std::optional<int>, M> keys;                               // Keys in the node
  std::array<std::optional<std::variant<Node *, Order *>>, N> children; // Child nodes or leaf nodes

public:
  Node(bool is_leaf = true) : is_leaf(is_leaf), size(0), parent(std::nullopt), keys{}, children{} {}

  //~Node() = delete;
  [[nodiscard]] bool is_full() const noexcept { return size == M; }
  [[nodiscard]] bool is_empty() const noexcept { return size == 0; }
  [[nodiscard]] bool is_root() const noexcept { return !parent.has_value(); }
  [[nodiscard]] bool is_leaf_root() const noexcept { return is_root() && is_leaf; }
  [[nodiscard]] bool is_internal_root() const noexcept { return is_root() && !is_leaf; }
};

// ANSI Color codes
namespace Color {
const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string BOLD = "\033[1m";
} // namespace Color

void print_indent(int level, std::ostream &os) {
  for (int i = 0; i < level; ++i) {
    os << "  ";
  }
}

std::string get_node_color(const std::optional<std::variant<Node *, Order *>> ov_node) {
  if (!ov_node.has_value())
    return Color::RESET;
  return std::visit(overloaded{
                        [](Node *arg) -> std::string {
                          if (arg->is_root()) {
                            return Color::RED + Color::BOLD; // Root nodes in bold red
                          } else if (arg->is_leaf) {
                            return Color::GREEN; // Leaf nodes in green
                          } else {
                            return Color::BLUE; // Internal nodes in blue
                          }
                        },
                        [](Order *arg) -> std::string { return Color::RESET; },
                    },
                    ov_node.value());
}

std::string get_node_index(const std::variant<Node *, Order *> v_node, size_t cursor_index = 0) {
  if (std::holds_alternative<Node *>(v_node)) {
    Node *node = std::get<Node *>(v_node);
    return std::format("{} [{}]: ", node->is_root() ? "Root" : std::format("Child {}", cursor_index),
                       node->is_leaf ? "LEAF" : "INTERNAL");
  } else {
    return std::format("Order {} ", cursor_index);
  }
}

std::string get_node_type(const std::variant<Node *, Order *> v_node, size_t cursor_index = 0) {
  if (std::holds_alternative<Node *>(v_node)) {
    Node *node = std::get<Node *>(v_node);
    return std::format("{} [{}]: ", node->is_root() ? "Root" : std::format("Child {}", cursor_index),
                       node->is_leaf ? "LEAF" : "INTERNAL");
  } else {
    return std::format("Order {} ", cursor_index);
  }
}

void print_keys(const std::variant<Node *, Order *> v_node, std::ostream &os) {
  os << "[";
  if (std::holds_alternative<Node *>(v_node)) {
    for (size_t i = 0; i < M; ++i) {
      Node *tmp_node = std::get<Node *>(v_node);
      if (i < tmp_node->size && tmp_node->keys[i].has_value()) {
        os << std::format("{:>{}}", tmp_node->keys[i].value(), W);
      } else {
        os << std::format("{:>{}}", "-", W);
      }
      if (i < M - 1)
        os << ", ";
    }
  } else {
    os << " o*";
  }
  os << "], ";
}

std::string get_child(const std::optional<std::variant<Node *, Order *>> ov_node) {
  if (!ov_node.has_value()) {
    return "  -";
  }
  return std::visit(overloaded{[](Node *arg) -> std::string { return "  +"; },
                               [](Order *arg) -> std::string {
                                 return std::format("Key={}, Price={}, Quantity={}", arg->key, arg->price,
                                                    arg->quantity);
                               }},
                    ov_node.value());
}

void print_children(const std::variant<Node *, Order *> v_node, std::ostream &os) {
  if (std::holds_alternative<Node *>(v_node)) {
    // Node *tmp_node = std::get<Node *>(v_node);
    if (auto tmp_node = std::get<Node *>(v_node); !tmp_node->is_leaf) {
      os << "[";
      for (size_t i = 0; i <= M; ++i) {
        if (i <= tmp_node->size && tmp_node->children[i].has_value()) {
          os << get_child(tmp_node->children[i]);
        } else {
          os << "  -";
        }
        if (i < M) {
          os << ", ";
        }
      }
      os << "] ";
    }
  }
}

void print_size(const std::variant<Node *, Order *> v_node, std::ostream &os) {
  if (std::holds_alternative<Node *>(v_node)) {
    Node *tmp_node = std::get<Node *>(v_node);
    os << "(size: " << tmp_node->size << ")" << "\n";
  } else {
    os << "Invalid type ";
  }
}

void print_leaf(const std::variant<Node *, Order *> v_node, int level, bool ignore_order, std::ostream &os) {
  if (ignore_order) {
    return;
  }
  Node *leaf_node = std::get<Node *>(v_node);
  for (std::size_t i = 0; i < leaf_node->size; ++i) {
    print_indent(level + 1, os);
    os << "Order " << i << ": ";
    if (auto order = leaf_node->children[i]) {
      os << get_child(order) << "\n";
    }
  }
}
void print_parent(const std::optional<Node *> o_node, std::ostream &os) {
  if (o_node.has_value()) {
    if (o_node.value()->keys[0].has_value()) {
      os << "[" << o_node.value()->keys[0].value() << "], ";
    } else {
      os << Color::RED << "invalid, " << Color::RESET;
    }
  } else {
    os << "[ - ], ";
  }
}

void print_subtree_recursive(const std::optional<std::variant<Node *, Order *>> ov_node, int level, bool ignore_order,
                             size_t cursor_index, std::ostream &os) {
  if (!ov_node.has_value()) {
    return;
  }
  std::string node_color = get_node_color(ov_node);
  print_indent(level, os);
  os << node_color << get_node_type(ov_node.value(), cursor_index) << Color::RESET;
  print_keys(ov_node.value(), os);
  print_parent(std::get<Node *>(ov_node.value())->parent, os);
  print_children(ov_node.value(), os);
  print_size(ov_node.value(), os);
  // For leaf nodes, print the actual orders
  if (std::holds_alternative<Node *>(ov_node.value())) {
    Node *tmp_node = std::get<Node *>(ov_node.value());
    if (tmp_node->is_leaf) {
      print_leaf(ov_node.value(), level, ignore_order, os);
    }
    // Recursively print children for internal nodes
    else {
      for (size_t i = 0; i <= tmp_node->size; ++i) {
        if (tmp_node->children[i].has_value()) {
          auto child = tmp_node->children[i];
          print_subtree_recursive(child, level + 1, ignore_order, i, os);
          //}
        }
      }
    }
  }
}

void check_variant(std::variant<Node *, Order *> &var) {
  std::visit(overloaded{
                 [](Node *arg) { std::cout << "Node* " << std::endl; },
                 [](Order *arg) { std::cout << "Order* " << arg->price << std::endl; },
             },
             var);
}

void log(const std::optional<std::variant<Node *, Order *>> &cursor, const std::string &file, int line,
         const std::string &func) {
  std::cout << file << ", " << line << ", " << func << ": " << std::endl;
  print_subtree_recursive(cursor, 0, false, 0, std::cout);
}

class BPlusTree {
  std::optional<Node *> root;

public:
  BPlusTree() = default;
  [[nodiscard]] std::optional<Node *> get_root() const noexcept { return root; }

  std::optional<Node *> range_search(int key) const {
    if (!root) {
      return std::nullopt;
    } else {
      Node *cursor = root.value();
      move_to_leaf(cursor, key);
      return cursor;
    }
  };

  std::optional<Order *> search(int key) const {
    if (!root) {
      return std::nullopt;
    } else {
      Node *cursor = root.value();

      move_to_leaf(cursor, key);

      // At the leaf node, perform a search for the key
      size_t j = 0;
      while (j < cursor->size && key != cursor->keys[j]) {
        ++j;
      }
      auto order_opt = cursor->children[j];
      if (order_opt) {
        return std::get<Order *>(*order_opt);
      }

      return std::nullopt; // Key not found
    }
  };

  void insert(Order *order) {
    /*
    Use the B-tree search algorithm to find
        the leaf node L where the x belongs

    Let:
         L = leaf node used to insert x, px
    */
    if (!root) {
      root = new Node(true);
    }

    Node *cursor = root.value();
    move_to_leaf(cursor, order->key);

    // Insert the key into the leaf node
    insert_into_leaf(cursor, order);
  };

  void remove(int key) {
    /*
      if ( x not found ){
         return;
      }
      else {
         Shift keys to delete the key x
         and its record ptr from the node
      }
    */
    if (!root) {
      std::cout << "Tree is empty. Nothing to remove." << std::endl;
      return;
    }
    Node *cursor = root.value();

    // Use the B-tree search algorithm to find the leaf node L where the x belongs
    move_to_leaf(cursor, key);
    if (!cursor) {
      std::cout << "Key " << key << " not found in the tree." << std::endl;
      return;
    }
    remove_from_leaf(cursor, key);

    if (root.value()->size == 0 && !root.value()->is_leaf) {
      Node *new_root = std::get<Node *>(*root.value()->children[0]);
      delete root.value();
      root.reset();
      root = new_root;
      new_root->parent = std::nullopt;
    }
  }

  /*
                |7|
        |3,5|,              |9|
  |1,2|, |3,4|, |5,6|, |7,8|, |9,10|

                |7|
         |5|              |9|
  |1,2,4|, |5,6|,  |7,8|, |9,10|


            |5, 7, 9|
  |1,2,4|, |5,6|,  |7,8|, |9,10|
  */

private:
  void move_to_leaf(Node *&cursor, int key) const {
    while (!cursor->is_leaf) {
      size_t i = 0;
      while (i < cursor->size && key >= cursor->keys[i]) {
        ++i; // Find the index of the child to follow
      }
      if (i < cursor->size) {
        // Access the child node directly
        if (auto child_opt = cursor->children[i]; child_opt) {
          cursor = std::get<Node *>(*child_opt);
        }
      } else {
        // Access the last child node if we're at the end
        if (auto child_opt = cursor->children[cursor->size]; child_opt) {
          cursor = std::get<Node *>(*child_opt);
        }
      }
    }
  };

  void new_internal_root(Node *left, Node *right) {
    Node *new_root = new Node(false);
    new_root->size = 1;
    new_root->keys[0] = right->keys[0];
    for (size_t i = 1; i < right->size; ++i) {
      right->keys[i - 1] = right->keys[i];
    }
    right->size--;
    right->keys[right->size] = std::nullopt;

    new_root->children[0] = left;
    new_root->children[1] = right;
    left->parent = new_root;
    right->parent = new_root;
    root = new_root;
  }

  void new_outer_root(Node *left, Node *right) {
    Node *new_root = new Node(false);
    new_root->size = 1;
    new_root->keys[0] = right->keys[0];
    new_root->children[0] = left;
    new_root->children[1] = right;
    left->parent = new_root;
    right->parent = new_root;
    root = new_root;
  };

  void insert_into_leaf(Node *cursor, Order *order) {
    size_t i = 0;

    // Find the position to insert the new key
    while (i < cursor->size && cursor->keys[i].has_value()) {
      if (cursor->keys[i].value() >= order->key) {
        break;
      }
      ++i;
    }

    // Shift keys and children to make room for the new key
    for (size_t j = cursor->size; j > i; --j) {
      // Shift the keys
      cursor->keys[j].swap(cursor->keys[j - 1]);
      // Shift the associated children
      cursor->children[j].swap(cursor->children[j - 1]);
    }

    // Insert the new key and order
    cursor->keys[i] = order->key;
    // Assuming order is the value stored in the leaf
    cursor->children[i] = order;
    ++cursor->size;

    //  Check if the leaf node is full and needs to be split
    if (cursor->is_full()) {
      split_leaf(cursor);
    }
  };

  void split_leaf(Node *cursor) {
    /*
        -------------------------------------------
        Leaf node L has n keys + n ptrs (full !!!):

           L: |p1|k1|p2|k2|....|pn|kn|next|
        -------------------------------------------
    */
    /*
        Make the following virtual node
              by inserting px,x in L:

             |p1|k1|p2|k2|...|px|x|...|pn|kn|next|
        (There are (n+1) keys in this node !!!)

         Split this node in two 2 "equal" halves:

             Let: m = ⌈(n+1)/2⌉
             L:  |p1|k1|p2|k2|...|pm-1|km-1|R|       <--- use the old node for L
             R:  |pm|km|....|px|x|...|pn|kn|next|   <--- use a new node for R

        ==================================================
        We need to fix the information at 1 level up to
        direct the search to the new node R

        We know that all keys in R: >= km
        ==================================================
    */
    // Create a new leaf node
    Node *new_leaf = new Node(true);
    // Move half the keys to the new leaf node
    new_leaf->size = D;

    //  Move the last D keys to the new leaf node
    for (size_t i = 0; i < D; ++i) {
      // Move the keys
      new_leaf->keys[i].swap(cursor->keys[i + D]);
      // Move the associated children
      new_leaf->children[i].swap(cursor->children[i + D]);
    }

    // Update the size of the original leaf node
    cursor->size = D;

    // If the leaf is a root, create a new root
    if (cursor->is_root()) {
      new_outer_root(cursor, new_leaf);
    } else {
      // Insert the new key into the parent node
      Node *parent = cursor->parent.value();
      insert_leaf_into_parent(parent, new_leaf);
    }
  };

  void insert_internal_into_parent(Node *parent, Node *right) {
    /*
        ========================================================
         Insert  (x, RSub(x)) into internal node N of B+-tree
        ========================================================
    */
    /*
    InsertInternal( x, rx, N )
    {
        if ( N ≠ full )
        {
            Shift keys to make room for x
            insert (x, rx) into its position in N

            return
        }
        else
        {
        -------------------------------------------
        Internal node N has: n keys + n+1 node ptrs:

            N: |p1|k1|p2|k2|....|pn|kn|pn+1|
        -------------------------------------------

        Make the following virtual node
            by inserting x,rx in N:

            |p1|k1|p2|k2|...|x|rx|...|pn|kn|pn+1|
            (There are (n+2) pointers in this node !!!)

        Split this node into 3 parts:
            1.  Take the middle key out
            2.  L = the "left"  half (use the old node to do this)
            3.  R = the "right" half (create a new node to do this)

                    Let: m = ⌈(n+1)/2⌉
                        1. Take km out
                        2. L =  |p1|k1|p2|k2|...|pm-1|km-1|pm|         (old node N)
                        3. R =  |pm+1|km+1|....|x|rx|...|pn|kn|pn+1|   (new node)

        if ( N == root )   // N is same node as L
        {
            Make a new root node containing (L, km, R)

            return;
        }
        else
        {
            InsertInternal( (km, R), parent(N));    // Recurse !!
        }
      }
    }
    */
    int key = right->keys[0].value();
    size_t i = 0;

    for (size_t j = 0; j < right->size - 1; ++j) {
      right->keys[j] = right->keys[j + 1];
    }
    right->size--;
    right->keys[right->size] == std::nullopt;

    // Find the correct position to insert the new key in the parent node
    while (i < parent->size && parent->keys[i] < key) {
      ++i;
    }

    // Shift keys and children to make room for the new key
    for (size_t j = parent->size; j > i; --j) {
      // Shift the keys
      parent->keys[j].swap(parent->keys[j - 1]);
      // Shift the child pointers
      parent->children[j + 1].swap(parent->children[j]);
    }

    // Insert the new key and the pointer to the new node
    parent->keys[i] = key;
    parent->children[i + 1] = right;
    right->parent = parent;
    ++parent->size;

    //  Check if the parent node is full and needs to be split
    if (parent->is_full()) {
      split_internal(parent);
    }
  }

  void insert_leaf_into_parent(Node *parent, Node *right) {
    /*
        ========================================================
         Insert  (x, RSub(x)) into internal node N of B+-tree
        ========================================================
    */
    /*
    InsertInternal( x, rx, N )
    {
        if ( N ≠ full )
        {
            Shift keys to make room for x
            insert (x, rx) into its position in N

            return
        }
        else
        {
        -------------------------------------------
        Internal node N has: n keys + n+1 node ptrs:

            N: |p1|k1|p2|k2|....|pn|kn|pn+1|
        -------------------------------------------

        Make the following virtual node
            by inserting x,rx in N:

            |p1|k1|p2|k2|...|x|rx|...|pn|kn|pn+1|
            (There are (n+2) pointers in this node !!!)

        Split this node into 3 parts:
            1.  Take the middle key out
            2.  L = the "left"  half (use the old node to do this)
            3.  R = the "right" half (create a new node to do this)

                    Let: m = ⌈(n+1)/2⌉
                        1. Take km out
                        2. L =  |p1|k1|p2|k2|...|pm-1|km-1|pm|         (old node N)
                        3. R =  |pm+1|km+1|....|x|rx|...|pn|kn|pn+1|   (new node)

        if ( N == root )   // N is same node as L
        {
            Make a new root node containing (L, km, R)

            return;
        }
        else
        {
            InsertInternal( (km, R), parent(N));    // Recurse !!
        }
      }
    }
    */
    int key = right->keys[0].value();
    size_t i = 0;

    // Find the correct position to insert the new key in the parent node
    while (i < parent->size && parent->keys[i] < key) {
      ++i;
    }

    // Shift keys and children to make room for the new key
    for (size_t j = parent->size; j > i; --j) {
      // Shift the keys
      parent->keys[j].swap(parent->keys[j - 1]);
      // Shift the child pointers
      parent->children[j + 1].swap(parent->children[j]);
    }

    // Insert the new key and the pointer to the new node
    parent->keys[i] = key;
    parent->children[i + 1] = right;
    right->parent = parent;
    ++parent->size;

    //  Check if the parent node is full and needs to be split
    if (parent->is_full()) {
      split_internal(parent);
    }
  }

  void split_internal(Node *cursor) {
    // Create a new internal node
    Node *new_internal = new Node(false);
    // Move half the keys to the new internal node
    new_internal->size = D;

    // Move the last D keys to the new internal node
    for (size_t j = 0; j < D; ++j) {
      new_internal->keys[j].swap(cursor->keys[j + D]);
      new_internal->children[j].swap(cursor->children[j + D + 1]);
      // Update the parent of the child node
      std::get<Node *>(*new_internal->children[j])->parent = new_internal;
    }

    // Update the size of the original internal node
    cursor->size -= D;

    // If the internal node is a root, create a new root
    if (cursor->is_root()) {
      new_internal_root(cursor, new_internal);
    } else {
      // Insert the new key into the parent node
      Node *parent = cursor->parent.value();
      insert_internal_into_parent(parent, new_internal);
    }
  }

  size_t find_child_index(Node *cursor, Node *child) {
    for (size_t i = 0; i <= cursor->size; ++i) {
      if (std::get<Node *>(cursor->children[i].value()) == child) {
        return i;
      }
    }
    return -1; // This should never happen if the tree is correctly structured
  }

  void remove_from_leaf(Node *cursor, int key) {
    size_t i = 0;
    while (i < cursor->size && key > cursor->keys[i]) {
      ++i;
    }
    if (i == cursor->size) {
      std::cout << "Key not found in the tree" << std::endl;
      return;
    }

    /*
    if ( L has ≥ ⌊(n+1)/2⌋ keys ) At least half full
    {
       return;   // Done
    }
    else
    {
        ---------------------------------------------------------
        L underflows: fix the size of L with transfer or merge
        ---------------------------------------------------------
        if ( leftSibling(L) has ≥ ⌊(n+1)/2⌋ + 1 keys ){
          1. transfer last key and ptr from leftSibling(L) into L;
          2. update the search key in the parent node;
        } else if ( rightSibling(L) has ≥ ⌊(n+1)/2⌋ + 1 keys ){
          1. transfer first key and ptr from rightSibling(L) into L;
          2. update the search key in the parent node;
        } else if ( leftSibling(L) exists ) {
          1. Merge leftSibling(L) + L + L's last pointer into leftSibling(L);        // Node L will be deleted !!
          2. Delete key and right subtree in parent node;  // discuss next !!
        } else {
          1. Merge L + rightSibling(L) + last pointer into L;             // rightSibling(L) will be deleted !!!
                2. Delete key and right subtree in parent node; // discuss next !!
        }
      }
    */
    // Remove the key and shift the remaining keys
    for (size_t j = i; j < cursor->size - 1; ++j) {
      cursor->keys[j] = cursor->keys[j + 1];
      cursor->keys[j + 1] = std::nullopt;
      cursor->children[j] = cursor->children[j + 1];
      cursor->children[j + 1] = std::nullopt;
    }
    --cursor->size;

    // If the cursor is the root and now empty, the tree becomes empty
    if (cursor->is_root() && cursor->size == 0) {
      delete cursor;
      root = nullptr;
      return;
    }
    //  If the leaf is not the root and now underflows, handle the underflow
    if (!cursor->is_root() && cursor->size < D) {
      handle_underflow(cursor);
    }
  }

  void handle_underflow(Node *cursor) {
    if (cursor->is_root()) {
      return;
    }

    Node *parent = cursor->parent.value();

    size_t cursor_index = find_child_index(parent, cursor);
    // Try to borrow from left sibling
    if (cursor_index > 0) {
      Node *left_sibling = std::get<Node *>(*parent->children[cursor_index - 1]);
      if (left_sibling->size > D) {
        redistribute_from_left(cursor, left_sibling, parent, cursor_index);
        return;
      }
    }

    // Try to borrow from right sibling
    if (cursor_index >= 0 && cursor_index < parent->size - 1) {
      Node *right_sibling = std::get<Node *>(*parent->children[cursor_index + 1]);
      if (right_sibling->size > D) {
        redistribute_from_right(cursor, right_sibling, parent, cursor_index);
        return;
      }
    }

    // If borrow is not possible, merge with a sibling
    if (cursor_index > 0) {
      std::cout << "Merging with left node" << std::endl;
      Node *left = std::get<Node *>(parent->children[cursor_index - 1].value());
      // print_subtree_recursive(left, 4, true, 0, std::cout);
      // print_subtree_recursive(cursor, 4, true, 0, std::cout);
      merge_with_left(cursor, left, parent, cursor_index - 1);
      // print_subtree_recursive(parent, 0, true, 0, std::cout);
    } else {
      std::cout << "Merging with right node" << std::endl;
      Node *right = std::get<Node *>(parent->children[cursor_index + 1].value());
      merge_with_right(cursor, right, parent, cursor_index);
    }
  }

  void redistribute_from_left(Node *cursor, Node *left, Node *parent, size_t index) {
    // Move the last key from the left sibling to the cursor node
    for (size_t i = cursor->size; i > 0; --i) {
      cursor->keys[i].swap(cursor->keys[i - 1]);
      cursor->children[i].swap(cursor->children[i - 1]);
    }
    ++cursor->size;

    // Move the last key from the left sibling to the cursor node
    cursor->keys[0].swap(left->keys[left->size - 1]);
    cursor->children[0].swap(left->children[left->size - 1]);
    if (cursor->children[0].has_value()) {
      if (auto tmp = cursor->children[0].value(); std::holds_alternative<Node *>(tmp)) {
        std::get<Node *>(tmp)->parent = cursor;
      } else {
        std::cout << "Invalid type in merge_with_right" << std::endl;
      }
    } else {
      std::cout << "No value in redistribute_with_right" << std::endl;
    }
    --left->size;

    // Update the parent key
    parent->keys[index - 1] = cursor->keys[0];
  }

  void redistribute_from_right(Node *cursor, Node *right_sibling, Node *parent, size_t index) {
    // Move the first key from the right sibling to the cursor node
    cursor->keys[cursor->size].swap(right_sibling->keys[0]);
    cursor->children[cursor->size].swap(right_sibling->children[0]);
    if (cursor->children[0].has_value()) {
      if (auto tmp = cursor->children[cursor->size].value(); std::holds_alternative<Node *>(tmp)) {
        std::get<Node *>(tmp)->parent = cursor;
      } else {
        std::cout << std::format("Invalid type in {}", __func__) << std::endl;
      }
    } else {
      std::cout << std::format("No value in {}", __func__) << std::endl;
    }
    ++cursor->size;

    // Shift the keys in the right sibling
    for (size_t i = 0; i < right_sibling->size - 1; ++i) {
      right_sibling->keys[i].swap(right_sibling->keys[i + 1]);
      right_sibling->children[i].swap(right_sibling->children[i + 1]);
    }
    --right_sibling->size;

    // Update the parent key
    parent->keys[index] = right_sibling->keys[0];
  }

  void merge_with_left(Node *cursor, Node *left, Node *parent, size_t index) {
    //  Move all keys and children from cursor to left sibling;
    // FIMXE (Peiyun) we might have overflow
    // print_subtree_recursive(cursor, 0, true, 0, std::cout);

    if (std::holds_alternative<Node *>(cursor->children[0].value())) {
      left->keys[left->size] = std::get<Node *>(cursor->children[0].value())->keys[0].value();
    }
    for (size_t i = 0; i < cursor->size; ++i) {
      left->keys[left->size + i + 1].swap(cursor->keys[i]);
      if (cursor->children[i].has_value()) {
        // std::variant<Node *, Order *> tmp = cursor->children[i].value();
        if (auto tmp = cursor->children[i].value(); std::holds_alternative<Node *>(tmp)) {
          std::get<Node *>(tmp)->parent = left;
        } else {
          std::cout << std::format("Invalid type in {}", __func__) << std::endl;
        }
      } else {
        std::cout << std::format("No value in {}", __func__) << std::endl;
      }
      left->children[left->size + i].swap(cursor->children[i]);
    }
    left->size += cursor->size;
    cursor->size = 0;

    // Remove the key from parent and adjust children
    for (size_t i = index; i < parent->size - 1; ++i) {
      parent->keys[i] = parent->keys[i + 1];
      parent->children[i + 1] = parent->children[i + 2];
    }
    parent->keys[parent->size - 1].reset();
    parent->children[parent->size].reset();
    --parent->size;

    // Delete the empty cursor node
    delete cursor;

    // If the parent is now underflowing, handle the underflow
    if (!parent->is_root() && parent->size < D) {
      handle_underflow(parent);
    }
  }

  void merge_with_right(Node *cursor, Node *right, Node *parent, size_t index) {

    // print_subtree_recursive(cursor, 0, true, 0, std::cout);
    if (!cursor->is_leaf) {
      cursor->keys[cursor->size] = parent->keys[index];
      cursor->size += 1;
    }
    //  Move all keys and children from right sibling to cursor
    for (size_t i = 0; i < right->size; ++i) {
      cursor->keys[cursor->size + i].swap(right->keys[i]);
      if (right->children[i].has_value()) {
        if (auto tmp = right->children[i].value(); std::holds_alternative<Node *>(tmp)) {
          std::get<Node *>(tmp)->parent = cursor;
        } else {
          std::cout << std::format("Invalid type in {}", __func__) << std::endl;
        }
      } else {
        std::cout << std::format("No value in {}", __func__) << std::endl;
      }
      cursor->children[cursor->size + i].swap(right->children[i]);
    }
    if (auto child = right->children[right->size]; child.has_value() && std::holds_alternative<Node *>(child.value())) {
      std::get<Node *>(child.value())->parent = cursor;
    }
    cursor->children[cursor->size + right->size].swap(right->children[right->size]);

    cursor->size += right->size;

    // Remove the key from parent and adjust children
    for (size_t i = index; i < parent->size - 1; ++i) {
      parent->keys[i] = parent->keys[i + 1];
      parent->children[i + 1] = parent->children[i + 2];
    }
    --parent->size;

    // Delete the empty right sibling node
    delete right;

    // If the parent is now underflowing, handle the underflow
    if (!parent->is_root() && parent->size < D) {
      handle_underflow(parent);
    }
  }
};

// Main print function that can be called on the tree
void print_bplus_tree(const BPlusTree tree, bool ignore_order = true, std::ostream &os = std::cout) {
  if (auto root = tree.get_root(); root) {
    os << Color::BOLD << "B+ Tree Structure:" << Color::RESET << "\n";
    print_subtree_recursive(root, 0, ignore_order, 0, os);
    os << std::endl;
    return;
  }
  os << Color::RED << "Empty tree" << Color::RESET << "\n";
}
#endif // BPlusTree_H
