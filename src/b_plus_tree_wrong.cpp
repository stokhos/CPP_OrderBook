#define DEBUG 1

#ifdef DEBUG
#define COUT std::cout
#else
#define COUT                                                                                                           \
  if (false)                                                                                                           \
  std::cout
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

using Key = size_t;
template <typename T> constexpr void safe_memcpy(T *dest, T const *src, size_t count = 1) {
  // std::cout << __LINE__ << " " << __func__ << std::endl;
  if constexpr (std::is_trivially_copyable_v<T>) {
    std::memcpy(dest, src, sizeof(T) * count);
  } else {
    new (dest) T(*src);
  }
}

template <typename T> constexpr void safe_memmove(T *dest, T const *src, size_t count = 1) {
  if constexpr (std::is_trivially_copyable_v<T>) {
    for (size_t i = 0; i < count; ++i) {
      dest[i] = std::move(src[i]);
    }
  } else {
    if (dest < src) {
      for (size_t i = 0; i < count; ++i) {
        new (dest + i) T(std::move(src[i]));
        src[i].~T();
      }
    } else {
      for (size_t i = count; i > 0; --i) {
        new (dest + i - 1) T(std::move(src[i - 1]));
        src[i - 1].~T();
      }
    }
  }
}

template <typename Value, size_t B = 4> class BPlusTree {
private:
  static_assert(B > 2, "B must be greater than 2");
  static constexpr size_t MIN_KEYS = B / 2;

  struct alignas(64) Node { // Align to cache line
    bool is_leaf;
    size_t num_keys;
    Node *parent;
    Key keys[B - 1];

    Node(bool leaf) : is_leaf(leaf), num_keys(0), parent(nullptr) {}
    virtual ~Node() = default;
  };

  struct InternalNode : Node {
    Node *children[B];

    InternalNode() : Node(false) { std::memset(children, 0, sizeof(children)); }
  };

  struct LeafNode : Node {
    Value values[B - 1];
    LeafNode *next;

    LeafNode() : Node(true), next(nullptr) {}
  };

  Node *root;
  size_t _height;
  size_t _size;

  // Custom memory pool for nodes
  class LeafNodePool {
  private:
    static constexpr size_t POOL_SIZE = 100;
    std::vector<std::unique_ptr<char[]>> memory_blocks;
    Node *free_leaf_nodes;

  public:
    LeafNodePool() : free_leaf_nodes(nullptr) {}

    template <typename NodeType> NodeType *allocate() {
      if (!free_leaf_nodes) {
        auto new_block = std::make_unique<char[]>(sizeof(NodeType) * POOL_SIZE);
        //! 确保所有 node 都被正常的通过构造函数初始化
        for (size_t i = 0; i < POOL_SIZE - 1; ++i) {

          new (new_block.get() + i * sizeof(NodeType)) NodeType();
          reinterpret_cast<NodeType *>(new_block.get() + i * sizeof(NodeType))->parent =
              reinterpret_cast<NodeType *>(new_block.get() + (i + 1) * sizeof(NodeType));
        }
        free_leaf_nodes = reinterpret_cast<NodeType *>(new_block.get());
        memory_blocks.push_back(std::move(new_block));
      }

      Node *result = free_leaf_nodes;
      free_leaf_nodes = reinterpret_cast<NodeType *>(result->parent);
      return static_cast<NodeType *>(result);
    }

    void deallocate(Node *node) {
      free_leaf_nodes->parent = node;
      free_leaf_nodes = node;
    }
  };

  LeafNodePool node_pool;

  // Binary search optimized for cache
  int binary_search(const Key *keys, size_t n, const Key &target) const {
    int pos;
    for (pos = 0; pos < n; pos++) {
      if (target < keys[pos]) {
        break;
      } else if (target == keys[pos]) {
        pos++;
      }
    }
    return pos;
  }

  // Insert a key-value pair into a leaf node
  void insert_in_leaf(LeafNode *leaf, const Key &key, const Value &value) {
    // std::cout << __LINE__ << " " << __func__ << ", num keys: " <<
    // leaf->num_keys
    //          << std::endl;
    int pos = binary_search(leaf->keys, leaf->num_keys, key);
    /*
    int pos;
    for (pos = 0; pos < leaf->num_keys; pos++) {
      if (leaf->keys[pos] = key) {
        break;
      }
    }
    */
    // std::cout << __LINE__ << " " << __func__ << ", num keys: " <<
    // leaf->num_keys
    //          << std::endl;

    /*
    1. replace existing value
    2. overflow won't happen.
    */
    if (pos < leaf->num_keys && leaf->keys[pos] == key) {
      leaf->values[pos] = value;
      return;
    }

    /*
    std::cout << __LINE__ << " " << __func__ << ", num keys: " << leaf->num_keys
              << std::endl;
    std::cout << __LINE__ << " value: " << value << " n " << leaf->num_keys
              << " pos " << pos << " n -pos " << leaf->num_keys - pos
              << std::endl;
    std::cout << __LINE__ << " " << __func__ << ", num keys: " << leaf->num_keys
              << std::endl;
    */
    safe_memmove(leaf->keys + pos + 1, leaf->keys + pos, (leaf->num_keys - pos));

    // std::cout << __LINE__ << " value: " << value << " " << leaf->num_keys
    //          << std::endl;
    safe_memmove(leaf->values + pos + 1, leaf->values + pos, (leaf->num_keys - pos));

    // std::cout << __LINE__ << " value: " << value << std::endl;
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
    for (size_t i = 0; i < new_leaf->num_keys; ++i) {
      new_leaf->keys[i] = std::move(leaf->keys[i + mid]);
      new_leaf->values[i] = std::move(leaf->values[i + mid]);
      // leaf->keys[i + mid] = Key();
    }

    // safe_memcpy(new_leaf->keys, &leaf->keys[mid], new_leaf->num_keys);
    // safe_memcpy(new_leaf->values, &leaf->values[mid], new_leaf->num_keys);

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
    safe_memmove(new_node->keys, &node->keys[mid + 1], new_node->num_keys);
    safe_memmove(new_node->children, &node->children[mid + 1], (new_node->num_keys + 1));

    node->num_keys = mid;

    return new_node;
  }

  // Insert a key-value pair into the tree
  void insert_in_internal(Node *node, size_t height, const Key &key, const Value &value, Node *&new_child) {
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

      insert_in_internal(child, height - 1, key, value, new_child);

      if (new_child) {
        Key new_key = static_cast<InternalNode *>(new_child)->keys[0];
        safe_memmove(&internal->keys[pos + 1], &internal->keys[pos], (internal->num_keys - pos));
        safe_memmove(&internal->children[pos + 2], &internal->children[pos + 1], (internal->num_keys - pos));
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
    Node *node = root;
    size_t height = _height;

    while (height > 1) {
      InternalNode *internal = static_cast<InternalNode *>(node);
      // for (auto key : internal->keys) {
      //   std::cout << key << " ";
      // }
      // std::cout << internal->num_keys << std::endl;
      size_t pos = binary_search(internal->keys, internal->num_keys, key);

      node = internal->children[pos];
      height--;
    }

    if (height == 1) {
      InternalNode *internal = static_cast<InternalNode *>(node);
      // for (auto key : internal->keys) {
      //   std::cout << key << " ";
      // }
      // std::cout << internal->num_keys << std::endl;
      int pos;
      for (pos = 0; pos < internal->num_keys; pos++) {
        if (key < internal->keys[pos]) {
          break;
        }
      }

      // size_t pos = binary_search(internal->keys, internal->num_keys, key);

      node = internal->children[pos];
      height--;
    }
    return static_cast<LeafNode *>(node);
  }

  // Helper function to find the index of a key in a node
  int find_key_index(const Node *node, const Key &key) const {
    return std::lower_bound(node->keys, node->keys + node->num_keys, key) - node->keys;
  }

  // Helper function to merge two leaf nodes
  void merge_leaves(LeafNode *left, LeafNode *right) {
    safe_memmove(left->keys + left->num_keys, right->keys, right->num_keys);
    safe_memmove(left->values + left->num_keys, right->values, right->num_keys);
    left->num_keys += right->num_keys;
    left->next = right->next;
    node_pool.deallocate(right);
  }

  // Helper function to merge two internal nodes
  void merge_internal(InternalNode *left, InternalNode *right, const Key &middle_key) {
    left->keys[left->num_keys] = middle_key;
    safe_memmove(left->keys + left->num_keys + 1, right->keys, right->num_keys);
    safe_memmove(left->children + left->num_keys + 1, right->children, (right->num_keys + 1));
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
      safe_memmove(right->keys + move_count, right->keys, right->num_keys);
      safe_memmove(right->values + move_count, right->values, right->num_keys);
      safe_memmove(right->keys, left->keys + new_left_size, move_count);
      safe_memmove(right->values, left->values + new_left_size, move_count);
      right->num_keys += move_count;
      left->num_keys = new_left_size;
    } else {
      // Move keys from right to left
      size_t move_count = new_left_size - left->num_keys;
      safe_memmove(left->keys + left->num_keys, right->keys, move_count);
      safe_memmove(left->values + left->num_keys, right->values, move_count);
      safe_memmove(right->keys, right->keys + move_count, (right->num_keys - move_count));
      safe_memmove(right->values, right->values + move_count, (right->num_keys - move_count));
      left->num_keys = new_left_size;
      right->num_keys -= move_count;
    }
  }

  // Helper function to redistribute keys between two internal nodes
  void redistribute_internal(InternalNode *left, InternalNode *right, InternalNode *parent, int parent_index) {
    size_t total = left->num_keys + right->num_keys + 1;
    size_t new_left_size = total / 2;

    if (left->num_keys > new_left_size) {
      // Move keys from left to right
      size_t move_count = left->num_keys - new_left_size;
      safe_memmove(right->keys + move_count, right->keys, right->num_keys);
      safe_memmove(right->children + move_count, right->children, (right->num_keys + 1));
      right->keys[move_count - 1] = parent->keys[parent_index];
      parent->keys[parent_index] = left->keys[new_left_size];
      safe_memmove(right->keys, left->keys + new_left_size + 1, (move_count - 1));
      safe_memmove(right->children, left->children + new_left_size + 1, move_count);
      right->num_keys += move_count;
      left->num_keys = new_left_size;
    } else {
      // Move keys from right to left
      size_t move_count = new_left_size - left->num_keys;
      left->keys[left->num_keys] = parent->keys[parent_index];
      safe_memmove(left->keys + left->num_keys + 1, right->keys, (move_count - 1));
      safe_memmove(left->children + left->num_keys + 1, right->children, move_count);
      parent->keys[parent_index] = right->keys[move_count - 1];
      safe_memmove(right->keys, right->keys + move_count, (right->num_keys - move_count));
      safe_memmove(right->children, right->children + move_count, (right->num_keys - move_count + 1));
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
        safe_memmove(leaf->keys + index, leaf->keys + index + 1, (leaf->num_keys - index - 1));
        safe_memmove(leaf->values + index, leaf->values + index + 1, (leaf->num_keys - index - 1));
        leaf->num_keys--;
        _size--;
        return leaf->num_keys < MIN_KEYS;
      }
      return false; // Key not found
    } else {
      InternalNode *internal = static_cast<InternalNode *>(node);
      int index = find_key_index(internal, key);
      bool need_rebalance = delete_internal(internal->children[index], key, depth + 1);

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
          redistribute_leaves(static_cast<LeafNode *>(left_sibling), static_cast<LeafNode *>(child));
        } else {
          redistribute_internal(static_cast<InternalNode *>(left_sibling), static_cast<InternalNode *>(child), parent,
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
          redistribute_leaves(static_cast<LeafNode *>(child), static_cast<LeafNode *>(right_sibling));
        } else {
          redistribute_internal(static_cast<InternalNode *>(child), static_cast<InternalNode *>(right_sibling), parent,
                                child_index);
        }
        return false;
      }
    }

    // Merge with a sibling
    if (child_index > 0) {
      // Merge with left sibling
      if (child->is_leaf) {
        merge_leaves(static_cast<LeafNode *>(parent->children[child_index - 1]), static_cast<LeafNode *>(child));
      } else {
        merge_internal(static_cast<InternalNode *>(parent->children[child_index - 1]),
                       static_cast<InternalNode *>(child), parent->keys[child_index - 1]);
      }
      safe_memmove(parent->keys + child_index - 1, parent->keys + child_index, (parent->num_keys - child_index));
      safe_memmove(parent->children + child_index, parent->children + child_index + 1,
                   (parent->num_keys - child_index));
      parent->num_keys--;
    } else {
      // Merge with right sibling
      if (child->is_leaf) {
        merge_leaves(static_cast<LeafNode *>(child), static_cast<LeafNode *>(parent->children[child_index + 1]));
      } else {
        merge_internal(static_cast<InternalNode *>(child),
                       static_cast<InternalNode *>(parent->children[child_index + 1]), parent->keys[child_index]);
      }
      safe_memmove(parent->keys + child_index, parent->keys + child_index + 1, (parent->num_keys - child_index - 1));
      safe_memmove(parent->children + child_index + 1, parent->children + child_index + 2,
                   (parent->num_keys - child_index - 1));
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
  BPlusTree() : root(nullptr), _height(0), _size(0) {}
  /*
  ~BPlusTree()
  {
      clear();
  }
  */
  void insert(const Key &key, const Value &value) {
    if (!root) {
      root = node_pool.template allocate<LeafNode>();
      static_cast<LeafNode *>(root)->is_leaf = true;
    }

    Node *new_child = nullptr;
    insert_in_internal(root, _height, key, value, new_child);

    if (new_child) {
      InternalNode *new_root = node_pool.template allocate<InternalNode>();
      new_root->is_leaf = false;
      new_root->num_keys = 1;
      new_root->keys[0] = static_cast<InternalNode *>(new_child)->keys[0];
      new_root->children[0] = root;
      new_root->children[1] = new_child;
      root = new_root;
      _height++;
    }

    _size++;
  }

  Value *find(const Key &key) const {
    if (!root)
      return nullptr;

    LeafNode *leaf = find_leaf(key);

    size_t pos = binary_search(leaf->keys, leaf->num_keys, key);

    // size_t pos;
    for (size_t pos = 0; pos < leaf->num_keys; pos++) {
      if (leaf->keys[pos] == key) {
        return &leaf->values[pos];
        // break;
      }
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
    std::vector<Node *> current_level = std::vector<Node *>(leaves.begin(), leaves.end());
    _height = 0;

    while (current_level.size() > 1) {
      std::vector<InternalNode *> next_level;
      InternalNode *current_internal = node_pool.template allocate<InternalNode>();
      next_level.push_back(current_internal);

      for (Node *child : current_level) {
        if (current_internal->num_keys == B - 1) {
          current_internal = node_pool.template allocate<InternalNode>();
          next_level.push_back(current_internal);
        }
        current_internal->children[current_internal->num_keys] = child;
        if (current_internal->num_keys > 0) {
          current_internal->keys[current_internal->num_keys - 1] = static_cast<InternalNode *>(child)->keys[0];
        }
        current_internal->num_keys++;
      }

      current_level = std::vector<Node *>(next_level.begin(), next_level.end());
      _height++;
    }

    root = current_level[0];
    _size = data.size();
  }

  bool remove(const Key &key) {
    if (!root)
      return false;

    bool need_rebalance = delete_internal(root, key, 0);

    if (need_rebalance && root->num_keys == 0) {
      if (root->is_leaf) {
        node_pool.deallocate(root);
        root = nullptr;
        _height = 0;
      } else {
        Node *new_root = static_cast<InternalNode *>(root)->children[0];
        node_pool.deallocate(root);
        root = new_root;
        _height--;
      }
    }

    return true;
  }
  void clear() {
    if (!root)
      return;

    std::vector<Node *> nodes_to_delete;
    collect_nodes(root, nodes_to_delete);

    for (Node *node : nodes_to_delete) {
      node_pool.deallocate(node);
    }

    root = nullptr;
    _height = 0;
    _size = 0;
    // Implementation omitted for brevity
    // You would need to implement a recursive delete function to properly clear
    // the tree
  }
  void merge(BPlusTree &other) {
    if (!other.root)
      return; // Nothing to merge

    if (!root) {
      // If this tree is empty, just move the other tree
      root = other.root;
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

    collect_data(root, merged_data);
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

void test_insertion() {
  BPlusTree<std::string> tree;

  // Insert some elements

  for (int i = 1; i <= 100; i++) {
    if (i == 4) {
      continue;
    }
    tree.insert(i, std::to_string(i));
  }
  std::cout << "Insertion tests passed." << std::endl;

  // Search for existing elements
  assert(tree.find(1) == nullptr);
  std::cout << "couldn't find 1." << std::endl;
  assert(*tree.find(1) == "one");
  std::cout << "found 1." << std::endl;
  assert(*tree.find(2) == "two");
  std::cout << "found 2." << std::endl;
  assert(*tree.find(3) == "three");

  std::cout << "found 3." << std::endl;

  // std::cout << "Insertion tests passed." << std::endl;
  //  Search for non-existing element
  assert(tree.find(4) == nullptr);

  assert(*tree.find(5) == "three");
  std::cout << "found 5." << std::endl;

  assert(*tree.find(7) == "three");
  std::cout << "found 7." << std::endl;

  // Update an existing element
  // Fixme
  // tree.insert(2, "TWO");
  // assert(*tree.find(2) == "TWO");

  std::cout << "Insertion and search tests passed." << std::endl;
}

void test_deletion() {
  BPlusTree<std::string> tree;

  // Insert elements
  for (int i = 1; i <= 40; i++) {
    tree.insert(i, std::to_string(i));
  }

  // Delete some elements
  assert(tree.remove(5));
  assert(tree.remove(8));

  // Check if deleted elements are gone
  assert(tree.find(5) == nullptr);
  assert(tree.find(8) == nullptr);

  // Try to delete non-existing element
  // FIXME: This should return false
  assert(tree.remove(50));

  // Check if other elements still exist
  assert(*tree.find(1) == "1");
  assert(*tree.find(10) == "10");

  std::cout << "Deletion tests passed." << std::endl;
}

void test_bulk_load() {
  std::vector<std::pair<size_t, std::string>> data = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}, {5, "five"}};

  BPlusTree<std::string> tree;
  tree.bulk_load(data);

  assert(tree.size() == 5);
  assert(*tree.find(1) == "one");
  assert(*tree.find(5) == "five");
  assert(tree.find(6) == nullptr);

  std::cout << "Bulk load tests passed." << std::endl;
}

void test_merge() {
  BPlusTree<std::string> tree1;
  BPlusTree<std::string> tree2;

  // Populate tree1
  tree1.insert(1, "one");
  tree1.insert(3, "three");
  tree1.insert(5, "five");

  // Populate tree2
  tree2.insert(2, "two");
  tree2.insert(4, "four");
  tree2.insert(6, "six");

  // Merge tree2 into tree1
  tree1.merge(tree2);

  assert(tree1.size() == 6);
  assert(*tree1.find(1) == "one");
  assert(*tree1.find(2) == "two");
  assert(*tree1.find(6) == "six");

  // Check if tree2 is empty after merge
  assert(tree2.size() == 0);
  assert(tree2.find(2) == nullptr);

  std::cout << "Merge tests passed." << std::endl;
}

void test_large_dataset() {
  BPlusTree<int> tree;
  std::vector<int> numbers;

  // Generate 10,000 unique random numbers
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 1000000);

  for (int i = 0; i < 10000; ++i) {
    std::cout << i << std::endl;
    int num = dis(gen);
    numbers.push_back(num);
    tree.insert(num, i);
  }

  // Check if all numbers are in the tree
  for (int num : numbers) {
    assert(tree.find(num) != nullptr);
  }

  // Remove half of the numbers
  for (int i = 0; i < 5000; ++i) {
    tree.remove(numbers[i]);
  }

  // Check if removed numbers are gone and others still exist
  for (int i = 0; i < 10000; ++i) {
    if (i < 5000) {
      assert(tree.find(numbers[i]) == nullptr);
    } else {
      assert(tree.find(numbers[i]) != nullptr);
    }
  }

  std::cout << "Large dataset tests passed." << std::endl;
}

int main() {
  test_insertion();
  test_deletion();
  test_bulk_load();
  test_large_dataset();
  test_merge();

  std::cout << "All tests passed successfully!" << std::endl;
  return 0;
}

/*
int main() {
  BPlusTree<std::string> tree;

  // Insert some elements
  tree.insert(1, "one");
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
  if (value) {
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

  // Bulk load
  std::vector<std::pair<Key, std::string>> data = {
      {4, "four"}, {5, "five"}, {6, "six"}, {7, "seven"}, {8, "eight"}};
  BPlusTree<std::string> bulk_tree;
  bulk_tree.bulk_load(data);

  std::cout << "Bulk Tree size: " << bulk_tree.size() << std::endl;
  std::cout << "Bulk Tree height: " << bulk_tree.height() << std::endl;

  std::cout << "Bulk Tree size before clearing: " << bulk_tree.size()
            << std::endl;

  bulk_tree.clear();

  std::cout << "Bulk Tree size after clearing: " << bulk_tree.size()
            << std::endl;
  return 0;
}
*/
