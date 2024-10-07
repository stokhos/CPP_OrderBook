#include <iostream>
#include <memory>
#include <vector>
#include <cassert>

template <typename T, std::size_t BlockSize = 4096>
class DoubleLinkedList {
private:
    struct Node {
        T data;
        std::unique_ptr<Node> next;
        Node* prev;

        template<typename... Args>
        Node(Args&&... args) : data(std::forward<Args>(args)...), next(nullptr), prev(nullptr) {}
    };

    class NodePool {
    private:
        static constexpr std::size_t nodes_per_block = BlockSize / sizeof(Node);
        std::vector<std::unique_ptr<char[]>> memory_blocks;
        std::vector<Node*> free_nodes;

    public:
        NodePool() = default;
        NodePool(const NodePool&) = delete;
        NodePool& operator=(const NodePool&) = delete;
        NodePool(NodePool&&) = default;
        NodePool& operator=(NodePool&&) = default;

        Node* allocate() {
            if (free_nodes.empty()) {
                allocate_block();
            }
            Node* node = free_nodes.back();
            free_nodes.pop_back();
            return node;
        }

        void deallocate(Node* node) {
            free_nodes.push_back(node);
        }

    private:
        void allocate_block() {
            auto block = std::make_unique<char[]>(BlockSize);
            Node* block_start = reinterpret_cast<Node*>(block.get());

            for (std::size_t i = 0; i < nodes_per_block; ++i) {
                free_nodes.push_back(block_start + i);
            }

            memory_blocks.push_back(std::move(block));
        }
    };

    NodePool allocator;
    std::unique_ptr<Node> head;
    Node* tail;

public:
    DoubleLinkedList() : head(nullptr), tail(nullptr) {}

    ~DoubleLinkedList() {
        clear();
    }

    void append(const T& value) {
        Node* new_node = allocator.allocate();
        new(new_node) Node(value);

        if (!head) {
            head.reset(new_node);
            tail = head.get();
        } else {
            tail->next.reset(new_node);
            new_node->prev = tail;
            tail = new_node;
        }
    }

    void prepend(const T& value) {
        Node* new_node = allocator.allocate();
        new(new_node) Node(value);

        if (!head) {
            head.reset(new_node);
            tail = head.get();
        } else {
            new_node->next = std::move(head);
            new_node->next->prev = new_node;
            head.reset(new_node);
        }
    }

    void remove(const T& value) {
        Node* current = head.get();
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
            current = current->next.get();
        }
    }

    void printForward() const {
        Node* current = head.get();
        while (current) {
            std::cout << current->data << " ";
            current = current->next.get();
        }
        std::cout << std::endl;
    }

    void printBackward() const {
        Node* current = tail;
        while (current) {
            std::cout << current->data << " ";
            current = current->prev;
        }
        std::cout << std::endl;
    }

    void clear() {
        Node* current = head.get();
        while (current) {
            Node* next = current->next.release();
            allocator.deallocate(current);
            current = next;
        }
        head.reset();
        tail = nullptr;
    }
};

int main() {
    DoubleLinkedList<int> list;
    list.append(1);
    //list.append(2);
    //list.append(3);
    //list.prepend(0);
    //list.printForward();  // Output: 0 1 2 3
    //list.printBackward(); // Output: 3 2 1 0

    /*
    list.remove(2);
    list.printForward();  // Output: 0 1 3
    list.printBackward(); // Output: 3 1 0
    */

    return 0;
}
