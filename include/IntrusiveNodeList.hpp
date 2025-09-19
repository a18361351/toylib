// IntrusiveNodeList.hpp: 
// Header file for simple intrusive linked list
// 简单的侵入式链表头文件

#ifndef TOYLIB_INTRUSIVE_NODE_LIST_HEADER
#define TOYLIB_INTRUSIVE_NODE_LIST_HEADER

#include <cstddef>
#include <iterator>

namespace toylib {
    
// 该宏的作用是，通过某个类的成员指针，获取该类对象本体的指针
// offsetof来自cstddef，作用是获取某个成员在类中的偏移量
#define CONTAINER_OF(ptr, type, member) \
    (reinterpret_cast<type*>((char*)(ptr) - offsetof(type, member)))

// 侵入式链表节点对象
// 侵入式链表与传统链表的区别在于：侵入式链表将链表节点对象嵌入用户定义的数据对象中，
// 其定义类似：
// ```C++
//  struct DataNode {
//      int data_;
//      DataNode* prev_;
//      DataNode* next_;
//  }
// ```

struct intrusive_node {
    intrusive_node() : prev_(nullptr), next_(nullptr) {}
    intrusive_node* prev_;
    intrusive_node* next_;
    void relink(intrusive_node* new_prev, intrusive_node* new_next) {
        prev_ = new_prev;
        next_ = new_next;
    }
};

// 代表侵入式链表的对象
// 链表对象需要管理链表节点对象的插入、删除、遍历等操作
// 我们这里使用带dummy + 循环 + 双链表的设计
template <typename T, intrusive_node T::* NodeMember>
class intrusive_list {
private:
    intrusive_node dummy_node;
    size_t size_;
    static intrusive_node* GetNode(T* obj) {
        return &(obj->*NodeMember);
    }

    static T* GetObject(intrusive_node* node) {
        return CONTAINER_OF(node, T, NodeMember);
    }

public:
    // 迭代器的实现
    class iterator {
    private:
        intrusive_node* pos_;
        iterator(intrusive_node* pos) : pos_(pos) {}
        friend class intrusive_list<T, NodeMember>;
    public:
        // 我们约定pos_ == dummy_时为end()

        // 迭代器的类型定义
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        
        // 顺序迭代器要求的方法：
        // operator++/--(void/int)
        // operator==/!=
        // operator*()
        // operator->()
        // 后置自增/自减
        
        // 默认构造函数
        iterator() : pos_(nullptr) {}

        // 注意：由于链表是循环的，所以++end()反而会绕回到起始位置上。
        // 不过++end()本身就是未定义行为，所以正常情况不要这样做。
        iterator& operator++() {
            pos_ = pos_->next_;
            return *this;
        }
        iterator operator++(int) {
            iterator ret = *this;
            ++(*this);
            return ret;
        }

        // --begin()也会有类似的绕回现象。
        iterator& operator--() {
            pos_ = pos_->prev_;
            return *this;
        }
        iterator operator--(int) {
            iterator ret = *this;
            --(*this);
            return ret;
        }

        bool operator==(const iterator& other) const {
            return pos_ == other.pos_;
        }
        bool operator!=(const iterator& other) const {
            return pos_ != other.pos_;
        }

        // 不要去解引用end()迭代器……
        reference operator*() const {
            return *GetObject(pos_);
        }
        // 不要去解引用end()迭代器……
        pointer operator->() const {
            return GetObject(pos_);
        }
    };

    // 常迭代器
    class const_iterator {
    private:
        intrusive_node* pos_;
        const_iterator(intrusive_node* pos) : pos_(pos) {}
        friend class intrusive_list<T, NodeMember>;
    public:
        // 常迭代器的类型定义，主要区别在于不能修改指向对象的值
        using value_type = T;
        using pointer = const T*;
        using reference = const T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        
        // 默认构造函数
        const_iterator() : pos_(nullptr) {}
        // 能从普通迭代器转为常迭代器
        const_iterator(const iterator& it) : pos_(it.pos_) {}

        // ++end()是未定义行为
        const_iterator& operator++() {
            pos_ = pos_->next_;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator ret = *this;
            ++(*this);
            return ret;
        }

        // --begin()是未定义行为
        const_iterator& operator--() {
            pos_ = pos_->prev_;
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator ret = *this;
            --(*this);
            return ret;
        }

        bool operator==(const const_iterator& other) const {
            return pos_ == other.pos_;
        }
        bool operator!=(const const_iterator& other) const {
            return pos_ != other.pos_;
        }

        // 不要去解引用end()迭代器……
        reference operator*() const {
            return *GetObject(pos_);
        }
        // 不要去解引用end()迭代器……
        pointer operator->() const {
            return GetObject(pos_);
        }
    };

    // ===============================
    // ctors/copys/moves
    
    intrusive_list() : size_(0) {
        dummy_node.relink(&dummy_node, &dummy_node);
    }

    // noncopyable
    intrusive_list(const intrusive_list&) = delete;
    intrusive_list& operator=(const intrusive_list&) = delete;
    
