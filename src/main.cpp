#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>

// Assuming the BPlusTree.h is in the same directory
#include "b_plus_tree.h"

void test_insert_and_search() {
  BPlusTree tree;
  print_bplus_tree(tree);
  std::vector<Order> orders = {{5, 100, 10}, {3, 90, 5},  {7, 110, 8}, {1, 80, 3},  {9, 120, 12},
                               {2, 85, 4},   {6, 105, 7}, {4, 95, 6},  {8, 115, 9}, {10, 125, 15}};

  // Insert orders
  for (const auto &order : orders) {
    std::cout << "Inserting order: " << order.key << std::endl;
    tree.insert(new Order(order));
  }
  print_bplus_tree(tree);

  // Search for each order
  for (const auto &order : orders) {
    auto result = tree.search(order.key);
    assert(result.has_value());
    // std::cout << "result key: " << result.value()->key << ", order key:" << order.key << std::endl;
    assert((*result)->key == order.key);
    assert((*result)->price == order.price);
    assert((*result)->quantity == order.quantity);
  }

  // Search for non-existent key
  assert(!tree.search(11).has_value());

  std::cout << "Insert and search test passed!" << std::endl;
}

void test_remove() {
  BPlusTree tree;
  std::vector<size_t> keys = {5, 3, 7, 1, 9, 2, 6, 4, 8, 10};

  // Insert orders
  for (size_t key : keys) {
    // std::cout << "Inserting order: " << key << std::endl;
    tree.insert(new Order{key, key * 10, key});
    // std::cout << tree << std::endl;
  }
  std::cout << std::endl;
  print_bplus_tree(tree);

  // Remove some keys
  std::cout << "Removing 3" << std::endl;
  tree.remove(3);
  std::cout << "After removing 3" << std::endl;
  print_bplus_tree(tree);
  std::cout << "Removing 7" << std::endl;
  tree.remove(7);
  std::cout << "After removing 7" << std::endl;
  print_bplus_tree(tree);
  std::cout << "Removing 1" << std::endl;
  tree.remove(1);
  std::cout << "After removing 1" << std::endl;
  print_bplus_tree(tree);
  //  return;

  // Verify removed keys are not found
  assert(!tree.search(3).has_value());
  assert(!tree.search(7).has_value());
  assert(!tree.search(1).has_value());
  // Verify remaining keys are still present
  for (size_t key : {2, 4, 5, 6, 8, 9, 10}) {
    // std::cout << "Searching " << key << std::endl;
    assert(tree.search(key).has_value());
  }

  std::cout << "Remove test passed!\n" << std::endl;
}

void test_debug_1() {
  std::vector<size_t> keys{9, 2, 17, 18, 7, 14, 1, 16, 15, 3, 5, 10, 4, 19, 12, 6, 11, 8, 13, 20};
  BPlusTree tree;
  for (size_t key : keys) {
    tree.insert(new Order{key, key * 10, key});
  }
  print_bplus_tree(tree, true);

  for (size_t key : keys) {
    std::cout << "Removing key: " << key << std::endl;
    tree.remove(key);
    print_bplus_tree(tree, true);
  }
  std::cout << "Debug 1 passed\n" << std::endl;
}

void test_debug_2() {
  std::vector<size_t> keys{13, 9, 16, 20, 1, 3, 12, 7, 10, 14, 6, 17, 2, 5, 4, 19, 15, 8, 18, 11};
  BPlusTree tree;
  for (size_t key : keys) {
    tree.insert(new Order{key, key * 10, key});
  }
  print_bplus_tree(tree, true);

  for (size_t key : keys) {
    std::cout << "Removing key: " << key << std::endl;
    tree.remove(key);
    std::cout << std::format("Key {} removed ", key) << std::endl;
    print_bplus_tree(tree, true);
  }
  std::cout << "Debug 2 passed\n" << std::endl;
}

void test_debug_3() {
  std::vector<size_t> keys{5, 19, 3, 6, 4, 15, 12, 18, 17, 14, 16, 1, 7, 13, 8, 2, 10, 9, 11, 20};
  BPlusTree tree;
  for (size_t key : keys) {
    tree.insert(new Order{key, key * 10, key});
  }
  print_bplus_tree(tree, true);

  for (size_t key : keys) {
    std::cout << "Removing key: " << key << std::endl;
    tree.remove(key);
    std::cout << std::format("Key {} removed ", key) << std::endl;
    print_bplus_tree(tree, true);
  }
  std::cout << "Debug 3 passed\n" << std::endl;
}

