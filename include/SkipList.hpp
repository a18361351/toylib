#ifndef TOYLIB_SKIPLIST_HEADER
#define TOYLIB_SKIPLIST_HEADER

#include <stdexcept>
#include <array>
#include <vector>
#include <cstddef>
// #include <optional>  // unluckily we are a C++11 header library

namespace toylib {
    
template <typename Key, typename Value, size_t MaxLevel = 6, size_t Numerator = 1, size_t Denominator = 4, typename Compare = std::less<Key>>
class skip_list {
private:
    static constexpr float P = static_cast<float>(Numerator) / static_cast<float>(Denominator);
    struct skip_node {
        std::array<skip_node*, MaxLevel> next_;  // next pointers for each level
        size_t lvl_;    // current level of skip node
        std::pair<const Key, Value> data_;
        // Key key_;       // storaged key
        // Value val_;     // storaged value
        skip_node(size_t layer) : lvl_(0) {
            next_.fill(nullptr);
        }
        skip_node(size_t layer, const Key& k, const Value& v) : lvl_(0), data_{k, v} {
            next_.fill(nullptr);
        }
    };
    skip_node dummy_front_{MaxLevel};
    // skip_node dummy_tail_; // nullptr
    size_t size_{0};
    Compare comp_;

    bool equal(const Key& a, const Key& b) const {
        return !(comp_(a, b) || comp_(b, a));
    }

    // @brief Lookup implement 
    // @return element position where its next node's key is equal or first greater than k
    skip_node* lookup_impl(const Key& k) {
        size_t lvl = MaxLevel - 1;
        skip_node* cur = &dummy_front_;
        while (true) {
            skip_node* nxt = cur->next_[lvl];
            if (!nxt || !comp_(nxt->data_.first, k)) { // nxt >= k
                // go down
                if (lvl == 0) {
                    return cur;
                }
                --lvl;
            } else {
                // go forward
                cur = nxt;
            }
        }
        return cur;
    }

    // @brief Lookup implement for insertion and deletion
    // @return every previous element's position of every level
    // @note When we perform insertion or deletion on a skip-list,
    //      we not only need the previous one node, but also the 
    //      previous nodes on every level.
    //       For example, [0-lv3, 1-lv2, 2-lv1, 4-lv3], if we insert
    //      node [3-lv3] to the list, we need to change three nodes'
    //      next pointers: 0-lv3, 1-lv2, 2-lv1. So we chose to return
    //      a list of previous nodes to help our implementation.
    std::vector<skip_node*> modify_lookup_impl(const Key& k) {
        std::vector<skip_node*> prevs(MaxLevel, nullptr);
        size_t lvl = MaxLevel - 1;
        skip_node* cur = &dummy_front_;
        while (true) {
            skip_node* nxt = cur->next_[lvl];
            if (!nxt || !comp_(nxt->data_.first, k)) { // nxt >= k
                // go down
                prevs[lvl] = cur;
                if (lvl == 0) {
                    return prevs;
                }
                --lvl;
            } else {
                // go forward
                cur = nxt;
            }
        }
    }

    skip_node* generate_node(const Key& k, const Value& v) {
        skip_node* ret = new skip_node(MaxLevel, k, v);
        ret->lvl_ = 0;
        while (ret->lvl_ < MaxLevel - 1 && ((float)rand() / RAND_MAX) < P) {    // TODO(me): Better random engine
            ++ret->lvl_;
        }
        return ret;
    }

    void destroy_node(skip_node* node) {
        delete node;
    }

