#include "../include/BlockingQueue.hpp"
#include "../include/ToyTest.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <iterator>
#include <vector>
#include <thread>

using namespace toylib;

/**
 * template <typename U> void push(U&& item) (1)
 * template <typename... Args> void emplace(Args&&... args) (2)
 * T pop(); (3)
 * bool pop(T& out); (4)
 * template <typename Rep, typename Period = std::ratio<1>> 
 * bool pop_wait_for(T& out, const std::chrono::duration<Rep, Period>& duration) (5)
 * template <typename Clock>
 * bool pop_wait_until(T& out, const std::chrono::time_point<Clock> tp) (6)
 * bool try_pop(T& out) (7)
 * template <typename It> size_t bulk_try_pop(size_t max_attempt, It inserter) (8)
 * template <typename It> void bulk_push(It begin, It end, bool notify_all = false) (9)
 * void shutdown() (10)
 * bool is_shutdown() (11)
 * bool empty() (12)
 * size_t size() (13)
 */

bool TestBlockingQueue_SanityTest() {
    blocking_queue<int> bq;
    // push/pop: (1)(3)(4)
    // empty/size: (12)(13)
    TOYTEST_ASSERT(bq.empty(), "newly constructed queue should be empty");
    TOYTEST_ASSERT_EQ(bq.size(), 0, "newly constructed queue size should be 0");

    int i = 0;
    bq.push(i);
    TOYTEST_ASSERT(!bq.empty(), "queue should not be empty after push");
    TOYTEST_ASSERT_EQ(bq.size(), 1, "queue size should be 1 after one push");

    TOYTEST_ASSERT_EQ(bq.pop(), i, "popped value mismatch");
    TOYTEST_ASSERT(bq.empty(), "queue should be empty after pop");
    TOYTEST_ASSERT_EQ(bq.size(), 0, "queue size should be 0 after pop");

    bq.push(1);
    TOYTEST_ASSERT_EQ(bq.size(), 1, "queue size should be 1 after push");

    bq.push(2);
    TOYTEST_ASSERT_EQ(bq.size(), 2, "queue size should be 2 after second push");

    int val;
    TOYTEST_ASSERT(bq.pop(val), "pop() should return true if not empty")
    TOYTEST_ASSERT_EQ(bq.size(), 1, "queue size should be 1 after pop");
    TOYTEST_ASSERT_EQ(val, 1, "popped value mismatch");
    
    // try_pop (7)
    int out;
    TOYTEST_ASSERT(bq.try_pop(out), "try_pop should succeed when there are items");
    TOYTEST_ASSERT_EQ(out, 2, "try_pop value mismatch");

    TOYTEST_ASSERT_EQ(bq.try_pop(out), false, "try_pop should fail when queue is empty");

    // wait ops (6)(7)
    bq.push(1);
    
    // should not block
    TOYTEST_ASSERT(bq.pop_wait_for(out, std::chrono::milliseconds(1000)), "pop_wait_for should succeed when there are items");
    TOYTEST_ASSERT_EQ(out, 1, "pop_wait_for value mismatch");

    bq.push(2);
    TOYTEST_ASSERT(bq.pop_wait_until(out, std::chrono::steady_clock::now() + std::chrono::seconds(1)), "pop_wait_until should succeed when there are items");
    TOYTEST_ASSERT_EQ(out, 2, "pop_wait_until value mismatch");

    // bulk ops (8)(9)
    std::vector<int> inp = {3, 4, 5, 6, 7, 8};
    bq.bulk_push(inp.begin(), inp.end());

    std::vector<int> outv;
    std::vector<int> except = {3, 4, 5, 6, 7, 8};
    auto count = bq.bulk_try_pop(2, std::back_inserter(outv));
    TOYTEST_ASSERT_EQ(count, 2, "bulk_try_pop return value mismatch");
    TOYTEST_ASSERT_EQ(outv.size(), 2, "bulk_try_pop out size mismatch");
    for (int i = 0; i < count; i++) {
        TOYTEST_ASSERT_EQ(outv[i], except[i], "bulk_try_pop value mismatch");
    }

    count = bq.bulk_try_pop(10, std::back_inserter(outv));
    TOYTEST_ASSERT_EQ(count, 4, "bulk_try_pop return value mismatch");
    TOYTEST_ASSERT_EQ(outv.size(), 6, "bulk_try_pop out size mismatch");
    for (int i = 0; i < count; i++) {
        TOYTEST_ASSERT_EQ(outv[i + 2], except[i + 2], "bulk_try_pop value mismatch");
    }

    count = bq.bulk_try_pop(10, std::back_inserter(outv));
    TOYTEST_ASSERT_EQ(count, 0, "bulk_try_pop return value mismatch on empty queue");

    // shutdown/is_shutdown (10)(11)
    TOYTEST_ASSERT_EQ(bq.is_shutdown(), false, "is_shutdown should return false before shutdown");
    bq.shutdown();
    TOYTEST_ASSERT_EQ(bq.is_shutdown(), true, "is_shutdown should return true after shutdown");

    TOYTEST_THROW(bq.push(10), "push should throw after shutdown");
    TOYTEST_THROW(bq.pop(), "pop should throw after shutdown");
    TOYTEST_ASSERT(!bq.pop(val), "pop should return false after shutdown on empty queue");
    TOYTEST_ASSERT(!bq.pop_wait_for(val, std::chrono::milliseconds(100)), "pop_wait_for should return false after shutdown on empty queue");
    TOYTEST_ASSERT(!bq.pop_wait_until(val, std::chrono::steady_clock::now() + std::chrono::milliseconds(100)), "pop_wait_until should return false after shutdown on empty queue");
    TOYTEST_ASSERT(!bq.try_pop(val), "try_pop should return false after shutdown on empty queue");
    count = bq.bulk_try_pop(10, std::back_inserter(outv));
    TOYTEST_ASSERT_EQ(count, 0, "bulk_try_pop should return 0 after shutdown on empty queue");
    TOYTEST_THROW(bq.bulk_push(inp.begin(), inp.end()), "bulk_push should throw after shutdown");

    // emplace (2)
    struct TestNode {
        int x;
        int y;
        int* z;
        TestNode(int* zbound) : z(zbound) {
            (*z) = 1;
        }
        ~TestNode() {
            (*z) = -1;
        }
    };
    int flag = 0;
    blocking_queue<TestNode> bq2;
    bq2.emplace(&flag);
    TOYTEST_ASSERT(flag == 1, "emplace constructor not called");
    int tmp_flag;
    TestNode tmp(&tmp_flag);
    bq2.pop(tmp);
    TOYTEST_ASSERT(flag == -1, "destructor not called after pop");

    return true;
}