    // move ops
    intrusive_list(intrusive_list&& rhs) noexcept : size_(rhs.size_) {
        // 重要的一点是将对面链表中dummy相邻节点的prev/next修改
        if (rhs.empty()) {
            dummy_node.relink(&dummy_node, &dummy_node);
            return;
        }
        dummy_node.relink(rhs.dummy_node.prev_, rhs.dummy_node.next_);
        rhs.dummy_node.prev_->next_ = &dummy_node;
        rhs.dummy_node.next_->prev_ = &dummy_node;
        rhs.dummy_node.relink(&rhs.dummy_node, &rhs.dummy_node);
        rhs.size_ = 0;
    }
    intrusive_list& operator=(intrusive_list&& rhs) noexcept {
        if (this != &rhs) {
            clear();
            if (rhs.empty()) {
                dummy_node.relink(&dummy_node, &dummy_node);
                size_ = 0;
                return *this;
            }
            dummy_node.relink(rhs.dummy_node.prev_, rhs.dummy_node.next_);
            rhs.dummy_node.prev_->next_ = &dummy_node;
            rhs.dummy_node.next_->prev_ = &dummy_node;
            size_ = rhs.size_;
            rhs.dummy_node.relink(&rhs.dummy_node, &rhs.dummy_node);
            rhs.size_ = 0;
        }
        return *this;
    }

    // =========================
    // push/pop

    void push_back(T* item) {
        auto tail = dummy_node.prev_;
        auto node = GetNode(item);
        if (node->next_ != nullptr || node->prev_ != nullptr) {
            return; // 已经在某链表中了！
        }

        tail->next_ = node;
        node->relink(tail, &dummy_node);
        dummy_node.prev_ = node;
        size_++;
    }
    void push_front(T* item) {
        auto head = dummy_node.next_;
        auto node = GetNode(item);
        if (node->next_ != nullptr || node->prev_ != nullptr) {
            return; // 已经在某链表中了！
        }

        head->prev_ = node;
        node->relink(&dummy_node, head);
        dummy_node.next_ = node;
        size_++;
    }
    
    void pop_back() {
        if (empty()) {
            return; // do nothing
        }
        auto tail = dummy_node.prev_;
        auto pre_tail = tail->prev_;

        // 清空链表节点
        tail->relink(nullptr, nullptr);

        dummy_node.prev_ = pre_tail;
        pre_tail->next_ = &dummy_node;
        size_--;
    }
    void pop_front() {
        if (empty()) {
            return;  // do nothing
        }
        auto head = dummy_node.next_;
        auto post_head = head->next_;

        // 清空链表节点
        head->relink(nullptr, nullptr);

        dummy_node.next_ = post_head;
        post_head->prev_ = &dummy_node;
        size_--;
    }

    // ===============================
    // insert/erase

    // 删除某个节点，返回下一个可用的迭代器
    iterator erase(iterator pos) {
        if (pos == end()) {
            return end(); // 不要删除end()
        }

        auto node = pos.pos_;
        auto pre_node = node->prev_;
        auto post_node = node->next_;

        // 清空
        node->relink(nullptr, nullptr);

        pre_node->next_ = post_node;
        post_node->prev_ = pre_node;
        size_--;

        return iterator(post_node);
    }

    // 插入的话，我们选择在pos节点前插入节点item，就像普通的链表那样
    void insert(iterator pos, T* item) {
        auto node = GetNode(item);

        // 检查是否在链表中
        if (node->next_ != nullptr || node->prev_ != nullptr) {
            return; // 已经在某链表中了！
        }

        auto pos_node = pos.pos_;
        auto pre_pos = pos_node->prev_;

        node->relink(pre_pos, pos_node);
        pre_pos->next_ = node;
        pos_node->prev_ = node;
        size_++;
    }

    // ===============================
    // swap/clear

    void clear() {
        // 清空所有节点
        for (auto it = begin(); it != end(); ) {
            it = erase(it);
        }
    }
    void swap(intrusive_list& rhs) noexcept {
        auto rhs_next = rhs.dummy_node.next_;
        auto rhs_prev = rhs.dummy_node.prev_;
        auto this_next = dummy_node.next_;
        auto this_prev = dummy_node.prev_;
        std::swap(rhs.dummy_node, dummy_node);
        std::swap(size_, rhs.size_);

        rhs_next->prev_ = &rhs.dummy_node;
        rhs_prev->next_ = &rhs.dummy_node;
        this_next->prev_ = &dummy_node;
        this_prev->next_ = &dummy_node;
    }


    // ===============================
    // front/back

    T* front() noexcept {
        if (empty()) {
            return nullptr;
        }
        return GetObject(dummy_node.next_);
    }
    T* back() noexcept {
        if (empty()) {
            return nullptr;
        }
        return GetObject(dummy_node.prev_);
    }
    
    // ==============================
    // others

    // 是否为空？
    bool empty() const {
        return dummy_node.next_ == &dummy_node;
    }

    // 这里我们维护一个链表长度的变量以求实现O(1)时间复杂度
    size_t size() const {
        return size_;
    }

    // ===============================
    // iterators

    // begin/end
    iterator begin() {
        return iterator(dummy_node.next_);
    }
    iterator end() {
        return iterator(&dummy_node);
    }
    // for const
    const_iterator begin() const {
        return const_iterator(dummy_node.next_);
    }
    const_iterator end() const {
        return const_iterator(&dummy_node);
    }

    // cbegin/cend
    const_iterator cbegin() const {
        return const_iterator(dummy_node.next_);
    }
    const_iterator cend() const {
        return const_iterator(&dummy_node);
    }

};

}

#endif