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

struct Order {
  int key;
  int price;
  int quantity;
};

// Color codes
constexpr std::string RESET = "\033[0m";
constexpr std::string RED = "\033[31m";
constexpr std::string GREEN = "\033[32m";
constexpr std::string YELLOW = "\033[33m";
constexpr std::string BLUE = "\033[34m";
constexpr std::string MAGENTA = "\033[35m";
constexpr std::string CYAN = "\033[36m";
constexpr std::string WHITE = "\033[37m";

constexpr std::size_t D = 2;     // Global constant for the degree of the B+ tree
constexpr std::size_t M = D * 2; // Max number of keys in an internal node
constexpr std::size_t N = M + 1; // Max number of children in an internal node

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
  friend std::ostream &operator<<(std::ostream &os, const Node &node);
};
// Now, let's define the operator<< function
inline std::ostream &operator<<(std::ostream &os, const Node &node) {
  os << std::format("Node {}[{}]{} (size: {}): ", node.is_leaf ? GREEN : BLUE, node.is_leaf ? "Leaf" : "Internal",
                    RESET, node.size);

  // os << (node.is_leaf ? GREEN : BLUE) << (node.is_leaf ? "Leaf" : "Internal") << RESET << " Node (size: " <<
  // node.size
  //    << "): ";

  // Print keys
  os << "Keys: [";
  for (size_t i = 0; i < M; ++i) {
    if (node.keys[i].has_value()) {
      os << node.keys[i].value();
    } else {
      os << "_";
    }
    if (i < node.size - 1)
      os << ", ";
  }
  os << "]";

  // Print children or orders
  if (!node.is_leaf) {
    os << ", Children: [";
    for (size_t i = 0; i <= N; ++i) {
      if (node.children[i].has_value()) {
        os << "Node*";
      } else {
        os << "Null";
      }
      if (i < node.size)
        os << ", ";
    }
    os << "]";
  } else {
    os << ", Orders: [";
    for (size_t i = 0; i < node.size; ++i) {
      if (node.children[i].has_value()) {
        if (auto order = std::get_if<Order *>(&*node.children[i])) {
          os << "(" << (*order)->key << "," << (*order)->price << "," << (*order)->quantity << ")";
        } else {
          os << "InvalidOrder";
        }
      } else {
        os << "null";
      }
      if (i < node.size - 1)
        os << ", ";
    }
    os << "]";
  }

  return os;
}

class BPlusTree {
  std::optional<Node *> root;

public:
  BPlusTree() = default;
  [[nodiscard]] std::optional<Node *> get_root() const noexcept { return root; }
  friend std::ostream &operator<<(std::ostream &os, const BPlusTree &tree);

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
        std::cout << "Order found" << std::endl;
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
    if (!root) {
      std::cout << "Tree is empty. Nothing to remove." << std::endl;
      return;
    }
    std::cout << "root size " << root.value()->size << std::endl;

    Node *cursor = root.value();
    move_to_leaf(cursor, key);
    std::cout << "cursor: " << (cursor->keys[0]).value() << (cursor->keys[1]).value() << std::endl;

    if (!cursor) {
      std::cout << "Key " << key << " not found in the tree." << std::endl;
      return;
    }

    remove_from_leaf(cursor, key);

    if (root.value()->size == 0 && !root.value()->is_leaf) {
      // std::cout << "i'm herer  root is empty" << std::endl;
      Node *new_root = std::get<Node *>(*root.value()->children[0]);
      delete root.value();
      root.reset();
      root = new_root;
      new_root->parent = std::nullopt;
    }
    std::cout << "removed leaf" << std::endl;
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
  [[nodiscard]] std::string node_to_string(const Node &cursor, int level) const {
    std::ostringstream oss;
    const std::string indent(level * 4, ' ');
    oss << std::format("{}Node {}[{}]{} (size: {}): ", indent, cursor.is_leaf ? GREEN : BLUE,
                       cursor.is_leaf ? "Leaf" : "Internal", RESET, cursor.size);
    // Transform the array of std::optional<int> into a view of strings
    auto transformed = cursor.keys | std::views::transform([](const auto &opt) {
                         return opt ? std::to_string(*opt) : "x"; // Convert optional to string
                       });
    // Collect the transformed values into a vector
    std::vector<std::string> string_values{transformed.begin(), transformed.end()}; // Correctly initialize vector
    for (size_t i = 0; i < string_values.size(); ++i) {
      oss << string_values[i];
      if (i < string_values.size() - 1) {
        oss << ", ";
      }
    }
    std::string joined = oss.str();
    std::string formatted = std::format("[{}]\n", joined);

    // Print children for inner nodes
    if (!cursor.is_leaf) {
      for (size_t i = 0; i <= cursor.size; ++i) {
        if (cursor.children[i]) {
          oss << std::format("{}  Child {}:\n", indent, i);
          oss << node_to_string(*std::get<Node *>(*cursor.children[i]), level + 1);
        }
      }
    } else {
      // Print Order pointers for leaf nodes
      for (size_t i = 0; i < cursor.size; ++i) {
        if (cursor.children[i]) {
          const auto *order = std::get<Order *>(*cursor.children[i]);
          oss << std::format("{}  Order {}: Key={}, Price={}, Quantity={}\n", indent, i, order->key, order->price,
                             order->quantity);
        }
      }
    }
    oss << "\n";

    return oss.str();
  }
  // std::string node_to_string(Node *cursor, int level) const {
  //   std::ostringstream oss;
  //   std::string indent(level * 4, ' ');