// blocking
bool TestBlockingQueue_BlockingTest() {
    {
        blocking_queue<int> bq;
        std::thread t1([&bq] {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            bq.push(12345);
        });
        
        TOYTEST_ASSERT_EQ(bq.pop(), 12345, "popped value mismatch"); // wait until push
        t1.join();

        std::thread t2([&bq] {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            bq.shutdown();
        });
        
        TOYTEST_THROW(bq.pop(), "pop should throw after shutdown"); // wait until shutdown

        t2.join();
    }
    {
        blocking_queue<int> bq;
        int val;
        std::thread t1([&bq] {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            bq.push(12345);
        });
        
        TOYTEST_ASSERT(bq.pop(val), "pop should return true if succeed"); // wait until push
        TOYTEST_ASSERT_EQ(val, 12345, "popped value mismatch");
        t1.join();

        std::thread t2([&bq] {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            bq.shutdown();
        });
        
        TOYTEST_ASSERT(!bq.pop(val), "pop should return false after shutdown"); // wait until shutdown
        
        t2.join();
    }

    return true;
}

bool TestBlockingQueue_WaitOpsTest() {
    blocking_queue<int> bq;
    int val;
    // timeout test
    // if this block forever then implementation is incorrect
    TOYTEST_ASSERT(!bq.pop_wait_for(val, std::chrono::milliseconds(500)), "pop_wait_for should timeout on empty queue");
    TOYTEST_ASSERT(!bq.pop_wait_until(val, std::chrono::steady_clock::now() + std::chrono::milliseconds(500)), "pop_wait_until should timeout on empty queue");

    bq.push(1);
    bq.push(2);

    TOYTEST_ASSERT(bq.pop_wait_for(val, std::chrono::milliseconds(500)), "pop_wait_for should succeed when there are items");
    TOYTEST_ASSERT_EQ(val, 1, "pop_wait_for value mismatch");
    TOYTEST_ASSERT(bq.pop_wait_until(val, std::chrono::steady_clock::now() + std::chrono::milliseconds(500)), "pop_wait_until should succeed when there are items");
    TOYTEST_ASSERT_EQ(val, 2, "pop_wait_until value mismatch");

    return true;

}

