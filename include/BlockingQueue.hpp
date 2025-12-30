/**
 * @file blocking_queue.hpp
 * @brief 用于多线程的阻塞队列的头文件
*/

#ifndef TOYLIB_BLOCKING_QUEUE_HEADER
#define TOYLIB_BLOCKING_QUEUE_HEADER

#include <chrono>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace toylib {

/**
 * @class blocking_queue
 * @brief 用于多线程的阻塞队列
 *      这是一个支持MPMC(多生产者多消费者)的阻塞队列。支持多线程安全的入队出队操作。
 *  同时，队列本身也支持批量(bulk)操作，以及提供阻塞和非阻塞的接口。
 * @tparam T 队列中元素的类型，要求可移动
 */

template <typename T>
class blocking_queue {
private:
    std::deque<T> dq_;
    mutable std::mutex mtx_;
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

    /**
     * @brief 向队列尾部添加一个元素。
     *      当队列已关闭时，则抛出runtime_error异常。
     * @param item 要添加的元素
     * @throws std::runtime_error 当队列已关闭时
     * @note 线程安全的
     */
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

    /**
     * @brief 在队列尾部构造一个元素。
     *      当队列已关闭时，则抛出runtime_error异常。
     * @param args 用于构造元素的参数
     * @throws std::runtime_error 当队列已关闭时
     * @note 线程安全的
     */
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

    /**
     * @brief 从队列头部弹出一个元素并返回。
     *      当队列为空时，则阻塞等待直到队列非空或队列被关闭。
     *      当调用时队列已经关闭，或当阻塞等待时队列被关闭，则
     *      抛出runtime_error异常。
     * @return T 队列头部的元素
     * @throws std::runtime_error 当队列已关闭时
     * @note 线程安全的
     */
    T pop() {
        std::unique_lock<std::mutex> l(mtx_);
        // while (dq_.empty() && !shutdown_) {
        //     cv_.wait(l);
        // }
        cv_.wait(l, [&]() -> bool { return !dq_.empty() || shutdown_; });
        if (shutdown_) {
            throw std::runtime_error("queue is shutdown");
        }
        T item = std::move(dq_.front());
        dq_.pop_front();
        return item;
    }
    

    /**
     * @brief 从队列头部弹出一个元素，移动到out参数中。
     *      当队列为空时，则阻塞等待直到队列非空或队列被关闭。
     *      当调用时队列已经关闭，则立即返回false。
     * @param out 用于存储弹出元素的引用
     * @return true 成功弹出元素
     * @return false 队列为空或队列已关闭
     * @note 线程安全的
     */
    bool pop(T& out) {
        std::unique_lock<std::mutex> l(mtx_);
        // while (dq_.empty() && !shutdown_) {
        //     cv_.wait(l);
        // }
        cv_.wait(l, [&]() -> bool { return !dq_.empty() || shutdown_; });

        if (shutdown_) {
            return false;
        }
        out = std::move(dq_.front());
        dq_.pop_front();
        return true;
    }

    /**
     * @brief 尝试从队列头部弹出一个元素，移动到out参数中。
     *      当队列为空时，则阻塞等待直到队列非空、超时或队列被关闭。
     *      当调用时队列已经关闭，或当阻塞等待时队列被关闭，则
     *      返回false。
     * @param out 用于存储弹出元素的引用
     * @param duration 等待的最大时间长度
     * @return true 成功弹出元素
     * @return false 队列为空或队列已关闭或超时
     * @note 线程安全的
     */
    template <typename Rep, typename Period = std::ratio<1>>
    bool pop_wait_for(T& out, const std::chrono::duration<Rep, Period>& duration) {
        std::unique_lock<std::mutex> l(mtx_);
        bool ret = cv_.wait_for(l, duration, [&]() -> bool { return !dq_.empty() || shutdown_; });
        if (dq_.empty() || shutdown_) { // 可能timeout，被唤醒，或者整个队列shutdown
            return false;
        }
        out = std::move(dq_.front());
        dq_.pop_front();
        return true;
    }

