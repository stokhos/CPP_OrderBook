#include "double_linked_list.h"
int main() {

  DoubleLinkedList<int> list;

  // Test basic operations
  list.push_back(1);
  list.push_back(2);
  list.push_back(0);

  // Test iteration
  std::cout << "Forward: ";
  for (const auto &item : list) {
    std::cout << item << " ";
  }
  std::cout << std::endl;

  std::cout << "Reverse: ";
  for (auto it = list.rbegin(); it != list.rend(); ++it) {
    std::cout << *it << " ";
  }
  std::cout << std::endl;

  // Test bulk operations
  std::vector<int> v = {10, 20, 30, 40, 50};
  list.assign(v.begin(), v.end());

  std::cout << "After bulk operations: " << std::endl;
  list.resize(7, 100);
  std::cout << "After resize" << std::endl;

  std::cout << "After bulk operations: ";
  for (const auto &item : list) {
    std::cout << item << " ";
  }
  std::cout << std::endl;

  // Test swap
  DoubleLinkedList<int> other_list;
  other_list.push_back(999);

  std::cout << "list Before swap: ";
  for (const auto &item : list) {
    std::cout << item << " ";
  }
  std::cout << std::endl;

  std::cout << "other list Before swap: ";
  for (const auto &item : other_list) {
    std::cout << item << " ";
  }
  std::cout << std::endl;
  // FIXME: swap will destruct the NodeAllocator first
  list.swap(other_list);

  std::cout << "list After swap: ";
  for (const auto &item : list) {
    std::cout << item << " ";
  }
  std::cout << std::endl;
  std::cout << "done\n";

  std::cout << "other list After swap: ";
  for (const auto &item : other_list) {
    std::cout << item << " ";
  }
  std::cout << std::endl;
  std::cout << "done\n";

  return 0;
}
