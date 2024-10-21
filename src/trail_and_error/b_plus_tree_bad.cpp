#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

using Key = size_t;

template <typename Value, size_t B = 64> class BPlusTree {
private:
  static_assert(B > 2, "B must be greater than 2");
  static constexpr size_t MIN_KEYS = B / 2;

  struct Node {
    bool is_leaf;
    size_t num_keys;
    Key keys[B - 1];

    Node(bool leaf) : is_leaf(leaf), num_keys(0) {}
  };

  struct InternalNode : Node {
    std::unique_ptr<Node> children[B];

    InternalNode() : Node(false) {}
  };

  struct LeafNode : Node {
    Value values[B - 1];
    std::unique_ptr<LeafNode> next;

    LeafNode() : Node(true) {}
  };

  std::unique_ptr<Node> _root;
  size_t _height;
  size_t _size;

  // Custom memory pool for nodes
  class LeafNodePool {
  private:
    static constexpr size_t POOL_SIZE = 1024;
    std::vector<std::unique_ptr<char[]>> memory_blocks;
    Node *free_leaf_nodes;

  public:
    LeafNodePool() : free_leaf_nodes(nullptr) {}

    template <typename NodeType> NodeType *allocate() {
      if (!free_leaf_nodes) {
        auto new_block = std::make_unique<char[]>(sizeof(NodeType) * POOL_SIZE);
        for (size_t i = 0; i < POOL_SIZE - 1; ++i) {
          reinterpret_cast<Node *>(new_block.get() + i * sizeof(NodeType))
              ->keys[0] = reinterpret_cast<Key>(new_block.get() +
                                                (i + 1) * sizeof(NodeType));
        }
        free_leaf_nodes = reinterpret_cast<Node *>(new_block.get());
        memory_blocks.push_back(std::move(new_block));
      }

      Node *result = free_leaf_nodes;
      free_leaf_nodes = reinterpret_cast<Node *>(free_leaf_nodes->keys[0]);
      // return new (result) NodeType();
      return static_cast<NodeType *>(result);
    }

    void deallocate(Node *node) {
      // node->~Node();
      node->keys[0] = reinterpret_cast<Key>(free_leaf_nodes);
      free_leaf_nodes = node;
    }
  };

  LeafNodePool node_pool;

public:
  BPlusTree() : _root(nullptr), _height(0), _size(0) {}

  ~BPlusTree() {
    std::cout << __func__ << std::endl;
    // clear();
    std::cout << __func__ << std::endl;
  }

  void insert(const Key &key, const Value &value) {
    if (!_root) {
      _root = std::unique_ptr<Node>(node_pool.template allocate<LeafNode>());
      _root->is_leaf = true;
    }

    std::unique_ptr<Node> new_child;
    // insert_internal(root.get(), _height, key, value, new_child);
    //  std::cout<< ""<<std::endl;

    if (new_child) {
      auto new_root = std::unique_ptr<InternalNode>(
          node_pool.template allocate<InternalNode>());
      new_root->num_keys = 1;
      new_root->keys[0] = static_cast<InternalNode *>(new_child.get())->keys[0];
      new_root->children[0] = std::move(_root);
      new_root->children[1] = std::move(new_child);
      _root = std::move(new_root);
      _height++;
    }

    _size++;
  }

  size_t size() const { return _size; }

  size_t height() const { return _height; }
};

int main() {
  BPlusTree<std::string> tree;

  // Insert some elements
  tree.insert(1, "one");
  /*
  tree.insert(2, "two");
  tree.insert(3, "three");
  tree.insert(4, "four");
  tree.insert(5, "five");

  std::cout << "Tree size: " << tree.size() << std::endl;
  std::cout << "Tree height: " << tree.height() << std::endl;

  // Find an element
  std::string* value = tree.find(3);
  if (value) {
      std::cout << "Found: " << *value << std::endl;
  }
  */

  // Clear the tree
  // tree.clear();

  // std::cout << "Tree size after clearing: " << tree.size() << std::endl;

  return 0;
}