    /**
     * @brief 尝试从队列头部弹出一个元素，移动到out参数中。
     *      当队列为空时，则阻塞等待直到队列非空、超时或队列被关闭。
     *      当调用时队列已经关闭，或当阻塞等待时队列被关闭，则
     *      返回false。
     * @param out 用于存储弹出元素的引用
     * @param tp 等待的绝对时间点
     * @return true 成功弹出元素
     * @return false 队列为空或队列已关闭或超时
     * @note 线程安全的
     */
    template <typename Clock>
    bool pop_wait_until(T& out, const std::chrono::time_point<Clock> tp) {
        std::unique_lock<std::mutex> l(mtx_);
        bool ret = cv_.wait_until(l, tp, [&]() -> bool { return !dq_.empty() || shutdown_; });
        if (dq_.empty() || shutdown_) { // 可能timeout，被唤醒，或者整个队列shutdown
            return false;
        }
        out = std::move(dq_.front());
        dq_.pop_front();
        return true;
    }
    
    /**
     * @brief 尝试从队列头部弹出一个元素，移动到out参数中。
     *      当队列为空或队列已经关闭时，则立即返回false。
     * @param out 用于存储弹出元素的引用
     * @return true 成功弹出元素
     * @return false 队列为空或队列已关闭
     * @note 线程安全的
     */
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
    /**
     * @brief 尝试从队列头部弹出最多max_attempt个元素，移动到inserter指向的位置。
     *      当队列为空或队列已经关闭时，则立即返回0。
     * @param max_attempt 最多尝试弹出的元素个数
     * @param inserter 用于存储弹出元素的迭代器，可以是插入迭代器
     * @return size_t 实际弹出的元素个数
     * @note 线程安全的
     */
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
            *(inserter++) = std::move(dq_.front());
            dq_.pop_front();
        }
        return cnt;
    }

    /**
     * @brief 批量将[begin, end)范围内的元素添加到队列尾部。
     *      当队列已关闭时，则抛出runtime_error异常。
     * @param begin 范围的起始迭代器
     * @param end 范围的结束迭代器
     * @throws std::runtime_error 当队列已关闭时
     * @note 线程安全的
     */
    template <typename It>
    void bulk_push(It begin, It end, bool notify_all = false) {
        {
            std::unique_lock<std::mutex> l(mtx_);
            if (shutdown_) {
                throw std::runtime_error("queue is shutdown");
            }
            for (auto it = begin; it != end; ++it) {
                dq_.push_back(*it);
            }
        }
        if (notify_all) {
            cv_.notify_all();
        } else {
            cv_.notify_one();
        }
    }
    
    /**
     * @brief 关闭队列，使所有等待的线程都能被唤醒。
     *      队列关闭后，所有等待的线程都将被唤醒，
     *      并抛出runtime_error异常。
     *       队列关闭后，无法再插入新元素，也无法获取
     *      剩余元素。
     */
    void shutdown() {
        {
            std::unique_lock<std::mutex> l(mtx_);
            if (shutdown_) {
                return;
            }
            shutdown_ = true;
        }
        cv_.notify_all();
    }

    /**
     * @brief 查询队列是否已关闭。
     * @return true 队列已关闭
     * @return false 队列未关闭
     * @note 线程安全的
     */
    bool is_shutdown() const {
        std::unique_lock<std::mutex> l(mtx_);
        return shutdown_;
    }

    /**
     * @brief 查询队列是否为空。
     * @return true 队列为空
     * @return false 队列非空
     * @note 线程安全的
     */
    bool empty() const {
        std::unique_lock<std::mutex> l(mtx_);
        return dq_.empty();
    }

    /**
     * @brief 查询队列当前元素个数。
     * @return size_t 队列当前元素个数
     * @note 线程安全的
     */
    size_t size() const {
        std::unique_lock<std::mutex> l(mtx_);
        return dq_.size();
    }

};

}
#endif