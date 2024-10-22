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
    std::cout << "\nInserting order: " << order.key << std::endl;
    tree.insert(new Order(order));
  }
  print_bplus_tree(tree);

  // Search for each order
  for (const auto &order : orders) {
    auto result = tree.search(order.key);
    assert(result.has_value());
    std::cout << "result key: " << result.value()->key << ", order key:" << order.key << std::endl;
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
  std::vector<int> keys = {5, 3, 7, 1, 9, 2, 6, 4, 8, 10};

  // Insert orders
  for (int key : keys) {
    std::cout << "\nInserting order: " << key << std::endl;
    tree.insert(new Order{key, key * 10, key});
    // std::cout << tree << std::endl;
  }

  // Remove some keys
  std::cout << "\nRemoving 3\n" << std::endl;
  tree.remove(3);
  std::cout << "\nAfter removing 3\n" << std::endl;
  print_bplus_tree(tree);
  // std::cout << "\nRemoving 7\n" << tree << std::endl;
  // std::cout << &tree << std::endl;
  tree.remove(7);
  // std::cout << "\nAfter removing 7\n" << tree << std::endl;
  tree.remove(1);

  // Verify removed keys are not found
  assert(!tree.search(3).has_value());
  assert(!tree.search(7).has_value());
  assert(!tree.search(1).has_value());

  // Verify remaining keys are still present
  for (int key : {2, 4, 5, 6, 8, 9, 10}) {
    assert(tree.search(key).has_value());
  }

  std::cout << "Remove test passed!" << std::endl;
}

void test_large_dataset() {
  BPlusTree tree;
  std::vector<int> keys(1000);
  std::iota(keys.begin(), keys.end(), 1);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(keys.begin(), keys.end(), g);

  // Insert shuffled keys
  for (int key : keys) {
    tree.insert(new Order{key, key * 10, key % 100});
  }

  // Verify all keys are present
  for (int key : keys) {
    assert(tree.search(key).has_value());
  }

  // Remove half of the keys
  for (int i = 0; i < 500; ++i) {
    tree.remove(keys[i]);
  }

  // Verify removed keys are not found and remaining keys are present
  for (int i = 0; i < 1000; ++i) {
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
  std::vector<int> expected_keys = {6, 7, 8, 9, 10};
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
  BPlusTree tree;
  test_insert_and_search();
  test_remove();
  test_large_dataset();
  test_range_search();

  std::cout << "All tests passed successfully!" << std::endl;
  return 0;
}

// #include "b_plus_tree.h"
// #include <cassert>
// #include <iostream>
//
// void test_b_plus_tree() {
//   BPlusTree tree;
//
//   // Test case 1: Inserting into an empty B+ Tree
//   {
//     Order *order1 = new Order{1, 100, 10};
//     tree.insert(order1);
//     assert(tree.get_root().has_value());
//     assert(tree.get_root().value()->size == 1);
//     assert(tree.get_root().value()->keys[0] == 1);
//   }
//   // tree.pretty_print();
//
//   // Test case 2: Inserting multiple orders
//   {
//     Order *order2 = new Order{2, 200, 20};
//     Order *order3 = new Order{3, 300, 30};
//     tree.insert(order2);
//     tree.insert(order3);
//     assert(tree.get_root().value()->size == 3);
//     assert(tree.get_root().value()->keys[0] == 1);
//     assert(tree.get_root().value()->keys[1] == 2);
//     assert(tree.get_root().value()->keys[2] == 3);
//   }
//   // tree.print();
//
//   // Test case 3: Searching for existing keys
//   {
//     auto result1 = tree.search(1);
//     assert(result1.has_value());
//     assert(result1.value()->price == 100);
//
//     auto result2 = tree.search(2);
//     assert(result2.has_value());
//     assert(result2.value()->price == 200);
//
//     auto result3 = tree.search(3);
//     assert(result3.has_value());
//     assert(result3.value()->price == 300);
//   }
//
//   // tree.print();
//   //  Test case 4: Searching for a non-existent key
//   {
//     auto result = tree.search(4);
//     assert(!result.has_value());
//   }
//
//   // Test case 5: Inserting to trigger a split
//   {
//     for (int i = 4; i <= 6; ++i) {
//       Order *order = new Order{i, i * 100, i * 10};
//       tree.insert(order);
//     }
//     assert(tree.get_root().has_value());
//     assert(tree.get_root().value()->size == 2); // Assuming split occurs 2 twitce
//   }
//   // tree.print();
//
//   // Test case 6: Check if new root is created after split
//   {
//     auto root = tree.get_root().value();
//     assert(root->is_inner_root());
//     assert(root->size == 2); // The new root should have two keys after split
//   }
//
//   // Test case 7: Check split and structure of children
//   {
//     auto root = tree.get_root().value();
//     Node *child1 = std::get<Node *>(root->children[0].value());
//     Node *child2 = std::get<Node *>(root->children[1].value());
//     Node *child3 = std::get<Node *>(root->children[2].value());
//
//     assert(child1->size == 2); // First child should have two keys
//     assert(child1->keys[0] == 1);
//     assert(child1->keys[1] == 2);
//
//     assert(child2->size == 2); // Second child should also have two keys
//     assert(child2->keys[0] == 3);
//     assert(child2->keys[1] == 4);
//
//     assert(child3->size == 2); // Third child should also have two keys
//     assert(child3->keys[0] == 5);
//     assert(child3->keys[1] == 6);
//     // assert(child3->is_root());
//   }
//
//   // Test case 8: Check range search
//   {
//     auto range_result = tree.range_search(3);
//     assert(range_result.has_value());
//     assert(range_result.value()->is_leaf);
//   }
//
//   // Test case 9: Inserting orders with the same key
//   // FIXME (Peiyun): Need to handlie duplicates prpperly
//   {
//     Order *orderDuplicate = new Order{1, 150, 15};
//     tree.insert(orderDuplicate);
//     // This might require handling duplicates based on your implementation logic.
//     auto result = tree.search(1);
//     assert(result.has_value());
//     assert(result.value()->price == 150); // Check if it updated
//   }
//
//   // Test case 10: Inserting enough keys to cause multiple splits
//   {
//     for (int i = 7; i <= 15; ++i)
//     // for (int i = 7; i <= 10; ++i)
//     {
//       Order *order = new Order{i, i * 100, i * 10};
//       tree.insert(order);
//     }
//     assert(tree.get_root().has_value());
//     // tree.print();
//     std::cout << "root key: " << tree.get_root().value()->keys[0] << std::endl;
//     std::cout << "root size: " << tree.get_root().value()->size << std::endl;
//     assert((tree.get_root().value()->size = 1) && (tree.get_root().value()->keys[0] == 7)); // Ensure root 1  key
//   }
//
//   // Clean up orders in a proper implementation
//   // Ideally implement destructor for BPlusTree that deletes all nodes.
//
//   std::cout << "All tests passed!" << std::endl;
// }
//
// int main() {
//   test_b_plus_tree();
//   return 0;
// }
//
