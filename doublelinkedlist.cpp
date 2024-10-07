#include <csignal>
#include <iostream>
#include <vector>
#include <cstddef>
#include <new>
#include <stdexcept>
#include <cstring>
//#include <algorithm>
#include <utility>

template <typename T, size_t BlockSize = 4096>
class DoubleLinkedList
{
private:
    struct alignas(std::max(alignof(T), alignof(void *))) Node
    {
        T data;
        Node *prev;
        Node *next;
        Node(const T &value) : data(value), prev(nullptr), next(nullptr) {}
    };

    class NodePool
    {
    private:
        static constexpr size_t nodes_per_block = BlockSize / sizeof(Node);
        std::vector<char *> memory_blocks;
        Node *free_nodes;

    public:
        NodePool() : free_nodes(nullptr)
        {
            static_assert(nodes_per_block > 0, "BlockSize is too small");
        }
        NodePool(const NodePool&) = delete;
        NodePool& operator=(const NodePool&) = delete;

        NodePool(NodePool &&other) noexcept:
            memory_blocks(std::move(other.memory_blocks)),
            //free_nodes(other.free_nodes)
            free_nodes(std::move(other.free_nodes))
        {
            //other.free_nodes = nullptr;
        }

        NodePool& operator=(NodePool&& other) noexcept
        {
            if (this != &other) {
                //clear();
                memory_blocks = std::move(other.memory_blocks);
                free_nodes = std::move(other.free_nodes);
                //free_nodes = other.free_nodes;
                //other.free_nodes = nullptr;
            }

            return *this;
        }

        void clear() {
            for (char *block : memory_blocks)
            {
                ::operator delete(block);
            }
        }

        ~NodePool()
        {
            std::cout << __func__ << ": " << memory_blocks.size() << std::endl;
            clear();
        }

        Node *allocate()
        {
            if (free_nodes == nullptr)
            {
                char *new_block = static_cast<char *>(::operator new(BlockSize));
                memory_blocks.push_back(new_block);

                Node *block_start = reinterpret_cast<Node *>(new_block);
                for (size_t i = 0; i < nodes_per_block - 1; ++i)
                {
                    block_start[i].next = &block_start[i + 1];
                }
                block_start[nodes_per_block - 1].next = nullptr;
                free_nodes = block_start;
            }

            Node *allocated = free_nodes;
            free_nodes = free_nodes->next;
            return allocated;
        }

        void deallocate(Node *node)
        {
            node->next = free_nodes;
            free_nodes = node;
        }
    };

    Node *head;
    Node *tail;
    size_t length;
    NodePool allocator;

    void link_nodes(Node *prev, Node *next)
    {
        if (prev)
            prev->next = next;
        if (next)
            next->prev = prev;
    }

public:
    DoubleLinkedList() : head(nullptr), tail(nullptr), length(0) {}

    ~DoubleLinkedList()
    {
        std::cout << "~DoubleLinkedList" << std::endl;
        clear();
    }

    void push_front(const T &value)
    {
        Node *new_node = allocator.allocate();
        new (new_node) Node(value);
        new_node->next = head;
        if (head)
            head->prev = new_node;
        head = new_node;
        if (!tail)
            tail = head;
        ++length;
    }

    void push_back(const T &value)
    {
        Node *new_node = allocator.allocate();
        new (new_node) Node(value);
        new_node->prev = tail;
        if (tail)
            tail->next = new_node;
        tail = new_node;
        if (!head)
            head = tail;
        ++length;
    }

    void pop_front()
    {
        if (!head)
            return;
        Node *old_head = head;
        head = head->next;
        if (head)
            head->prev = nullptr;
        else
            tail = nullptr;
        old_head->~Node();
        allocator.deallocate(old_head);
        --length;
    }

    void pop_back()
    {
        if (!tail)
            return;
        Node *old_tail = tail;
        tail = tail->prev;
        if (tail)
            tail->next = nullptr;
        else
            head = nullptr;
        old_tail->~Node();
        allocator.deallocate(old_tail);
        --length;
    }

    T &front()
    {
        if (!head)
            throw std::out_of_range("List is empty");
        return head->data;
    }

    const T &front() const
    {
        if (!head)
            throw std::out_of_range("List is empty");
        return head->data;
    }

