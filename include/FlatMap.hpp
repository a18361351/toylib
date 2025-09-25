// FlatMap.hpp
// Header file for flat map

#ifndef TOYLIB_FLATMAP_HEADER
#define TOYLIB_FLATMAP_HEADER


#include <cstddef>
#include <iterator>
#include <vector>

namespace toylib {


//  flat map container, like std::map but using sorted array as backend and elements are sorted in ascending order by default
//  Lookup is done by binary search, complexity is O(log n), though it's efficient thanks to cache locality
//  Insert, Delete's complexity is O(n) due to array shifting
//  Iteration is done by array traversal, complexity is O(n)
//  Suitable for scenarios where insertion and deletion are rare but lookup and iteration are frequent
//  This implement is not thread-safe
template <typename Key, typename Value, typename Compare = std::less<Key>>
class flat_map {
public:
    // Using Compare to judge two elem's equal
    // a != b -> comp_(a, b) || comp_(b, a)
    // a == b -> !(comp_(a, b) || comp_(b, a))

    // iterator definition
    // We can't modify element in a set by iterator, so we set them to const
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Key, Value>;   // We don't restrain value_type to be std::pair<const Key, Value> but user shouldn't modify key
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using size_type = size_t;
    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;
private:
    bool equal(const Key& a, const Key& b) const {
        return !(comp_(a, b) || comp_(b, a));
    }

    std::vector<value_type> data_;

    // binary search method in range [l, r)
    // returns the position of k in data_ or the first element greater than v
    size_t bin_impl(const Key& k, size_t l, size_t r) const {
        while (l < r) {
            size_t mid = l + (r - l) / 2;
            // data_[mid] < k
            if (comp_(data_[mid].first, k)) l = mid + 1;
            else r = mid;
        }
        return l; // l == r
    }
    Compare comp_;
public:
    flat_map() = default;
    ~flat_map() = default;

    // @brief count the number of elements matching key
    // @param key key to lookup
    // @return 1 if the element is found, 0 otherwise
    size_t count(const Key& key) const {
        auto pos = bin_impl(key, 0, data_.size());
        // if (pos >= data_.size() || (key != data_[pos])) return 0;
        // return 1;
        return (pos < data_.size() && equal(key, data_[pos].first)) ? 1 : 0;
    }

    // template <typename... Args>
    // std::pair<iterator, bool> emplace(Args&&... args) {

    // }

    // template <typename... Args>
    // iterator emplace_hint(const_iterator hint, Args&&... args) {

    // }

    // @brief insert one key
    // @param key key to insert
    // @return pair of {iterator to inserted or existing element, whether insertion took place}
    std::pair<iterator, bool> insert(const value_type& val) {
        auto pos = bin_impl(val.first, 0, data_.size());
        if (pos < data_.size() && equal(val.first, data_[pos].first)) {
            return {data_.begin() + pos, false};
        }
        data_.insert(data_.begin() + pos, val);
        return {data_.begin() + pos, true};
    }

    // @brief hinted insert
    // @param hint expected position to insert
    // @param val key to insert
    // @return Position of inserted or existing equal element
    iterator insert(const_iterator hint, const value_type& val) {
        // check whether this position(hint) matches:
        //  prev < val < hint
        if (data_.empty()) {
            return data_.insert(data_.begin(), val);
        }
        if (hint == cbegin()) {
            if ((comp_(val.first, hint->first))) {
                data_.insert(data_.begin(), val);
                return data_.begin();
            }
            // just a normal insert
            return insert(val).first;
        }
        if (hint == cend()) {
            if (comp_((hint - 1)->first, val.first)) {
                data_.push_back(val);
                return data_.end() - 1;
            }
            // just a normal insert
            return insert(val).first;
        }
        if (comp_((hint - 1)->first, val.first) && comp_(val.first, hint->first)) {
            return data_.insert(hint, val);
        }
        // normal insert
        return insert(val).first;
    }

    // @brief range insert
    template <typename InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    // @brief erase by key
    // @param key key to erase
    // @return 1 if the element is erased, 0 if not found
    size_t erase(const Key& key) {
        auto pos = bin_impl(key, 0, data_.size());
        if (pos >= data_.size() || !equal(key, data_[pos].first)) return 0;
        data_.erase(data_.begin() + pos);
        return 1;
    }

    // @brief erase by iterator
    // @return next iterator after erased one
    iterator erase(iterator pos) {
        return data_.erase(pos);
    }

    // @brief range erase
    // @return next iterator after last erased one
    iterator erase(iterator first, iterator last) {
        return data_.erase(first, last);
    }

    // @brief find elem
    // @return element's iterator or end() if not found
    iterator find(const Key& key) {
        auto pos = bin_impl(key, 0, data_.size());
        if (pos == data_.size() || !equal(key, data_[pos].first)) return end();
        return data_.begin() + pos;
    }

    Value& at(const Key& key) {
        auto pos = bin_impl(key, 0, data_.size());
        if (pos >= data_.size() || !equal(key, data_[pos].first)) {
            throw std::out_of_range("flat_map::at: key not found");
        } 
        return data_[pos].second;
    }

    Value& operator[](const Key& key) {
        auto pos = bin_impl(key, 0, data_.size());
        if (pos >= data_.size() || !equal(key, data_[pos].first)) {
            auto it = data_.emplace(data_.begin() + pos, key, Value());
            return it->second;
        } 
        return data_[pos].second;
    }

    // begins/ends
    iterator begin() {
        return data_.begin();
    }

    iterator end() {
        return data_.end();
    }

    const_iterator begin() const {
        return data_.begin();
    }

    const_iterator end() const {
        return data_.end();    
    }

    const_iterator cbegin() const {
        return data_.cbegin();
    }

    const_iterator cend() const {
        return data_.cend();
    }
    
    // some other methods
    size_t size() const {
        return data_.size();
    }
    void clear() {
        data_.clear();
    }
    bool empty() const {
        return data_.empty();
    }
    void reserve(size_t n) {
        data_.reserve(n);
    }
    // std::vector<Key>& get_data() {
    //     return data_;
    // }
};
}

#endif