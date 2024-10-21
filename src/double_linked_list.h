#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

template <typename T, std::size_t BlockSize = 4096> class DoubleLinkedList {
private:
  struct alignas(std::max(alignof(T), alignof(void *))) Node {
    T data;
    Node *next;
    Node *prev;

    template <typename... Args>
    Node(Args &&...args) : data(std::forward<Args>(args)...), next(nullptr), prev(nullptr) {}
  };

  class NodePool {
  private:
    static constexpr std::size_t nodes_per_block = BlockSize / sizeof(Node);
    std::vector<std::unique_ptr<char[]>> memory_blocks;
    Node *free_nodes;

  public:
    NodePool() : free_nodes(nullptr) {};
    NodePool(const NodePool &) = delete;
    NodePool &operator=(const NodePool &) = delete;
    NodePool(NodePool &&other) noexcept
        : memory_blocks(std::move(other.memory_blocks)), free_nodes(std::move(other.free_nodes)) {}

    NodePool &operator=(NodePool &&other) noexcept {
      if (this != &other) {
        // clear();
        memory_blocks = std::move(other.memory_blocks);
        free_nodes = std::move(other.free_nodes);
      }

      return *this;
    }

    Node *allocate() {
      if (!free_nodes) {
        allocate_block();
      }
      Node *node = free_nodes;
      free_nodes = node->next;
      return node;
    }

    void deallocate(Node *node) {
      free_nodes = (!free_nodes) ? node : free_nodes->next = node;
      node->next = nullptr;
    }

  private:
    void allocate_block() {
      auto block = std::make_unique<char[]>(BlockSize);
      Node *block_start = reinterpret_cast<Node *>(block.get());

      free_nodes = block_start;
      for (std::size_t i = 1; i < nodes_per_block; ++i) {
        free_nodes->next = (block_start + i);
      }
      memory_blocks.push_back(std::move(block));
    }
  };

  NodePool allocator;
  Node *head;
  Node *tail;
  size_t _size = 0;

public:
  DoubleLinkedList() : head(nullptr), tail(nullptr) {}

  ~DoubleLinkedList() { clear(); }

  void push_back(const T &value) {
    Node *new_node = allocator.allocate();
    new (new_node) Node(value);

    if (!head) {
      head = new_node;
      tail = head;
    } else {
      new_node->prev = std::move(tail);
      new_node->prev->next = new_node;
      tail = new_node;
      // tail->next = std::move(new_node);
    }
    ++_size;
  }

  void push_front(const T &value) {
    Node *new_node = allocator.allocate();
    new (new_node) Node(value);

    if (!head) {
      head = new_node;
      tail = head;
    } else {
      new_node->next = std::move(head);
      new_node->next->prev = new_node;
      head = new_node;
    }
    ++_size;
  }

  void pop_back() {
    if (!tail) {
      return;
    }
    Node *old_tail = tail;
    tail = tail->prev;
    if (tail) {
      tail->next = nullptr;
    } else {
      head = nullptr;
    }
    old_tail->~Node();
    allocator.deallocate(old_tail);
    --_size;
  }

  void pop_front() {
    if (!head) {
      return;
    }
    Node *old_head = head;
    head = head->next;
    if (head) {
      head->prev = nullptr;
    } else {
      tail = nullptr;
    }
    old_head->~Node();
    allocator.deallocate(old_head);
    --_size;
  }

  T &front() {
    if (!head) {
      throw std::out_of_range("Empty list");
    }
    return head->data;
  }

  const T &front() const {
    if (!head) {
      throw std::out_of_range("Empty list");
    }
    return head->data;
  }

  T &back() {
    if (!tail) {
      throw std::out_of_range("Empty list");
    }
    return tail->data;
  }

  const T &back() const {
    if (!tail) {
      throw std::out_of_range("Empty list");
    }
    return tail->data;
  }

  size_t size() const { return _size; }

  bool empty() const { return _size == 0; }

  class Iterator {
  private:
    Node *current;

  public:
    Iterator(const Iterator &) = delete;
    Iterator(Iterator &&) = delete;
    Iterator &operator=(const Iterator &) = delete;
    Iterator &operator=(Iterator &&) = delete;
    Iterator(Node *node) : current(node) {}
    Iterator &operator++() {
      current = current->next;
      return *this;
    }
    Iterator &operator--() {
      current = current->prev;
      return *this;
    }
    T &operator*() { return current->data; }
    T *operator->() { return &current->data; }
    Node *node() const { return current; }
    bool operator==(const Iterator &other) const { return current == other.current; }
    bool operator!=(const Iterator &other) const { return current != other.current; }
  };
  Iterator begin() { return Iterator(head); }
  Iterator end() { return Iterator(nullptr); }

  class ReverseIterator {
  private:
    Node *current;

  public:
    ReverseIterator(Node *node) : current(node) {}

    ReverseIterator &operator++() {
      current = current->prev;
      return *this;
    }
    ReverseIterator &operator--() {
      current = current->next;
      return *this;
    }

    T &operator*() const { return current->data; }
    T *operator->() const { return &current->data; }
    Node *node() const { return current; }

    bool operator==(const ReverseIterator &other) const { return current == other.current; }
    bool operator!=(const ReverseIterator &other) const { return current != other.current; }
  };

  ReverseIterator rbegin() { return ReverseIterator(tail); }
  ReverseIterator rend() { return ReverseIterator(nullptr); }

  void swap(DoubleLinkedList &other) noexcept {

    std::swap(head, other.head);
    std::swap(tail, other.tail);
    std::swap(_size, other._size);
    std::swap(allocator, other.allocator);
  }

  void clear() {
    Node *current = head;
    while (current) {
      Node *next = current->next;
      current->~Node();
      allocator.deallocate(current);
      current = next;
    }
    head = tail = nullptr;
    _size = 0;
  }

  void remove(const T &value) {
    Node *current = head;
    while (current) {
      if (current->data == value) {
        if (current->prev) {
          current->prev->next = std::move(current->next);
          if (current->next) {
            current->next->prev = current->prev;
          } else {
            tail = current->prev;
          }
        } else {
          head = std::move(current->next);
          if (head) {
            head->prev = nullptr;
          } else {
            tail = nullptr;
          }
        }
        allocator.deallocate(current);
        return;
      }
      current = current->next;
    }
  }

  // buld operations
  template <typename InputIt> void assign(InputIt first, InputIt last) {
    clear();
    for (auto it = first; it != last; ++it) {
      push_back(*it);
    }
  }

  void resize(size_t new_size, const T &value = T()) {
    while (_size < new_size) {
      push_back(value);
    }
    while (_size > new_size) {
      pop_back();
    }
  }

  void printForward() const {
    Node *current = head;
    while (current) {
      std::cout << current->data << " ";
      current = current->next;
    }
    std::cout << std::endl;
  }

  void printBackward() const {
    Node *current = tail;
    while (current) {
      std::cout << current->data << " ";
      current = current->prev;
    }
    std::cout << std::endl;
  }
};
