// BlockingQueue.hpp
// Header file for multi-thread blocking queue
// 用于多线程的阻塞队列的头文件

#ifndef TOYLIB_BLOCKING_QUEUE_HEADER
#define TOYLIB_BLOCKING_QUEUE_HEADER

#include <deque>
#include <mutex>
#include <condition_variable>

template <typename T>
class BlockingQueue {
private:
    std::deque<T> dq_;
    std::mutex mtx_;
    std::condition_variable cv_;
public:
    template <typename U>
    void push(U&& item) {
        {
            std::unique_lock l(mtx_);
            dq_.emplace_back(std::forward<U>(item));
        }
        cv_.notify_one();
    }
    T pop() {
        std::unique_lock l(mtx_);
        while (dq_.empty()) {
            cv_.wait(l);
        }
        T item = std::move(dq_.front());
        dq_.pop_front();
        return item;
    }
    void pop(T& out) {
        std::unique_lock l(mtx_);
        while (dq_.empty()) {
            cv_.wait(l);
        }
        out = std::move(dq_.front());
        dq_.pop_front();
    }

    
};

#endif