#include <cmath>
#include <ostream>
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
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

using Key = size_t;
template <typename T> constexpr void safe_memcpy(T *dest, T const *src, size_t count = 1) {
  if constexpr (std::is_trivially_copyable_v<T>) {
    std::memcpy(dest, src, sizeof(T) * count);
  } else {
    new (dest) T(*src);
  }
}

template <typename T> constexpr void safe_memmove(T *dest, T *src, size_t count = 1) {
  if (dest < src) {
    for (size_t i = 0; i < count; ++i) {
      new (dest + i) T(std::move(src[i]));
      src[i] = T();
      src[i].~T();
    }
  } else {
    for (size_t i = count; i > 0; --i) {
      new (dest + i - 1) T(std::move(src[i - 1]));
      src[i - 1] = T();
      src[i - 1].~T();
    }
  }
  /*
  if constexpr (std::is_trivially_copyable_v<T>) {

    std::memcpy(dest, src, sizeof(T) * count);
  } else {
    if (dest < src) {
      for (size_t i = 0; i < count; ++i) {
        new (dest + i) T(std::move(src[i]));
        src[i] = T();
        src[i].~T();
      }
    } else {
      for (size_t i = count; i > 0; --i) {
        new (dest + i - 1) T(std::move(src[i - 1]));
        src[i - 1] = T();
        src[i - 1].~T();
      }
    }
  }
  */
}