    // recursive destroy every node in the list
    void destroy_list() {
        skip_node* ptr = dummy_front_.next_[0];
        while (ptr) {
            skip_node* nxt = ptr->next_[0];
            destroy_node(ptr);
            ptr = nxt;
        }
    }
    
public:
    class iterator {
    private:
        skip_node* cur_;
        explicit iterator(skip_node* node) : cur_(node) {}
        friend class skip_list;
    public:
        iterator() : cur_(nullptr) {}
        iterator& operator++() {
            cur_ = cur_->next_[0];
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        bool operator==(const iterator& other) const {
            return cur_ == other.cur_;
        }
        bool operator!=(const iterator& other) const {
            return cur_ != other.cur_;
        }
        std::pair<const Key, Value>& operator*() {
            return cur_->data_;
        }
        std::pair<const Key, Value>* operator->() {
            return &(cur_->data_);
        }
    };

    skip_list() = default;
    ~skip_list() {
        destroy_list();
    }

    // no copy
    skip_list(const skip_list&) = delete;
    skip_list& operator=(const skip_list&) = delete;

    // move ops
    skip_list(skip_list&& other) noexcept {
        dummy_front_.next_ = other.dummy_front_.next_;
        size_ = other.size_;
        other.dummy_front_.next_.fill(nullptr);
        other.size_ = 0;
    }
    skip_list& operator=(skip_list&& other) noexcept {
        if (this != &other) {
            destroy_list();
            dummy_front_.next_ = other.dummy_front_.next_;
            size_ = other.size_;
            other.dummy_front_.next_.fill(nullptr);
            other.size_ = 0;
        }
        return *this;
    }

    std::pair<iterator, bool> insert(const Key& key, const Value& val) {
        std::vector<skip_node*> prevs = modify_lookup_impl(key);
        if (prevs[0]->next_[0] && equal(prevs[0]->next_[0]->data_.first, key)) { // key already exists
            iterator exists = iterator(prevs[0]->next_[0]);
            return {exists, false};
        }
        // the level is randomized
        skip_node* new_node = generate_node(key, val);
        for (size_t i = 0; i <= new_node->lvl_; i++) {
            skip_node* nxt = prevs[i]->next_[i];
            prevs[i]->next_[i] = new_node;
            new_node->next_[i] = nxt;
        }
        ++size_;
        return {iterator(new_node), true};
    }

    size_t erase(const Key& key) {
        std::vector<skip_node*> prevs = modify_lookup_impl(key);
        if (!prevs[0]->next_[0] || !equal(prevs[0]->next_[0]->data_.first, key)) { // key already exists
            return 0;
        }
        skip_node* target = prevs[0]->next_[0];
        for (size_t i = 0; i <= target->lvl_; i++) {
            prevs[i]->next_[i] = target->next_[i];
        }
        destroy_node(target);
        --size_;
        return 1;
    }

    Value& at(const Key& key) {
        skip_node* prev = lookup_impl(key);
        if (prev->next_[0] && equal(prev->next_[0]->data_.first, key)) {
            return prev->next_[0]->data_.second;
        }
        throw std::out_of_range("skip_list::at: key not found");
    }

    iterator find(const Key& key) {
        skip_node* prev = lookup_impl(key);
        if (prev->next_[0] && equal(prev->next_[0]->data_.first, key)) {
            return iterator(prev->next_[0]);
        }
        return end();
    }

    Value& operator[](const Key& key) {
        std::vector<skip_node*> prevs = modify_lookup_impl(key);
        if (prevs[0]->next_[0] == nullptr || !equal(prevs[0]->next_[0]->data_.first, key)) {
            // element not exist, insert default value
            skip_node* new_node = generate_node(key, Value());
            for (size_t i = 0; i <= new_node->lvl_; i++) {
                skip_node* nxt = prevs[i]->next_[i];
                prevs[i]->next_[i] = new_node;
                new_node->next_[i] = nxt;
            }
        }
        // return found value or newly initialized node's value
        return prevs[0]->next_[0]->data_.second;
    }

    iterator begin() {
        return iterator(dummy_front_.next_[0]);
    }
    iterator end() {
        return iterator(nullptr);
    }
    size_t size() const {
        return size_;
    }
    bool empty() const {
        return size_ == 0;
    }


};

}

#endif