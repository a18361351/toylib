// RingBuffer.hpp
// Header file for simple ring buffer
// 简单环形缓冲区头文件


#ifndef TOYLIB_RING_BUFFER_HEADER
#define TOYLIB_RING_BUFFER_HEADER

#include <atomic>
#include <vector>
#include <cstddef>

// 缓存行长度
#if defined(__cpp_lib_hardware_interference_size)
constexpr size_t DEFAULT_CACHE_LINE_WIDTH = std::hardware_destructive_interference_size;
#else
constexpr size_t DEFAULT_CACHE_LINE_WIDTH = 64;
#endif
// 无锁结构的固定大小环形缓冲区队列
// 注：该缓冲区是SPSC限定的，即最多有一个生产者和一个消费者
template <typename T>
struct ring_buffer_spsc {
private:
    alignas(DEFAULT_CACHE_LINE_WIDTH) std::atomic<size_t> head_;    // 将两个变量放入不同缓存行，避免false sharing
    alignas(DEFAULT_CACHE_LINE_WIDTH) std::atomic<size_t> tail_;
    std::vector<T> data_;
    
public:
    // @brief size表示队列中能够存放的对象数量
    explicit ring_buffer_spsc(size_t size) : head_(0), tail_(0), data_(size + 1) {}

    // @brief 从缓冲区中取出一个对象
    // @param out 用于存放取出的对象
    // @return 成功返回true，失败（缓冲区空）返回false
    bool pop(T& out) {
        size_t cur_tail = tail_.load(std::memory_order_relaxed);
        if (cur_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        out = data_[cur_tail];
        tail_.store((cur_tail + 1) % data_.size(), std::memory_order_release);
        return true;
    }

    // @brief 向缓冲区中放入一个对象
    // @param item 对象
    // @return 成功返回true，失败（缓冲区满）返回false
    template <typename U>   // 这里我们使用了模板以支持完美转发！
    bool push(U&& item) {
        size_t cur_head = head_.load(std::memory_order_relaxed);
        size_t next_head = (cur_head + 1) % data_.size();
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        data_[cur_head] = std::forward<U>(item);
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // 注意size(), empty()和full()的值在并发条件下可能不是最新的，仅供参考

    // @brief 获取当前缓冲区中对象的数量
    // @return 对象数量
    size_t size() const {
        return (head_ + data_.size() - tail_) % data_.size();
    }
    // @brief 判断缓冲区是否为空
    bool empty() const {
        return head_ == tail_;
    }
    // @brief 判断缓冲区是否已满
    bool full() const {
        return (tail_ + 1) % data_.size() == head_;
    }
};

#endif