  //  oss << indent << "Node [" << (cursor->is_leaf ? "Leaf" : "Internal") << "] (size: " << cursor->size << "): ";

  //  // Print keys
  //  oss << "Keys: [";
  //  for (size_t i = 0; i < cursor->size; ++i) {
  //    oss << cursor->keys[i].value();
  //    if (i < cursor->size - 1)
  //      oss << ", ";
  //  }
  //  oss << "]";
  //  oss << "\n";

  //  // Print children for internal nodes
  //  if (!cursor->is_leaf) {
  //    for (size_t i = 0; i <= cursor->size; ++i) {
  //      if (cursor->children[i]) {
  //        oss << indent << "  Child " << i << ":\n";
  //        oss << node_to_string(std::get<Node *>(*cursor->children[i]), level + 1);
  //      }
  //    }
  //  } else {
  //    // Print Order pointers for leaf nodes
  //    for (size_t i = 0; i < cursor->size; ++i) {
  //      if (cursor->children[i]) {
  //        Order *order = std::get<Order *>(*cursor->children[i]);
  //        oss << indent << "  Order " << i << ": Key=" << order->key << ", Price=" << order->price
  //            << ", Quantity=" << order->quantity << "\n";
  //      }
  //    }
  //  }

  //  return oss.str();
  //}

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
      insert_into_parent(parent, new_leaf);
    }
  };

  void insert_into_parent(Node *parent, Node *right_sibling) {
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
    int key = right_sibling->keys[0].value();

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
    parent->children[i + 1] = right_sibling;
    right_sibling->parent = parent;
    ++parent->size;

    // Check if the parent node is full and needs to be split
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

    std::optional<int> parent_key = new_internal->keys[D];
    for (size_t i = 0; i < new_internal->size - 1; ++i) {
      new_internal->keys[i].swap(new_internal->keys[i + 1]);
    }
    new_internal->keys[new_internal->size - 1] = std::nullopt;

    // Update the size of the original internal node
    cursor->size -= D;

    // If the internal node is a root, create a new root
    if (cursor->is_root()) {
      new_internal_root(cursor, new_internal);
    } else {
      // Insert the new key into the parent node
      Node *parent = cursor->parent.value();
      insert_into_parent(parent, new_internal);
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

    // Remove the key and shift the remaining keys
    for (size_t j = i; j < cursor->size - 1; ++j) {
      cursor->keys[j] = cursor->keys[j + 1];
      cursor->keys[j + 1] = 0;
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

    // If the leaf is not the root and now underflows, handle the underflow
    if (!cursor->is_root() && cursor->size < D) {
      handle_underflow(cursor);
    }
  }

  void handle_underflow(Node *cursor) {
    std::cout << "handle underflow " << cursor->keys[0].value() << std::endl;
    if (cursor->is_root()) {
      return;
    }

    Node *parent = cursor->parent.value();
    size_t cursor_index = find_child_index(parent, cursor);

    // Try to borrow from left sibling
    if (cursor_index > 0) {
      Node *left_sibling = std::get<Node *>(*parent->children[cursor_index - 1]);
      if (left_sibling->size > D) {
        borrow_from_left(cursor, left_sibling, parent, cursor_index);
        return;
      }
    }

    // Try to borrow from right sibling
    if (cursor_index > 0) {
      Node *right_sibling = std::get<Node *>(*parent->children[cursor_index + 1]);
      if (right_sibling->size > D) {
        borrow_from_right(cursor, right_sibling, parent, cursor_index);
        return;
      }
    }

    // If borrow is not possible, merge with a sibling
    if (cursor_index > 0) {
      std::cout << "merge left, cursor keys: " << cursor->keys[0].value() << ", " << cursor->keys[1].value()
                << std::endl;
      Node *left_sibling = std::get<Node *>(*parent->children[cursor_index - 1]);
      merge_with_left(cursor, left_sibling, parent, cursor_index - 1);
    } else {
      std::cout << "merge right, cursor keys: " << cursor->keys[0].value() << ", " << cursor->keys[1].value()
                << std::endl;
      Node *right_sibling = std::get<Node *>(*parent->children[cursor_index + 1]);
      merge_with_right(cursor, right_sibling, parent, cursor_index);
    }
  }

  void borrow_from_left(Node *cursor, Node *left_sibling, Node *parent, size_t index) {
    // Move the last key from the left sibling to the cursor node
    for (size_t i = cursor->size; i > 0; --i) {
      cursor->keys[i] = cursor->keys[i - 1];
      cursor->children[i] = cursor->children[i - 1];
    }

    // Move the last key from the left sibling to the cursor node
    cursor->keys[0] = left_sibling->keys[left_sibling->size - 1];
    cursor->children[0] = left_sibling->children[left_sibling->size - 1];
    ++cursor->size;
    --left_sibling->size;

    // Update the parent key
    parent->keys[index - 1] = cursor->keys[0];
  }

  void borrow_from_right(Node *cursor, Node *right_sibling, Node *parent, size_t index) {
    // Move the first key from the right sibling to the cursor node
    cursor->keys[cursor->size] = right_sibling->keys[0];
    cursor->children[cursor->size] = right_sibling->children[0];
    ++cursor->size;

    // Shift the keys in the right sibling
    for (size_t i = 0; i < right_sibling->size - 1; ++i) {
      right_sibling->keys[i] = right_sibling->keys[i + 1];
      right_sibling->children[i] = right_sibling->children[i + 1];
    }
    --right_sibling->size;

    // Update the parent key
    parent->keys[index] = right_sibling->keys[0];
  }

  void merge_with_left(Node *cursor, Node *left_sibling, Node *parent, size_t index) {
    std::cout << "merge with left" << std::endl;
    // Move all keys and children from cursor to left sibling;
    for (size_t i = 0; i < cursor->size; ++i) {
      left_sibling->keys[left_sibling->size + i] = cursor->keys[i];
      left_sibling->children[left_sibling->size + i] = cursor->children[i];
    }
    left_sibling->size += cursor->size;

    // Remove the key from parent and adjust children
    for (size_t i = index; i < parent->size - 1; ++i) {
      parent->keys[i] = parent->keys[i + 1];
      parent->children[i + 1] = parent->children[i + 2];
    }
    parent->keys[parent->size - 1] = 0;
    parent->children[parent->size].reset();
    --parent->size;

    // Delete the empty cursor node
    delete cursor;

    // If the parent is now underflowing, handle the underflow
    if (!parent->is_root() && parent->size < D) {
      handle_underflow(parent);
    }
    Node *tmp = std::get<Node *>(parent->children[0].value());
  }

  void merge_with_right(Node *cursor, Node *right_sibling, Node *parent, size_t index) {
    std::cout << std::boolalpha << "is leaf: " << cursor->is_leaf << " cursor size " << cursor->size
              << " right sibling size " << right_sibling->size << std::endl;

    std::cout << "cursor keys: " << cursor->keys[0].value() << ", " << cursor->keys[1].value()
              << ", right keys: " << right_sibling->keys[0].value() << ", " << right_sibling->keys[1].value()
              << std::endl;

    if (!cursor->is_leaf) {
      cursor->keys[cursor->size] = parent->keys[index];
      cursor->size += 1;
    }
    // Move all keys and children from right sibling to cursor
    for (size_t i = 0; i < right_sibling->size; ++i) {
      cursor->keys[cursor->size + i] = right_sibling->keys[i];
      cursor->children[cursor->size + 1 + i] = right_sibling->children[i];
    }
    cursor->children[cursor->size + 1 + right_sibling->size + 1] = right_sibling->children[right_sibling->size + 1];
    cursor->size += right_sibling->size + 1;

    // Remove the key from parent and adjust children
    for (size_t i = index; i < parent->size - 1; ++i) {
      parent->keys[i] = parent->keys[i + 1];
      parent->children[i + 1] = parent->children[i + 2];
    }
    --parent->size;

    // Delete the empty right sibling node
    delete right_sibling;

    // If the parent is now underflowing, handle the underflow
    if (!parent->is_root() && parent->size < D) {
      handle_underflow(parent);
    }
  }
};
inline std::ostream &operator<<(std::ostream &os, const BPlusTree &tree) {
  return os << (tree.root ? std::format("B+ Tree Structure:\n{}", tree.node_to_string(*tree.root, 0)) : "\nEmpty tree");
}

#endif // BPlusTree_H