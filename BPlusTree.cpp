#include <cstring>
#include <memory>
#include <string>
#include <vector>
// #include <iostream>
#include <algorithm>
#include <cassert>
#include <cstddef>

using Key = size_t;

template <typename Value, size_t B = 64> class BPlusTree {
private:
  static_assert(B > 2, "B must be greater than 2");
  static constexpr size_t MIN_KEYS = B / 2;

  struct alignas(64) Node { // Align to cache line
    bool is_leaf;
    size_t num_keys;
    Key keys[B - 1];

    Node(bool leaf) : is_leaf(leaf), num_keys(0) {}
    virtual ~Node() = default;
  };

  struct alignas(64) InternalNode : Node {
    Node *children[B];

    InternalNode() : Node(false) { std::memset(children, 0, sizeof(children)); }
  };

  struct alignas(64) LeafNode : Node {
    Value values[B - 1];
    LeafNode *next;

    LeafNode() : Node(true), next(nullptr) {}
  };

  Node *_root;
  size_t _height;
  size_t _size;

  // Custom memory pool for nodes
  class LeafNodePool {
  private:
    static constexpr size_t POOL_SIZE = 1024;
    std::vector<std::unique_ptr<char[]>> memory_blocks;
    std::vector<Node *> free_leaf_nodes;

    template <typename NodeType> void allocate_block() {
      auto block = std::make_unique<char[]>(POOL_SIZE * sizeof(NodeType));
      NodeType *block_start = reinterpret_cast<NodeType *>(block.get());

      for (std::size_t i = 0; i < POOL_SIZE; ++i) {
        free_leaf_nodes.push_back(reinterpret_cast<Node *>(block_start + i));
      }
      memory_blocks.push_back(std::move(block));
    }

  public:
    LeafNodePool() = default;

    template <typename NodeType> NodeType *allocate() {
      if (free_leaf_nodes.empty()) {
        allocate_block<NodeType>();
      }
      NodeType *node = static_cast<NodeType *>(free_leaf_nodes.back());
      free_leaf_nodes.pop_back();
      return node;
    }

    void deallocate(Node *node) { free_leaf_nodes.push_back(node); }
  };

  LeafNodePool node_pool;

  // Binary search optimized for cache
  int binary_search(const Key *keys, size_t n, const Key &target) const {
    int left = 0, right = n;
    while (left < right) {
      int mid = left + (right - left) / 2;
      if (keys[mid] < target) {
        left = mid + 1;
      } else {
        right = mid;
      }
    }
    return left;
  }

  // Insert a key-value pair into a leaf node
  void insert_in_leaf(LeafNode *leaf, const Key &key, const Value &value) {
    int pos = binary_search(leaf->keys, leaf->num_keys, key);

    /*
    1. replace existing value
    2. overflow won't happen.
    */
    if (pos < leaf->num_keys && leaf->keys[pos] == key) {
      leaf->values[pos] = value;
      return;
    }

    std::memmove(leaf->keys + pos + 1, leaf->keys + pos,
                 sizeof(Key) * (leaf->num_keys - pos));
    std::memmove(leaf->values + pos + 1, leaf->values + pos,
                 sizeof(Value) * (leaf->num_keys - pos));

    leaf->keys[pos] = key;
    leaf->values[pos] = value;
    leaf->num_keys++;
  }

  // Split a leaf node
  LeafNode *split_leaf(LeafNode *leaf) {
    LeafNode *new_leaf = node_pool.template allocate<LeafNode>();
    new_leaf->is_leaf = true;
    size_t mid = B / 2;

    new_leaf->num_keys = leaf->num_keys - mid;
    std::memcpy(new_leaf->keys, &leaf->keys[mid],
                sizeof(Key) * new_leaf->num_keys);
    std::memcpy(new_leaf->values, &leaf->values[mid],
                sizeof(Value) * new_leaf->num_keys);

    leaf->num_keys = mid;
    new_leaf->next = leaf->next;
    leaf->next = new_leaf;

    return new_leaf;
  }

  // Split an internal node
  InternalNode *split_internal(InternalNode *node) {
    InternalNode *new_node = node_pool.template allocate<InternalNode>();
    new_node->is_leaf = false;
    size_t mid = B / 2;

    new_node->num_keys = node->num_keys - mid - 1;
    std::memcpy(new_node->keys, &node->keys[mid + 1],
                sizeof(Key) * new_node->num_keys);
    std::memcpy(new_node->children, &node->children[mid + 1],
                sizeof(Node *) * (new_node->num_keys + 1));

    node->num_keys = mid;

    return new_node;
  }

  // Insert a key-value pair into the tree
  void insert_internal(Node *node, size_t height, const Key &key,
                       const Value &value, Node *&new_child) {
    if (height == 0) {
      LeafNode *leaf = static_cast<LeafNode *>(node);
      insert_in_leaf(leaf, key, value);

      if (leaf->num_keys == B - 1) {
        LeafNode *new_leaf = split_leaf(leaf);
        new_child = new_leaf;
      }
    } else {
      InternalNode *internal = static_cast<InternalNode *>(node);
      size_t pos = binary_search(internal->keys, internal->num_keys, key);
      Node *child = internal->children[pos];

      insert_internal(child, height - 1, key, value, new_child);

      if (new_child) {
        Key new_key = static_cast<InternalNode *>(new_child)->keys[0];
        std::memmove(&internal->keys[pos + 1], &internal->keys[pos],
                     sizeof(Key) * (internal->num_keys - pos));
        std::memmove(&internal->children[pos + 2], &internal->children[pos + 1],
                     sizeof(Node *) * (internal->num_keys - pos));
        internal->keys[pos] = new_key;
        internal->children[pos + 1] = new_child;
        internal->num_keys++;

        if (internal->num_keys == B - 1) {
          InternalNode *new_internal = split_internal(internal);
          new_child = new_internal;
        } else {
          new_child = nullptr;
        }
      }
    }
  }

  // Find the leaf node containing the key
  LeafNode *find_leaf(const Key &key) const {
    Node *node = _root;
    size_t h = _height;

    while (h > 0) {
      InternalNode *internal = static_cast<InternalNode *>(node);
      size_t pos = binary_search(internal->keys, internal->num_keys, key);
      node = internal->children[pos];
      h--;
    }

    return static_cast<LeafNode *>(node);
  }

  // Helper function to find the index of a key in a node
  int find_key_index(const Node *node, const Key &key) const {
    return std::lower_bound(node->keys, node->keys + node->num_keys, key) -
           node->keys;
  }

  // Helper function to merge two leaf nodes
  void merge_leaves(LeafNode *left, LeafNode *right) {
    std::memcpy(left->keys + left->num_keys, right->keys,
                sizeof(Key) * right->num_keys);
    std::memcpy(left->values + left->num_keys, right->values,
                sizeof(Value) * right->num_keys);
    left->num_keys += right->num_keys;
    left->next = right->next;
    node_pool.deallocate(right);
  }

  // Helper function to merge two internal nodes
  void merge_internal(InternalNode *left, InternalNode *right,
                      const Key &middle_key) {
    left->keys[left->num_keys] = middle_key;
    std::memcpy(left->keys + left->num_keys + 1, right->keys,
                sizeof(Key) * right->num_keys);
    std::memcpy(left->children + left->num_keys + 1, right->children,
                sizeof(Node *) * (right->num_keys + 1));
    left->num_keys += right->num_keys + 1;
    node_pool.deallocate(right);
  }

  // Helper function to redistribute keys between two leaf nodes
  void redistribute_leaves(LeafNode *left, LeafNode *right) {
    size_t total = left->num_keys + right->num_keys;
    size_t new_left_size = total / 2;

    if (left->num_keys > new_left_size) {
      // Move keys from left to right
      size_t move_count = left->num_keys - new_left_size;
      std::memmove(right->keys + move_count, right->keys,
                   sizeof(Key) * right->num_keys);
      std::memmove(right->values + move_count, right->values,
                   sizeof(Value) * right->num_keys);
      std::memcpy(right->keys, left->keys + new_left_size,
                  sizeof(Key) * move_count);
      std::memcpy(right->values, left->values + new_left_size,
                  sizeof(Value) * move_count);
      right->num_keys += move_count;
      left->num_keys = new_left_size;
    } else {
      // Move keys from right to left
      size_t move_count = new_left_size - left->num_keys;
      std::memcpy(left->keys + left->num_keys, right->keys,
                  sizeof(Key) * move_count);
      std::memcpy(left->values + left->num_keys, right->values,
                  sizeof(Value) * move_count);
      std::memmove(right->keys, right->keys + move_count,
                   sizeof(Key) * (right->num_keys - move_count));
      std::memmove(right->values, right->values + move_count,
                   sizeof(Value) * (right->num_keys - move_count));
      left->num_keys = new_left_size;
      right->num_keys -= move_count;
    }
  }

  // Helper function to redistribute keys between two internal nodes
  void redistribute_internal(InternalNode *left, InternalNode *right,
                             InternalNode *parent, int parent_index) {
    size_t total = left->num_keys + right->num_keys + 1;
    size_t new_left_size = total / 2;

    if (left->num_keys > new_left_size) {
      // Move keys from left to right
      size_t move_count = left->num_keys - new_left_size;
      std::memmove(right->keys + move_count, right->keys,
                   sizeof(Key) * right->num_keys);
      std::memmove(right->children + move_count, right->children,
                   sizeof(Node *) * (right->num_keys + 1));
      right->keys[move_count - 1] = parent->keys[parent_index];
      parent->keys[parent_index] = left->keys[new_left_size];
      std::memcpy(right->keys, left->keys + new_left_size + 1,
                  sizeof(Key) * (move_count - 1));
      std::memcpy(right->children, left->children + new_left_size + 1,
                  sizeof(Node *) * move_count);
      right->num_keys += move_count;
      left->num_keys = new_left_size;
    } else {
      // Move keys from right to left
      size_t move_count = new_left_size - left->num_keys;
      left->keys[left->num_keys] = parent->keys[parent_index];
      std::memcpy(left->keys + left->num_keys + 1, right->keys,
                  sizeof(Key) * (move_count - 1));
      std::memcpy(left->children + left->num_keys + 1, right->children,
                  sizeof(Node *) * move_count);
      parent->keys[parent_index] = right->keys[move_count - 1];
      std::memmove(right->keys, right->keys + move_count,
                   sizeof(Key) * (right->num_keys - move_count));
      std::memmove(right->children, right->children + move_count,
                   sizeof(Node *) * (right->num_keys - move_count + 1));
      left->num_keys = new_left_size;
      right->num_keys -= move_count;
    }
  }

  // Recursive delete helper function
  bool delete_internal(Node *node, const Key &key, size_t depth) {
    if (node->is_leaf) {
      LeafNode *leaf = static_cast<LeafNode *>(node);
      int index = find_key_index(leaf, key);
      if (index < leaf->num_keys && leaf->keys[index] == key) {
        std::memmove(leaf->keys + index, leaf->keys + index + 1,
                     sizeof(Key) * (leaf->num_keys - index - 1));
        std::memmove(leaf->values + index, leaf->values + index + 1,
                     sizeof(Value) * (leaf->num_keys - index - 1));
        leaf->num_keys--;
        _size--;
        return leaf->num_keys < MIN_KEYS;
      }
      return false; // Key not found
    } else {
      InternalNode *internal = static_cast<InternalNode *>(node);
      int index = find_key_index(internal, key);
      bool need_rebalance =
          delete_internal(internal->children[index], key, depth + 1);

      if (need_rebalance) {
        return rebalance_internal(internal, index, depth);
      }
      return false;
    }
  }

  // Helper function to rebalance internal nodes after deletion
  bool rebalance_internal(InternalNode *parent, int child_index, size_t depth) {
    Node *child = parent->children[child_index];

    // Try to borrow from left sibling
    if (child_index > 0) {
      Node *left_sibling = parent->children[child_index - 1];
      if (left_sibling->num_keys > MIN_KEYS) {
        if (child->is_leaf) {
          redistribute_leaves(static_cast<LeafNode *>(left_sibling),
                              static_cast<LeafNode *>(child));
        } else {
          redistribute_internal(static_cast<InternalNode *>(left_sibling),
                                static_cast<InternalNode *>(child), parent,
                                child_index - 1);
        }
        return false;
      }
    }

    // Try to borrow from right sibling
    if (child_index < parent->num_keys) {
      Node *right_sibling = parent->children[child_index + 1];
      if (right_sibling->num_keys > MIN_KEYS) {
        if (child->is_leaf) {
          redistribute_leaves(static_cast<LeafNode *>(child),
                              static_cast<LeafNode *>(right_sibling));
        } else {
          redistribute_internal(static_cast<InternalNode *>(child),
                                static_cast<InternalNode *>(right_sibling),
                                parent, child_index);
        }
        return false;
      }
    }

    // Merge with a sibling
    if (child_index > 0) {
      // Merge with left sibling
      if (child->is_leaf) {
        merge_leaves(static_cast<LeafNode *>(parent->children[child_index - 1]),
                     static_cast<LeafNode *>(child));
      } else {
        merge_internal(
            static_cast<InternalNode *>(parent->children[child_index - 1]),
            static_cast<InternalNode *>(child), parent->keys[child_index - 1]);
      }
      std::memmove(parent->keys + child_index - 1, parent->keys + child_index,
                   sizeof(Key) * (parent->num_keys - child_index));
      std::memmove(parent->children + child_index,
                   parent->children + child_index + 1,
                   sizeof(Node *) * (parent->num_keys - child_index));
      parent->num_keys--;
    } else {
      // Merge with right sibling
      if (child->is_leaf) {
        merge_leaves(
            static_cast<LeafNode *>(child),
            static_cast<LeafNode *>(parent->children[child_index + 1]));
      } else {
        merge_internal(
            static_cast<InternalNode *>(child),
            static_cast<InternalNode *>(parent->children[child_index + 1]),
            parent->keys[child_index]);
      }
      std::memmove(parent->keys + child_index, parent->keys + child_index + 1,
                   sizeof(Key) * (parent->num_keys - child_index - 1));
      std::memmove(parent->children + child_index + 1,
                   parent->children + child_index + 2,
                   sizeof(Node *) * (parent->num_keys - child_index - 1));
      parent->num_keys--;
    }

    return parent->num_keys < MIN_KEYS;
  }
  void collect_nodes(Node *node, std::vector<Node *> &nodes) {
    if (!node)
      return;

    nodes.push_back(node);

    if (!node->is_leaf) {
      InternalNode *internal = static_cast<InternalNode *>(node);
      for (size_t i = 0; i <= internal->num_keys; ++i) {
        collect_nodes(internal->children[i], nodes);
      }
    }
  }
  // Helper function to collect all key-value pairs from a tree
  void collect_data(Node *node, std::vector<std::pair<Key, Value>> &data) {
    if (!node)
      return;

    if (node->is_leaf) {
      LeafNode *leaf = static_cast<LeafNode *>(node);
      for (size_t i = 0; i < leaf->num_keys; ++i) {
        data.emplace_back(leaf->keys[i], leaf->values[i]);
      }
    } else {
      InternalNode *internal = static_cast<InternalNode *>(node);
      for (size_t i = 0; i <= internal->num_keys; ++i) {
        collect_data(internal->children[i], data);
      }
    }
  }

public:
  BPlusTree() : _root(nullptr), _height(0), _size(0) {}
  /*
  ~BPlusTree()
  {
      clear();
  }
  */
  void insert(const Key &key, const Value &value) {
    if (!_root) {
      _root = node_pool.template allocate<LeafNode>();
      static_cast<LeafNode *>(_root)->is_leaf = true;
    }

    Node *new_child = nullptr;
    insert_internal(_root, _height, key, value, new_child);

    if (new_child) {
      InternalNode *new_root = node_pool.template allocate<InternalNode>();
      new_root->is_leaf = false;
      new_root->num_keys = 1;
      new_root->keys[0] = static_cast<InternalNode *>(new_child)->keys[0];
      new_root->children[0] = _root;
      new_root->children[1] = new_child;
      _root = new_root;
      _height++;
    }

    _size++;
  }

  Value *find(const Key &key) const {
    if (!_root)
      return nullptr;

    LeafNode *leaf = find_leaf(key);
    size_t pos = binary_search(leaf->keys, leaf->num_keys, key);

    if (pos < leaf->num_keys && leaf->keys[pos] == key) {
      return &leaf->values[pos];
    }

    return nullptr;
  }

  // Bulk load from a sorted vector of key-value pairs
  void bulk_load(const std::vector<std::pair<Key, Value>> &data) {
    clear();

    // Create leaf nodes
    std::vector<LeafNode *> leaves;
    LeafNode *current_leaf = node_pool.template allocate<LeafNode>();
    leaves.push_back(current_leaf);

    for (const auto &item : data) {
      if (current_leaf->num_keys == B - 1) {
        current_leaf = node_pool.template allocate<LeafNode>();
        leaves.push_back(current_leaf);
      }
      current_leaf->keys[current_leaf->num_keys] = item.first;
      current_leaf->values[current_leaf->num_keys] = item.second;
      current_leaf->num_keys++;
    }

    // Link leaf nodes
    for (size_t i = 0; i < leaves.size() - 1; i++) {
      leaves[i]->next = leaves[i + 1];
    }

    // Build internal nodes
    std::vector<Node *> current_level =
        std::vector<Node *>(leaves.begin(), leaves.end());
    _height = 0;

    while (current_level.size() > 1) {
      std::vector<InternalNode *> next_level;
      InternalNode *current_internal =
          node_pool.template allocate<InternalNode>();
      next_level.push_back(current_internal);

      for (Node *child : current_level) {
        if (current_internal->num_keys == B - 1) {
          current_internal = node_pool.template allocate<InternalNode>();
          next_level.push_back(current_internal);
        }
        current_internal->children[current_internal->num_keys] = child;
        if (current_internal->num_keys > 0) {
          current_internal->keys[current_internal->num_keys - 1] =
              static_cast<InternalNode *>(child)->keys[0];
        }
        current_internal->num_keys++;
      }

      current_level = std::vector<Node *>(next_level.begin(), next_level.end());
      _height++;
    }

    _root = current_level[0];
    _size = data.size();
  }

  bool remove(const Key &key) {
    if (!_root)
      return false;

    bool need_rebalance = delete_internal(_root, key, 0);

    if (need_rebalance && _root->num_keys == 0) {
      if (_root->is_leaf) {
        node_pool.deallocate(_root);
        _root = nullptr;
        _height = 0;
      } else {
        Node *new_root = static_cast<InternalNode *>(_root)->children[0];
        node_pool.deallocate(_root);
        _root = new_root;
        _height--;
      }
    }

    return true;
  }
  void clear() {
    if (!_root)
      return;

    std::vector<Node *> nodes_to_delete;
    collect_nodes(_root, nodes_to_delete);

    for (Node *node : nodes_to_delete) {
      node_pool.deallocate(node);
    }

    _root = nullptr;
    _height = 0;
    _size = 0;
    // Implementation omitted for brevity
    // You would need to implement a recursive delete function to properly clear
    // the tree
  }
  void merge(BPlusTree &other) {
    if (!other.root)
      return; // Nothing to merge

    if (!_root) {
      // If this tree is empty, just move the other tree
      _root = other.root;
      _height = other._height;
      _size = other._size;
      other.root = nullptr;
      other._height = 0;
      other._size = 0;
      return;
    }

    // Collect all key-value pairs from both trees
    std::vector<std::pair<Key, Value>> merged_data;
    merged_data.reserve(_size + other._size);

    collect_data(_root, merged_data);
    collect_data(other.root, merged_data);

    // Sort the merged data
    std::sort(merged_data.begin(), merged_data.end());

    // Clear both trees
    clear();
    other.clear();

    // Bulk load the sorted data into this tree
    bulk_load(merged_data);
  }

  size_t size() const { return _size; }

  size_t height() const { return _height; }
};

