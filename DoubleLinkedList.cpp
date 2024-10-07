#include <memory>
#include <cstddef>
#include <utility>
#include <stdexcept>
#include <vector>
#include <type_traits>

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

        ~NodePool() = default;

        std::unique_ptr<Node> allocate() {
            if (free_nodes.empty()) {
                auto new_block = std::make_unique<char[]>(BlockSize);
                Node* block_start = reinterpret_cast<Node*>(new_block.get());

                for (std::size_t i = 0; i < nodes_per_block; ++i) {
                    free_nodes.push_back(&block_start[i]);
                }

                memory_blocks.push_back(std::move(new_block));
            }

            Node* node = free_nodes.back();
            free_nodes.pop_back();
            return std::unique_ptr<Node>(new (node) Node());
        }

        void deallocate(std::unique_ptr<Node> node) noexcept {
            node->~Node();
            free_nodes.push_back(node.release());
        }
    };

    std::unique_ptr<Node> head;
    Node* tail;
    std::size_t length;
    NodePool allocator;

    void link_nodes(Node* prev, std::unique_ptr<Node>& next) {
        if (prev) {
            prev->next = std::move(next);
            prev->next->prev = prev;
            next = std::move(prev->next);
        }
    }

public:
    DoubleLinkedList() noexcept : head(nullptr), tail(nullptr), length(0) {}

    ~DoubleLinkedList() = default;

    template<typename... Args>
    void emplace_front(Args&&... args) {
        auto new_node = allocator.allocate();
        new_node->data = T(std::forward<Args>(args)...);
        new_node->prev = nullptr;

        if (head) {
            new_node->next = std::move(head);
            new_node->next->prev = new_node.get();
        } else {
            tail = new_node.get();
        }

        head = std::move(new_node);
        ++length;
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        auto new_node = allocator.allocate();
        new_node->data = T(std::forward<Args>(args)...);
        new_node->next = nullptr;

        if (tail) {
            new_node->prev = tail;
            tail->next = std::move(new_node);
            tail = tail->next.get();
        } else {
            new_node->prev = nullptr;
            head = std::move(new_node);
            tail = head.get();
        }

        ++length;
    }

    void pop_front() {
        if (!head) return;

        auto old_head = std::move(head);
        head = std::move(old_head->next);
        if (head) {
            head->prev = nullptr;
        } else {
            tail = nullptr;
        }

        allocator.deallocate(std::move(old_head));
        --length;
    }

    void pop_back() {
        if (!tail) return;

        if (tail->prev) {
            Node* new_tail = tail->prev;
            allocator.deallocate(std::move(new_tail->next));
            new_tail->next = nullptr;
            tail = new_tail;
        } else {
            allocator.deallocate(std::move(head));
            head = nullptr;
            tail = nullptr;
        }

        --length;
    }

    [[nodiscard]] T& front() {
        if (!head) throw std::out_of_range{"List is empty"};
        return head->data;
    }

    [[nodiscard]] const T& front() const {
        if (!head) throw std::out_of_range{"List is empty"};
        return head->data;
    }

    [[nodiscard]] T& back() {
        if (!tail) throw std::out_of_range{"List is empty"};
        return tail->data;
    }

    [[nodiscard]] const T& back() const {
        if (!tail) throw std::out_of_range{"List is empty"};
        return tail->data;
    }

    void clear() noexcept {
        while (head) {
            pop_front();
        }
    }

    [[nodiscard]] std::size_t size() const noexcept { return length; }
    [[nodiscard]] bool empty() const noexcept { return length == 0; }

    // Iterator implementation
    class Iterator {
    private:
        Node* current;

    public:
        explicit Iterator(Node* node) : current(node) {}

        T& operator*() const { return current->data; }
        Iterator& operator++() { current = current->next.get(); return *this; }
        Iterator& operator--() { current = current->prev; return *this; }
        bool operator!=(const Iterator& other) const { return current != other.current; }
        Node* node() const { return current; }
    };

    Iterator begin() { return Iterator(head.get()); }
    Iterator end() { return Iterator(nullptr); }

    // ... (implement ReverseIterator similarly)

    void insert(Iterator pos, const T& value) {
        if (pos.node() == nullptr) {
            emplace_back(value);
            return;
        }

        auto new_node = allocator.allocate();
        new_node->data = value;
        new_node->prev = pos.node()->prev;
        new_node->next = std::move(pos.node()->prev->next);
        pos.node()->prev = new_node.get();

        if (new_node->prev) {
            new_node->prev->next = std::move(new_node);
        } else {
            head = std::move(new_node);
        }

        ++length;
    }

    Iterator erase(Iterator pos) {
        if (pos.node() == nullptr) return end();

        Node* to_erase = pos.node();
        Node* next_node = to_erase->next.get();

        if (to_erase->prev) {
            to_erase->prev->next = std::move(to_erase->next);
            if (next_node) {
                next_node->prev = to_erase->prev;
            } else {
                tail = to_erase->prev;
            }
        } else {
            head = std::move(to_erase->next);
            if (head) {
                head->prev = nullptr;
            } else {
                tail = nullptr;
            }
        }

        allocator.deallocate(std::unique_ptr<Node>(to_erase));
        --length;

        return Iterator(next_node);
    }

    // ... (implement other methods like swap, assign, resize)
};


int main()
{
    DoubleLinkedList<int> list;

    // Test basic operations
    /*
    list.push_back(1);
    list.push_back(2);
    list.push_front(0);

    // Test iteration
    std::cout << "Forward: ";
    for (const auto &item : list)
    {
        std::cout << item << " ";
    }
    std::cout << std::endl;

    std::cout << "Reverse: ";
    for (auto it = list.rbegin(); it != list.rend(); ++it)
    {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // Test insert and erase
    auto it = list.begin();
    ++it;
    list.insert(it, 5);
    it = list.begin();
    ++it;
    ++it;
    list.erase(it);

    // Test bulk operations
    std::vector<int> v = {10, 20, 30, 40, 50};
    list.assign(v.begin(), v.end());

    list.resize(7, 100);

    std::cout << "After bulk operations: ";
    for (const auto &item : list)
    {
        std::cout << item << " ";
    }
    std::cout << std::endl;

    // Test swap
    DoubleLinkedList<int> other_list;
    other_list.push_back(999);

    std::cout << "Before swap: ";
    for (const auto &item: list) {
        std::cout << item <<" ";
    }
    std::cout<<std::endl;
    // FIXME: swap will destruct the NodeAllocator first
    list.swap(other_list);

    std::cout << "After swap: ";
    for (const auto &item : list)
    {
        std::cout << item << " ";
    }
    std::cout << std::endl;
    std::cout << "done\n";
    return 0;
    */
}