bool TestBlockingQueue_ConcurrentTest() {
    blocking_queue<int> bq;
    std::atomic<int> wcount{0};
    std::atomic<int> rcount{0};
    std::atomic<int> sentinelcount{0};
    std::atomic<bool> start{false};
    const int writer_count = 8;
    const int reader_count = 8;
    auto writer = [&bq, &wcount, &start](int tid, int thread_total) -> bool {
        while (!start.load(std::memory_order_relaxed));
        for (int i = tid; i < 5000000; i += thread_total) {
            bq.push(i);
            wcount.fetch_add(1, std::memory_order_relaxed);
        }
        return true;
    };
    auto reader = [&bq, &rcount, &start, &sentinelcount]() -> bool {
        while (!start.load(std::memory_order_relaxed));
        int val;
        while (rcount.load(std::memory_order_relaxed) < 5000000) {
            if (bq.pop(val)) {
                if (val != -1) {
                    rcount.fetch_add(1, std::memory_order_relaxed);
                } else {
                    sentinelcount.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
            }
        }
        return true;
    };

    std::vector<std::thread> writers;
    std::vector<std::thread> readers;
    for (int i = 0; i < writer_count; i++) {
        writers.emplace_back(writer, i, writer_count);
    }
    for (int i = 0; i < reader_count; i++) {
        readers.emplace_back(reader);
    }
    start.store(true, std::memory_order_relaxed);

    for (auto& t : writers) {
        t.join();
    }
    for (int i = 0; i < reader_count; i++) {
        bq.push(-1); // sentinel to wake up readers
    }
    for (auto& t : readers) {
        t.join();
    }

    TOYTEST_ASSERT_EQ(wcount.load(), 5000000, "total pushed count mismatch");
    TOYTEST_ASSERT_EQ(rcount.load(), 5000000, "total popped count mismatch");

    // there are still reader_count - sentinelcount item in queue
    for (int i = 0; i < reader_count - sentinelcount.load(); i++) {
        int tmp;
        TOYTEST_ASSERT(bq.pop(tmp), "pop failed");
        TOYTEST_ASSERT_EQ(tmp, -1, "sentinel value mismatch");
    }

    int tmp;
    TOYTEST_ASSERT(!bq.try_pop(tmp), "queue should be empty");
    
    return true;
}


int main() {
    std::vector<std::string> passed, failed;
    
    RUN_TEST("BlockingQueue Sanity Test", TestBlockingQueue_SanityTest, passed, failed);
    RUN_TEST("BlockingQueue Blocking Test", TestBlockingQueue_BlockingTest, passed, failed);
    RUN_TEST("BlockingQueue Wait Ops Test", TestBlockingQueue_WaitOpsTest, passed, failed);
    RUN_TEST_TIMER("BlockingQueue Concurrent Test", TestBlockingQueue_ConcurrentTest, passed, failed);
    
    if (failed.empty()) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests failed!" << std::endl;
        std::cout << "Passed tests: ";
        for (const auto& name : passed) {
            std::cout << name << " ";
        }
        std::cout << std::endl;

        std::cout << "Failed tests: ";
        for (const auto& name : failed) {
            std::cout << name << " ";
        }
        std::cout << std::endl;

        return 1;
    }

}