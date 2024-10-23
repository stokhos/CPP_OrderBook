#include <iomanip>
#include <iostream>
#include <string>

void print_indent(std::ostream &os, int level) {
  for (int i = 0; i < level; ++i) {
    os << "  ";
  }
}

void print_bplus_tree_recursive(std::ostream &os, const Node *node, int level = 0) {
  if (!node) {
    return;
  }

  print_indent(os, level);
  os << "Node [" << (node->is_leaf ? "Leaf" : "Internal") << "]: [";

  // Print keys
  for (std::size_t i = 0; i < M; ++i) {
    if (i < node->size && node->keys[i].has_value()) {
      os << node->keys[i].value();
    } else {
      os << "x";
    }
    if (i < M - 1)
      os << ", ";
  }
  os << "], [";

  // Print children pointers representation
  if (!node->is_leaf) {
    for (std::size_t i = 0; i <= M; ++i) {
      if (i <= node->size && node->children[i].has_value()) {
        os << "Node*";
      } else {
        os << "x";
      }
      if (i < M)
        os << ", ";
    }
  }
  os << "] (size: " << node->size << ")\n";

  // For leaf nodes, print the actual orders
  if (node->is_leaf) {
    for (std::size_t i = 0; i < node->size; ++i) {
      print_indent(os, level + 1);
      os << "Order " << i << ": ";
      if (auto order = std::get<Order *>(node->children[i].value())) {
        os << "Key=" << order->key << ", Price=" << order->price << ", Quantity=" << order->quantity << "\n";
      }
    }
  }
  // Recursively print children for internal nodes
  else {
    for (std::size_t i = 0; i <= node->size; ++i) {
      if (node->children[i].has_value()) {
        if (auto child = std::get<Node *>(node->children[i].value())) {
          print_bplus_tree_recursive(os, child, level + 1);
        }
      }
    }
  }
}

// Main print function that can be called on the tree
void print_bplus_tree(const Node *root, std::ostream &os = std::cout) {
  if (!root) {
    os << "Empty tree\n";
    return;
  }
  os << "B+ Tree Structure:\n";
  print_bplus_tree_recursive(os, root);
}