int main() {
  BPlusTree<int> tree;

  // Insert some elements
  // std::string one = "one";
  tree.insert(1, 2);
  /*
  tree.insert(2, "two");
  tree.insert(3, "three");
  tree.insert(4, "four");
  tree.insert(5, "five");
  tree.insert(6, "six");
  tree.insert(7, "seven");
  tree.insert(8, "eight");

  std::cout << "Tree size before deletion: " << tree.size() << std::endl;
  // Find an element
  std::string *value = tree.find(2);
  if (value)
  {
      std::cout << "Found: " << *value << std::endl;
  }

  tree.remove(3);

  // Check if the element was deleted
  value = tree.find(3);
  if (value) {
      std::cout << "Key 3 is still in the tree: " << *value << std::endl;
  } else {
      std::cout << "Key 3 was successfully deleted" << std::endl;
  }

  std::cout << "Tree size after deletion: " << tree.size() << std::endl;

  std::cout << "Tree size before clearing: " << tree.size() << std::endl;

  tree.clear();

  std::cout << "Tree size after clearing: " << tree.size() << std::endl;
  */

  // Bulk load
  /*
  std::vector<std::pair<int, std::string>> data = {
      {4, "four"}, {5, "five"}, {6, "six"}, {7, "seven"}, {8, "eight"}};
  BPlusTree<int, std::string> bulk_tree;
  bulk_tree.bulk_load(data);

  std::cout << "Bulk Tree size: " << bulk_tree.size() << std::endl;
  std::cout << "Bulk Tree height: " << bulk_tree.height() << std::endl;

  std::cout << "Bulk Tree size before clearing: " << bulk_tree.size() <<
  std::endl;

  bulk_tree.clear();

  std::cout << "Bulk Tree size after clearing: " << bulk_tree.size() <<
  std::endl;
  */
  return 0;
}