    T &back()
    {
        if (!tail)
            throw std::out_of_range("List is empty");
        return tail->data;
    }

    const T &back() const
    {
        if (!tail)
            throw std::out_of_range("List is empty");
        return tail->data;
    }

    void clear()
    {
        Node *current = head;
        while (current)
        {
            Node *next = current->next;
            current->~Node();
            allocator.deallocate(current);
            current = next;
        }
        head = tail = nullptr;
        length = 0;
    }

    size_t size() const
    {
        return length;
    }

    bool empty() const
    {
        return length == 0;
    }

    class Iterator
    {
    private:
        Node *current;

    public:
        Iterator(Node *node) : current(node) {}

        T &operator*() const
        {
            return current->data;
        }

        Iterator &operator++()
        {
            current = current->next;
            return *this;
        }

        Iterator &operator--()
        {
            current = current->prev;
            return *this;
        }

        bool operator!=(const Iterator &other) const
        {
            return current != other.current;
        }

        Node *node() const { return current; }
    };

    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }

    class ReverseIterator
    {
    private:
        Node *current;

    public:
        ReverseIterator(Node *node) : current(node) {}

        T &operator*() const
        {
            return current->data;
        }

        ReverseIterator &operator++()
        {
            current = current->prev;
            return *this;
        }

        ReverseIterator &operator--()
        {
            current = current->next;
            return *this;
        }

        bool operator!=(const ReverseIterator &other) const
        {
            return current != other.current;
        }
    };

    ReverseIterator rbegin() { return ReverseIterator(tail); }
    ReverseIterator rend() { return ReverseIterator(nullptr); }

    void insert(Iterator pos, const T &value)
    {
        if (pos.node() == nullptr)
        {
            push_back(value);
            return;
        }
        Node *new_node = allocator.allocate();
        new (new_node) Node(value);
        Node *next_node = pos.node();
        Node *prev_node = next_node->prev;
        new_node->prev = prev_node;
        new_node->next = next_node;
        link_nodes(prev_node, new_node);
        link_nodes(new_node, next_node);
        if (next_node == head)
            head = new_node;
        ++length;
    }

    Iterator erase(Iterator pos)
    {
        if (pos.node() == nullptr)
            return end();
        Node *to_erase = pos.node();
        Node *next_node = to_erase->next;
        Node *prev_node = to_erase->prev;
        link_nodes(prev_node, next_node);
        if (to_erase == head)
            head = next_node;
        if (to_erase == tail)
            tail = prev_node;
        to_erase->~Node();
        allocator.deallocate(to_erase);
        --length;
        return Iterator(next_node);
    }

    void swap(DoubleLinkedList &other) noexcept
    {
        std::swap(head, other.head);
        std::swap(tail, other.tail);
        std::swap(length, other.length);
        std::cout<< "move constructible: "<< std::boolalpha << std::is_move_constructible_v<NodePool> << std::endl;
        std::cout<< "move assignable: "<< std::boolalpha << std::is_move_assignable<NodePool>::value << std::endl;
        std::swap(allocator, other.allocator);
    }

    // Bulk operations
    template <typename InputIt>
    void assign(InputIt first, InputIt last)
    {
        clear();
        while (first != last)
        {
            push_back(*first);
            ++first;
        }
    }

    void resize(size_t new_size, const T &value = T())
    {
        while (length > new_size)
        {
            pop_back();
        }
        while (length < new_size)
        {
            push_back(value);
        }
    }
};

int main()
{
    DoubleLinkedList<int> list;

    // Test basic operations
    list.push_back(1);
    list.push_back(2);
    list.push_back(0);

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

    std::cout << "list Before swap: ";
    for (const auto &item: list) {
        std::cout << item <<" ";
    }
    std::cout<<std::endl;

    std::cout << "other list Before swap: ";
    for (const auto &item: other_list) {
        std::cout << item <<" ";
    }
    std::cout<<std::endl;
    // FIXME: swap will destruct the NodeAllocator first
    list.swap(other_list);

    std::cout << "list After swap: ";
    for (const auto &item : list)
    {
        std::cout << item << " ";
    }
    std::cout << std::endl;
    std::cout << "done\n";

    std::cout << "other list After swap: ";
    for (const auto &item : other_list)
    {
        std::cout << item << " ";
    }
    std::cout << std::endl;
    std::cout << "done\n";

    return 0;
}