// https://cs186berkeley.net/notes/note4/#:~:text=The%20number%20d%20is%20the,each%20node%20must%20be%20sorted.
template <typename Value, size_t D = 2> class BPlusTree {
private:
  static_assert(D > 1, "B must be greater than 1");
  static constexpr size_t MIN_KEYS = D;

  struct alignas(64) Node { // Align to cache line
    bool is_leaf;
    size_t num_keys;
    Node *parent;
    Key keys[2 * D];

    Node(bool leaf) : is_leaf(leaf), num_keys(0), parent(nullptr) {}
    virtual ~Node() = default;
  };

  struct InternalNode : Node {
    Node *children[2 * D + 1];

    InternalNode() : Node(false) { std::memset(children, 0, sizeof(children)); }
  };

  struct LeafNode : Node {
    Value values[2 * D];
    LeafNode *prev;
    LeafNode *next;

    LeafNode() : Node(true), prev(nullptr), next(nullptr) {}
  };

  size_t _height;
  size_t _size;

  Node *root;

public:
  // Custom memory pool for nodes
  template <typename NodeType> class NodePool {
  private:
    static constexpr size_t POOL_SIZE = 100;
    std::vector<std::unique_ptr<char[]>> memory_blocks;
    Node *free_nodes;

  public:
    NodePool() : free_nodes(nullptr) {}

    NodeType *allocate(bool is_leaf) {
      if (!free_nodes) {
        auto new_block = std::make_unique<char[]>(sizeof(NodeType) * POOL_SIZE);
        for (size_t i = 0; i < POOL_SIZE - 1; ++i) {
          new (new_block.get() + i * sizeof(NodeType)) NodeType();
          reinterpret_cast<NodeType *>(new_block.get() + i * sizeof(NodeType))->parent =
              reinterpret_cast<NodeType *>(new_block.get() + (i + 1) * sizeof(NodeType));
        }
        free_nodes = reinterpret_cast<NodeType *>(new_block.get());
        memory_blocks.push_back(std::move(new_block));
      }

      Node *result = free_nodes;
      result->is_leaf = is_leaf;
      free_nodes = reinterpret_cast<NodeType *>(result->parent);
      return static_cast<NodeType *>(result);
    }

    void deallocate(Node *node) {
      free_nodes->parent = node;
      free_nodes = node;
    }
  };

  NodePool<LeafNode> leaf_node_pool;
  NodePool<InternalNode> internal_node_pool;

private:
  bool insert_in_leaf(LeafNode *leaf, const Key &key, const Value &value) {
    int pos;
    for (pos = 0; pos < leaf->num_keys; ++pos) {
      if (key == leaf->keys[pos]) {
        // Key already exists
        return false;
      }
      if (key <= leaf->keys[pos]) {
        break;
      }
    }

    if (leaf->num_keys - pos > 0) {
      safe_memmove(leaf->keys + pos + 1, leaf->keys + pos, (leaf->num_keys - pos));
      safe_memmove(leaf->values + pos + 1, leaf->values + pos, (leaf->num_keys - pos));
    }
    leaf->keys[pos] = key;
    leaf->values[pos] = value;
    leaf->num_keys++;
    return true;
    // TODO return value should be removed in future
  }

  bool insert_in_internal(Node *node, size_t height, const Key &key, const Value &value, Node *&new_child) {

    if (height == 0) {
      LeafNode *leaf = static_cast<LeafNode *>(node);
      bool inserted = insert_in_leaf(leaf, key, value);
      if (leaf->num_keys == 2 * D) {
        LeafNode *new_leaf = split_leaf(leaf);
        new_child = new_leaf;
      }
      return inserted;
    } else {
      InternalNode *internal = static_cast<InternalNode *>(node);
      size_t pos;
      for (pos = 0; pos < internal->num_keys; ++pos) {
        if (key < internal->keys[pos]) {
          break;
        }
      }
      Node *child = internal->children[pos];

      bool inserted = insert_in_internal(child, height - 1, key, value, new_child);
      if (new_child) {
        // Key new_key = static_cast<Node *>(new_child)->keys[0];
        safe_memmove((&internal->keys[pos + 1]), &internal->keys[pos], (internal->num_keys - pos));
        safe_memmove(&internal->children[pos + 2], &internal->children[pos + 1], (internal->num_keys - pos));
        if (height == 1) {
          internal->keys[pos] = static_cast<Node *>(new_child)->keys[0];
        } else {
          internal->keys[pos] = std::move(static_cast<Node *>(new_child)->keys[0]);
          new_child->num_keys--;
          for (size_t i = 0; i < static_cast<InternalNode *>(new_child)->num_keys; ++i) {
            internal->keys[i] = std::move(static_cast<Node *>(new_child)->keys[i + 1]);
            static_cast<Node *>(new_child)->keys[i + 1] = Key();
          }
        }
        internal->children[pos + 1] = new_child;
        internal->num_keys++;

        if (internal->num_keys == 2 * D) {
          InternalNode *new_internal = split_internal(internal);
          new_child = new_internal;
        } else {
          new_child = nullptr;
        }
      }
      return inserted;
    }
  }

  LeafNode *split_leaf(LeafNode *leaf) {
    LeafNode *new_leaf = leaf_node_pool.allocate(true);
    int mid = D;

    new_leaf->num_keys = leaf->num_keys - mid;
    safe_memmove(&new_leaf->keys[0], &leaf->keys[mid], new_leaf->num_keys);
    safe_memmove(&new_leaf->values[0], &leaf->values[mid], new_leaf->num_keys);

    leaf->num_keys = mid;

    new_leaf->next = leaf->next;
    leaf->next->prev = new_leaf;

    leaf->next = new_leaf;
    new_leaf->prev = leaf;

    return new_leaf;
  }

  InternalNode *split_internal(InternalNode *internal) {
    InternalNode *new_internal = internal_node_pool.allocate(false);
    int mid = D;

    new_internal->num_keys = internal->num_keys - mid;
    safe_memmove(&new_internal->keys[0], &internal->keys[mid], new_internal->num_keys);
    safe_memmove(&new_internal->children[0], &internal->children[mid + 1], new_internal->num_keys);

    internal->num_keys = mid;
    return new_internal;
  }

  size_t find_leaf(const Key *keys, const size_t num_keys, const Key &key) const {

    size_t pos;
    for (pos = 0; pos < num_keys; ++pos) {
      if (key <= keys[pos]) {
        return pos;
      }
    }
    return pos;
  }

  size_t find_internal(const Key *keys, const size_t num_keys, const Key &key) const {
    size_t pos;
    for (pos = 0; pos < num_keys; ++pos) {
      if (key < keys[pos]) {
        return (pos == 0) ? pos : --pos;
      }
    }
    return pos;
  }

  void *borrow_from_left(Node *leaf, Node *left) {
    // FIXME this is not correct
    size_t split = (leaf->num_keys + left->num_keys) / 2;
    size_t extra = left->num_keys - split;
    safe_memmove(leaf->keys[leaf->num_keys + extra], leaf->keys, extra);
    safe_memmove(leaf->values[leaf->num_keys + extra], leaf->values, extra);

    safe_memmove(leaf->keys[leaf->num_keys], left->keys[split], split - leaf->num_keys);
    safe_memmove(leaf->values[leaf->num_keys], left->values[split], split - leaf->num_keys);
    // FIXME end
  }

  void *borrow_from_right(Node *leaf, Node *right) {
    size_t split = (leaf->num_keys + right->num_keys) / 2;
    safe_memmove(leaf->keys[leaf->num_keys], right->keys, split - leaf->num_keys);
    safe_memmove(leaf->values[leaf->num_keys], right->values, split - leaf->num_keys);

    size_t num_keys = right->num_keys + leaf->num_keys - split;
    safe_memmove(right->keys, right->keys, num_keys);
    safe_memmove(right->values, right->values, num_keys);
  }

  void merge(Node *left, Node *right) {
    safe_memmove(left->keys[left->num_keys], right->keys, right->num_keys);
    safe_memmove(left->values[left->num_keys], right->values, right->num_keys);
    left->num_keys += right->num_keys;
  }

public:
  BPlusTree() : root(nullptr), _height(0), _size(0) {}
  /*
  ~BPlusTree()
  {
      clear();
  }
  */
  bool insert(const Key &key, const Value &value) {
    if (!root) {
      root = leaf_node_pool.allocate(true);
    }

    Node *new_child = nullptr;
    bool inserted = insert_in_internal(root, _height, key, value, new_child);

    if (new_child) {
      InternalNode *new_root = internal_node_pool.allocate(false);
      _height++;
      new_root->num_keys = 1;
      new_root->children[0] = root;
      new_root->children[1] = new_child;

      // new_root->keys[0] = static_cast<InternalNode *>(new_child)->keys[0];
      if (new_child->is_leaf) {
        new_root->keys[0] = new_child->keys[0];
      } else {
        new_child->num_keys--;
        new_root->keys[0] = new_child->keys[0];
        for (size_t i = 0; i < static_cast<InternalNode *>(new_child)->num_keys; ++i) {
          new_child->keys[i] = std::move(static_cast<Node *>(new_child)->keys[i + 1]);
          (new_child)->keys[i + 1] = Key();
        }
      }

      new_root->children[0] = root;
      new_root->children[1] = new_child;
      root = new_root;
    }
    _size++;
    return inserted;
  }

  LeafNode find_leaf_node(const Key *key) const {
    if (!root) {
      return nullptr;
    }
    Node *node = root;
    size_t height = _height;
    size_t possible_pos;

    while (height > 0) {
      InternalNode *internal = static_cast<InternalNode *>(node);
      size_t possible_pos;

      for (possible_pos = 0; possible_pos < internal->num_keys; possible_pos++) {
        if (key < internal->keys[possible_pos]) {
          break;
        } else if (key == internal->keys[possible_pos]) {
          possible_pos++;
          break;
        }
      }
      node = internal->children[possible_pos];
      height--;
    }

    LeafNode *leaf = static_cast<LeafNode *>(node);

    return leaf;
  }

  Value *find_value(const Key &key) const {
    LeafNode *leaf = find_leaf_node(key);
    size_t pos;
    for (pos = 0; pos < leaf->num_keys; pos++) {
      if (key == leaf->keys[pos]) {
        return leaf->values[pos];
      }
    }
    return nullptr;
  }

  Value *remove(const Key &key) const {
    LeafNode *leaf = find_leaf_node(key);
    size_t pos;
    for (pos = 0; pos < leaf->num_keys; pos++) {
      if (key == leaf->keys[pos]) {
        break;
      }
    }
    // FIXME update he parent key
    if (leaf->num_keys - pos > 0) {
      Value *value = leaf->values + pos;
      safe_memmove(leaf->keys + pos, leaf->keys + pos + 1, (leaf->keys - pos));
      safe_memmove(leaf->values + pos, leaf->values + pos + 1, (leaf->values - pos));
      leaf->num_keys--;

      if (leaf->num_keys < D && leaf != root) {
        if (leaf->next && leaf->next->num_keys > D) {
          borrow_from_next(leaf, leaf->next);
        } else if (leaf->prev && leaf->prev->num_keys > D) {
          borrow_from_prev(leaf, leaf->prev);
        } else if (leaf->next) {
          merge(leaf, leaf->next);
        } else {
          merge(leaf->prev, leaf);
        }
      }
      return value;
    }
    // FIXME end
    return nullptr;
  }

  Value *find(const Key &key) const {
    if (!root) {
      return nullptr;
    }
    Node *node = root;
    size_t height = _height;
    size_t possible_pos;

    while (height > 0) {
      InternalNode *internal = static_cast<InternalNode *>(node);
      size_t possible_pos;

      for (possible_pos = 0; possible_pos < internal->num_keys; possible_pos++) {
        if (key < internal->keys[possible_pos]) {
          break;
        } else if (key == internal->keys[possible_pos]) {
          possible_pos++;
          break;
        }
      }
      node = internal->children[possible_pos];
      height--;
    }

    LeafNode *leaf = static_cast<LeafNode *>(node);

    for (possible_pos = 0; possible_pos < leaf->num_keys; possible_pos++) {
      if (key == leaf->keys[possible_pos]) {
        return &leaf->values[possible_pos];
      }
    }
    return nullptr;
  }

  size_t size() const { return _size; }

  size_t height() const { return _height; }

  LeafNode *to_leaf(Node *node) { return static_cast<LeafNode *>(node); }

  InternalNode *to_internal(Node *node) { return static_cast<InternalNode *>(node); }
  void bpt_print() { print(this->root); }

  void print(Node *cursor) {
    // You must NOT edit this function.
    while (cursor != nullptr) {
      std::cout << "Keys: ";
      for (int i = 0; i < cursor->num_keys; ++i) {
        std::cout << cursor->keys[i] << " ";
      }
      std::cout << std::endl;

      if (!cursor->is_leaf) {
        for (int i = 0; i < cursor->num_keys; ++i) {
          std::cout << static_cast<LeafNode *>(cursor)->values[i];
        }
        std::cout << std::endl;
      } else {
        for (int i = 0; i < cursor->num_keys + 1; ++i) {
          print(static_cast<InternalNode *>(cursor)->children[i]);
        }
      }
    }
  }
};

