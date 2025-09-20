// RingBuffer.hpp
// Header file for simple ring buffer
// 简单环形缓冲区头文件

#ifndef TOYLIB_RING_BUFFER_HEADER
#define TOYLIB_RING_BUFFER_HEADER

#include <atomic>
#include <memory>
#include <cstddef>

namespace toylib {

// Adapted from cameron314/readerwriterqueue
template<typename U>
static inline char* align_for(char* ptr)
{
    const std::size_t alignment = std::alignment_of<U>::value;
    return ptr + (alignment - (reinterpret_cast<uintptr_t>(ptr) % alignment)) % alignment;
}

// Cache line width
#if defined(__cpp_lib_hardware_interference_size) // since C++17
constexpr size_t DEFAULT_CACHE_LINE_WIDTH = std::hardware_destructive_interference_size;
#else
constexpr size_t DEFAULT_CACHE_LINE_WIDTH = 64;
#endif
// A lock-free fixed-size ring buffer queue
// This implement is SPSC-only: one producer and one consumer at most.
template <typename T>
struct ring_buffer_spsc {
private:
    alignas(DEFAULT_CACHE_LINE_WIDTH) std::atomic<size_t> head_;    // avoid false sharing
    alignas(DEFAULT_CACHE_LINE_WIDTH) std::atomic<size_t> tail_;
    // raw buffer and aligned data pointer
    std::unique_ptr<char[]> raw_;
    char* data_;
    size_t size_;

public:
    // @brief fixed size ring buffer constructor
    explicit ring_buffer_spsc(size_t size) : head_(0), tail_(0), size_(size + 1) {  // preserve one slot for full/empty flag
        raw_ = std::make_unique<char[]>(sizeof(T) * (size + 1) + std::alignment_of<T>::value - 1);
        if (!raw_) {
            throw std::bad_alloc();
        }
        data_ = align_for<T>(raw_.get());
    }

    ~ring_buffer_spsc() {
        // destroy all elements
        while (!empty()) {
            T* pos = reinterpret_cast<T*>(data_ + tail_.load(std::memory_order_relaxed) * sizeof(T));
            pos->~T();
            tail_.store((tail_.load(std::memory_order_relaxed) + 1) % size_, std::memory_order_relaxed);
        }
    }

    // @brief Pop an item from ring buffer
    // @param out Output parameter to store the popped item
    // @return True if success, false if buffer is empty
    bool pop(T& out) {
        size_t cur_tail = tail_.load(std::memory_order_relaxed);
        if (cur_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        T* pos = reinterpret_cast<T*>(data_ + cur_tail * sizeof(T));
        out = std::move(*pos);
        pos->~T();

        tail_.store((cur_tail + 1) % size_, std::memory_order_release);
        return true;
    }

    // @brief Push an item into ring buffer
    // @param item Item to enqueue
    // @return True if success, false if buffer is full
    template <typename U>   // Support perfect forwarding
    bool push(U&& item) {
        size_t cur_head = head_.load(std::memory_order_relaxed);
        size_t next_head = (cur_head + 1) % size_;
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        
        // copy or move
        T* pos = reinterpret_cast<T*>(data_ + cur_head * sizeof(T));
        ::new (pos) T(std::forward<U>(item));

        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // @brief Emplace an item in ring buffer
    // @return True if success, false if buffer is full
    template <typename... Args>
    bool emplace(Args&&... args) {
        size_t cur_head = head_.load(std::memory_order_relaxed);
        size_t next_head = (cur_head + 1) % size_;
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        
        // construct in place
        T* pos = reinterpret_cast<T*>(data_ + cur_head * sizeof(T));
        ::new (pos) T(std::forward<Args>(args)...);

        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // @brief This is inaccurate since there are concurrent modifications
    size_t size() const {
        return (head_ + size_ - tail_) % size_;
    }
    // @brief This is inaccurate since there are concurrent modifications
    bool empty() const {
        return head_ == tail_;
    }
    // @brief This is inaccurate since there are concurrent modifications
    bool full() const {
        return (head_ + 1) % size_ == tail_;
    }
};
    
}

#endif