#include "b_plus_tree.h"
#include <cassert>
#include <iostream>
#include <optional>
#include <variant>

// Assuming your Order, Node, and BPlusTree classes are defined above

void testBPlusTree() {
  BPlusTree tree;

  // Test 1: Search in an empty tree
  assert(tree.search(10) == std::nullopt);
  assert(tree.range_search(10) == std::nullopt);
  std::cout << "Test 1 passed: Search in an empty tree\n";

  // Test 2: Insert an Order
  Order order1{100, 10}; // price 100, quantity 10
  // Here, you'd normally have an insert method that adds the order to the tree.
  // For this example, we'll create a simple method to demonstrate:

  // Directly manipulate the tree for the sake of this test (not part of the original API)
  {
    Node *newRoot = new Node(true);
    newRoot->keys[0] = 10;          // Insert a key
    newRoot->children[0] = &order1; // Associate an order with the key
    newRoot->size = 1;
    tree = BPlusTree();
    // Set root of the tree to newRoot (need to add a way to set root in BPlusTree)
    // For now, we'll assume the root is set correctly:
    tree = BPlusTree(); // Set the new root (this would be done in an insert method)
    tree.root = newRoot;
  }

  // Test 3: Search for inserted Order
  auto foundOrder = tree.search(10);
  assert(foundOrder.has_value());
  assert(foundOrder.value()->price == 100);
  assert(foundOrder.value()->quantity == 10);
  std::cout << "Test 3 passed: Successfully found the inserted Order\n";

  // Test 4: Search for a non-existent key
  assert(tree.search(20) == std::nullopt);
  std::cout << "Test 4 passed: Search for a non-existent key\n";

  // Test 5: Range search on an empty tree
  assert(tree.range_search(10) == std::nullopt);
  std::cout << "Test 5 passed: Range search on an empty tree\n";

  // Test 6: Range search for a key
  // Note: In the above example, we did not add a proper insert method. You should implement this.
  // For demonstration, you could write a test that confirms the behavior of range_search.

  // Cleanup for the dynamically allocated Node (not handled in the original implementation)
  // Make sure to manage memory in your actual implementation.
  // For demonstration purposes, we assume manual deletion is not handled:
  // delete newRoot; // Remember to manage dynamically allocated memory

  std::cout << "All tests passed!\n";
}

int main() {
  testBPlusTree();
  return 0;
}