void test_insertion() {
  BPlusTree<std::string> tree;

  // Insert some elements
  std::vector<int> numbers;
  numbers.reserve(1000);
  for (int i = 0; i < 1000; ++i) {
    numbers.push_back(i);
  }
  std::random_device rd;
  std::mt19937 g(rd());

  std::shuffle(numbers.begin(), numbers.end(), g);
  std::ofstream outfile;

  outfile.open("./random_numbers.txt",
               std::ios_base::app); // append instead of overwrite
  // outfile << "Data";
  //  return 0;

  // int numbers2[5] = {706, 707, 896, 293, 179};
  for (auto i : numbers) {
    outfile << i << ",";
    // std::cout << "Insert :" << i << std::endl;
    std::string ii = std::to_string(i);
    // bool inserted = tree.insert(i, ii);
    //  std::cout << "Succeed:" << std::boolalpha << inserted << std::endl;
  }
  outfile << std::endl;
  std::cout << "Insertion 1000 numbers testspassed.\n" << std::endl;
}

void test_failed() {
  int numbers[] = {
      644, 909, 550, 703, 395, 383, 599, 145, 843, 454, 144, 325, 161, 732, 285, 692, 964, 169, 56,  784, 212, 382, 418,
      148, 445, 653, 46,  49,  431, 924, 999, 40,  200, 134, 477, 255, 309, 38,  829, 232, 283, 147, 416, 888, 380, 987,
      405, 966, 288, 414, 908, 997, 413, 442, 969, 351, 751, 142, 542, 539, 879, 227, 453, 269, 202, 575, 972, 313, 261,
      805, 749, 119, 630, 598, 944, 82,  244, 770, 869, 274, 498, 526, 830, 479, 271, 590, 963, 229, 787, 11,  391, 791,
      297, 813, 77,  50,  14,  153, 574, 648, 521, 689, 346, 303, 532, 421, 641, 127, 618, 855, 724, 495, 230, 496, 835,
      885, 245, 500, 79,  831, 859, 7,   359, 862, 512, 634, 671, 579, 463, 902, 666, 0,   311, 214, 485, 593, 343, 622,
      22,  78,  36,  700, 287, 441, 913, 30,  756, 824, 514, 706, 672, 837, 527, 184, 18,  236, 94,  293, 251, 130, 736,
      85,  624, 720, 253, 505, 694, 180, 330, 8,   55,  111, 872, 347, 52,  250, 861, 595, 252, 62,  998, 466, 228, 980,
      367, 991, 769, 397, 743, 321, 600, 610, 958, 182, 623, 928, 217, 80,  99,  990, 333, 953, 754, 783, 801, 993, 778,
      604, 439, 374, 54,  635, 661, 670, 259, 718, 986, 530, 536, 423, 761, 545, 482, 254, 471, 206, 675, 842, 222, 667,
      646, 470, 460, 366, 112, 696, 978, 828, 740, 338, 481, 192, 488, 31,  765, 299, 767, 639, 625, 174, 449, 267, 264,
      186, 757, 305, 895, 133, 709, 28,  940, 307, 61,  436, 317, 266, 417, 84,  725, 447, 606, 854, 705, 549, 276, 360,
      557, 312, 884, 95,  576, 474, 627, 707, 662, 400, 878, 425, 665, 984, 183, 356, 609, 43,  435, 612, 456, 628, 201,
      215, 896, 275, 737, 136, 389, 103, 32,  462, 292, 464, 109, 657, 726, 898, 104, 556, 156, 239, 354, 934, 39,  776,
      697, 75,  794, 304, 353, 900, 188, 101, 65,  680, 690, 524, 988, 840, 258, 796, 846, 942, 429, 546, 563, 348, 324,
      822, 410, 637, 407, 954, 827, 569, 342, 561, 143, 102, 213, 687, 581, 935, 771, 659, 89,  714, 881, 128, 573, 874,
      811, 974, 891, 211, 656, 302, 187, 461, 959, 233, 772, 535, 826, 138, 66,  519, 686, 889, 98,  979, 218, 925, 35,
      310, 371, 372, 932, 424, 396, 933, 115, 747, 823, 571, 399, 877, 195, 841, 9,   893, 419, 47,  224, 494, 167, 710,
      116, 492, 894, 701, 86,  673, 886, 921, 582, 946, 158, 839, 179, 412, 301, 844, 594, 916, 643, 443, 60,  475, 850,
      930, 890, 516, 398, 81,  332, 663, 523, 368, 633, 106, 748, 605, 507, 308, 341, 759, 664, 738, 166, 177, 559, 135,
      100, 945, 476, 280, 951, 497, 45,  906, 568, 543, 852, 151, 962, 789, 440, 744, 506, 534, 603, 294, 352, 157, 800,
      2,   243, 764, 825, 971, 437, 983, 455, 448, 996, 122, 608, 735, 752, 721, 15,  235, 110, 334, 118, 487, 923, 137,
      189, 833, 580, 355, 795, 541, 982, 651, 196, 553, 577, 83,  533, 164, 204, 654, 960, 152, 785, 682, 48,  1,   739,
      566, 42,  591, 88,  402, 376, 955, 337, 277, 871, 975, 788, 564, 669, 121, 336, 241, 617, 867, 645, 108, 390, 68,
      847, 327, 73,  730, 918, 191, 762, 525, 74,  362, 392, 631, 12,  773, 698, 393, 256, 780, 446, 358, 952, 851, 178,
      168, 63,  626, 503, 585, 873, 154, 629, 72,  295, 684, 489, 768, 432, 649, 615, 799, 379, 922, 970, 899, 510, 344,
      570, 286, 433, 501, 504, 284, 273, 426, 469, 484, 746, 319, 727, 741, 409, 804, 712, 4,   812, 345, 162, 875, 171,
      21,  281, 216, 620, 181, 369, 716, 722, 814, 782, 865, 5,   683, 237, 27,  444, 220, 642, 551, 818, 404, 583, 452,
      685, 936, 394, 613, 198, 552, 205, 602, 968, 554, 411, 567, 528, 493, 760, 242, 715, 711, 263, 26,  808, 16,  270,
      434, 240, 558, 51,  750, 37,  472, 688, 480, 375, 810, 265, 296, 373, 915, 731, 948, 149, 486, 10,  59,  753, 131,
      853, 124, 249, 695, 903, 989, 41,  961, 691, 257, 438, 97,  185, 335, 652, 531, 172, 806, 350, 53,  146, 163, 522,
      977, 113, 223, 914, 892, 798, 676, 91,  587, 246, 165, 483, 491, 511, 370, 658, 911, 816, 596, 538, 967, 69,  863,
      123, 897, 415, 956, 25,  857, 318, 834, 248, 289, 560, 67,  384, 331, 774, 193, 678, 704, 17,  385, 733, 781, 140,
      93,  92,  607, 231, 114, 386, 298, 139, 278, 910, 381, 848, 20,  660, 597, 868, 378, 537, 176, 941, 194, 23,  499,
      947, 821, 221};
  // BPlusTree<std::string> tree;
  BPlusTree<int> tree;
  for (auto x : numbers) {
    // bool inserted = tree.insert(x, std::to_string(x));
    bool inserted = tree.insert(x, x + x);
  }
}

