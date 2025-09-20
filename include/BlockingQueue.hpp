// BlockingQueue.hpp
// Header file for multi-thread blocking queue
// 用于多线程的阻塞队列的头文件

#ifndef TOYLIB_BLOCKING_QUEUE_HEADER
#define TOYLIB_BLOCKING_QUEUE_HEADER

#include <deque>
#include <mutex>
#include <condition_variable>

namespace toylib {

template <typename T>
class blocking_queue {
private:
    std::deque<T> dq_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool shutdown_;
public:
    blocking_queue() : shutdown_(false) {}
    ~blocking_queue() {
        shutdown();
    }

    // noncopyable
    blocking_queue(const blocking_queue&) = delete;
    blocking_queue& operator=(const blocking_queue&) = delete;

    template <typename U>
    void push(U&& item) {
        {
            std::unique_lock<std::mutex> l(mtx_);
            if (shutdown_) {
                throw std::runtime_error("queue is shutdown");
            }
            dq_.push_back(std::forward<U>(item));
        }
        cv_.notify_one();
    }
    template <typename... Args>
    void emplace(Args&&... args) {
        {
            std::unique_lock<std::mutex> l(mtx_);
            if (shutdown_) {
                throw std::runtime_error("queue is shutdown");
            }
            dq_.emplace_back(std::forward<Args>(args)...);
        }
        cv_.notify_one();
    }

    // @message If the queue shutdowns with thread blocking in pop(), this will throw a runtime_error.
    T pop() {
        std::unique_lock<std::mutex> l(mtx_);
        while (dq_.empty() && !shutdown_) {
            cv_.wait(l);
        }
        if (shutdown_) {
            throw std::runtime_error("queue is shutdown");
        }
        T item = std::move(dq_.front());
        dq_.pop_front();
        return item;
    }
    bool pop(T& out) {
        std::unique_lock<std::mutex> l(mtx_);
        while (dq_.empty() && !shutdown_) {
            cv_.wait(l);
        }
        if (shutdown_) {
            return false;
        }
        out = std::move(dq_.front());
        dq_.pop_front();
        return true;
    }

    // try pop
    bool try_pop(T& out) {
        std::unique_lock<std::mutex> l(mtx_);
        if (dq_.empty() || shutdown_) {
            return false;
        }
        out = std::move(dq_.front());
        dq_.pop_front();
        return true;
    }

    // bulk operations
    template <typename It>
    size_t bulk_try_pop(size_t max_attempt, It inserter) {
        std::unique_lock<std::mutex> l(mtx_);
        if (shutdown_) {
            return 0;
        }
        size_t cnt = 0;
        for (; cnt < max_attempt; ++cnt) {
            if (dq_.empty()) {
                break;
            }
            *inserter = std::move(dq_.front());
            dq_.pop_front();
        }
        return cnt;
    }

    template <typename It>
    void bulk_push(It begin, It end) {
        {
            std::unique_lock<std::mutex> l(mtx_);
            if (shutdown_) {
                throw std::runtime_error("queue is shutdown");
            }
            for (auto it = begin; it != end; ++it) {
                dq_.push_back(*it);
            }
        }
        cv_.notify_all(); // wake up all waiters
    }
    
    void shutdown() {
        {
            std::unique_lock<std::mutex> l(mtx_);
            shutdown_ = true;
        }
        cv_.notify_all();
    }


};

}
#endif