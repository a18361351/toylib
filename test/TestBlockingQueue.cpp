#include "../include/BlockingQueue.hpp"
#include "../include/ToyTest.hpp"
#include <atomic>
#include <iostream>
#include <vector>
#include <thread>

using namespace toylib;

bool TestBlockingQueue_SanityTest() {
    blocking_queue<int> bq;
    // push/pop
    int i = 0;
    bq.push(i);
    TOYTEST_ASSERT_EQ(bq.pop(), i, "popped value mismatch");

    bq.push(1);
    bq.push(2);
    TOYTEST_ASSERT_EQ(bq.pop(), 1, "popped value mismatch");
    
    int out;
    TOYTEST_ASSERT(bq.try_pop(out), "try_pop should succeed when there are items");
    TOYTEST_ASSERT_EQ(out, 2, "try_pop value mismatch");

    // bulk ops
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

    // emplace
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

bool TestBlockingQueue_BlockingTest() {
    blocking_queue<int> bq;
    std::thread t1([&bq] {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        bq.push(12345);
    });
    
    TOYTEST_ASSERT_EQ(bq.pop(), 12345, "popped value mismatch");
    t1.join();

    std::thread t2([&bq] {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        bq.shutdown();
    });
    
    TOYTEST_THROW(bq.pop(), "pop should throw after shutdown");

    t2.join();

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