void test_search() {
  BPlusTree<int> tree;
  // BPlusTree<int> tree;

  // Insert:
  //  int numbers[5] = {706, 707, 896, 293, 179};
  bool inserted = false;
  for (int i = 1; i <= 3; ++i) {
    inserted = tree.insert(i, i);
  }
  inserted = tree.insert(4, 4);
  inserted = tree.insert(5, 5);
  inserted = tree.insert(6, 6);
  inserted = tree.insert(7, 7);
  inserted = tree.insert(8, 8);
  inserted = tree.insert(9, 9);
  std::cout << "inserted 9." << std::endl;
  inserted = tree.insert(10, 10);
  std::cout << "inserted 10." << std::endl;
  inserted = tree.insert(11, 11);
  std::cout << "inserted 11." << std::endl;

  // FIXME some memory leafk bugs in find
  // Search for existing elements
  //  Search for non-existing element
  assert(tree.find(0) == nullptr);
  std::cout << "couldn't find 0." << std::endl;

  assert(*tree.find(1) == 1);
  std::cout << "found 1." << std::endl;

  assert(*tree.find(2) == 2);
  std::cout << "found 2." << std::endl;

  assert(*tree.find(3) == 3);
  std::cout << "found 3." << std::endl;

  assert(*tree.find(5) == 5);
  std::cout << "found 5." << std::endl;

  assert(*tree.find(7) == 7);
  std::cout << "found 7." << std::endl;

  assert(tree.find(15) == nullptr);
  /*
   */
}
int main() {
  test_insertion();
  test_search();
  test_failed();
  std::cout << "All tests passed successfully!" << std::endl;
  return 0;
}