void test_debug_4() {
  std::vector<size_t> keys{5, 8, 11, 1, 14, 18, 13, 15, 10, 12, 4, 6, 17, 9, 19, 7, 3, 20, 16, 2};
  BPlusTree tree;
  for (size_t key : keys) {
    tree.insert(new Order{key, key * 10, key});
  }
  print_bplus_tree(tree, true);

  for (size_t key : keys) {
    std::cout << "Removing key: " << key << std::endl;
    tree.remove(key);
    std::cout << std::format("Key {} removed ", key) << std::endl;
    print_bplus_tree(tree, true);
  }
  std::cout << "Debug 4 passed\n" << std::endl;
}

void test_random_dataset() {
  BPlusTree tree;
  std::vector<size_t> keys(20);
  std::iota(keys.begin(), keys.end(), 1);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(keys.begin(), keys.end(), g);
  // Using std::format
  for (const size_t &key : keys) {
    std::cout << std::format("{},", key);
  }
  std::cout << std::endl;

  std::cout << "Inserting keys..." << std::endl;
  // Insert shuffled keys

  for (size_t key : keys) {
    tree.insert(new Order{key, key * 10, key % 100});
  }
  std::cout << "Searching keys..." << std::endl;

  // Verify all keys are present
  for (size_t key : keys) {
    assert(tree.search(key).has_value());
  }
  print_bplus_tree(tree, true, std::cout);

  //// Remove half of the keys
  for (size_t i = 0; i < 10; ++i) {
    tree.remove(keys[i]);
    print_bplus_tree(tree);
  }

  std::cout << "Verifying remaining keys: " << std::endl;
  // Verify removed keys are not found and remaining keys are present
  for (size_t i = 0; i < 20; ++i) {
    if (i < 10) {
      assert(!tree.search(keys[i]).has_value());
    } else {
      assert(tree.search(keys[i]).has_value());
    }
  }

  std::cout << "Random dataset test passed!\n" << std::endl;
}
void test_large_dataset() {
  BPlusTree tree;
  std::vector<size_t> keys(1000);
  std::iota(keys.begin(), keys.end(), 1);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(keys.begin(), keys.end(), g);

  std::cout << "Inserting keys..." << std::endl;
  // Insert shuffled keys

  for (size_t key : keys) {
    tree.insert(new Order{key, key * 10, key % 100});
  }
  std::cout << "Searching keys..." << std::endl;

  // Verify all keys are present
  for (size_t key : keys) {
    std::cout << "searching: " << key << std::endl;
    assert(tree.search(key).has_value());
  }

  // Remove half of the keys
  std::cout << "Removing keys..." << std::endl;
  for (size_t i = 0; i < 500; ++i) {
    std::cout << "Removing key: " << keys[i] << std::endl;
    tree.remove(keys[i]);
  }

  std::cout << "Verifying remaining keys: " << std::endl;
  // Verify removed keys are not found and remaining keys are present
  for (size_t i = 0; i < 1000; ++i) {
    if (i < 500) {
      assert(!tree.search(keys[i]).has_value());
    } else {
      assert(tree.search(keys[i]).has_value());
    }
  }

  std::cout << "Large dataset test passed!" << std::endl;
}

void test_range_search() {
  BPlusTree tree;
  std::vector<Order> orders = {{5, 100, 10}, {3, 90, 5},  {7, 110, 8}, {1, 80, 3},  {9, 120, 12},
                               {2, 85, 4},   {6, 105, 7}, {4, 95, 6},  {8, 115, 9}, {10, 125, 15}};

  // Insert orders
  for (const auto &order : orders) {
    tree.insert(new Order(order));
  }

  // Perform range search
  auto leaf_node = tree.range_search(6);
  assert(leaf_node.has_value());

  Node *cursor = *leaf_node;
  std::vector<size_t> expected_keys = {6, 7, 8, 9, 10};
  size_t i = 0;

  while (cursor && i < expected_keys.size()) {
    for (size_t j = 0; j < cursor->size && i < expected_keys.size(); ++j) {
      assert(cursor->keys[j] == expected_keys[i]);
      ++i;
    }
    if (i < expected_keys.size()) {
      cursor = std::get<Node *>(*cursor->children[cursor->size]);
    }
  }

  assert(i == expected_keys.size());
  std::cout << "Range search test passed!" << std::endl;
}

int main() {
  // test_insert_and_search();
  // test_remove();
  // test_debug_1();
  // test_debug_2();
  // test_debug_3();
  // test_debug_4();
  test_random_dataset();
  // test_large_dataset();
  // test_range_search();

  std::cout << "All tests passed successfully!" << std::endl;
  return 0;
}
