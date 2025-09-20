#include "../include/FixedMemPool.hpp"
#include "../include/ToyTest.hpp"
#include <iostream>
#include <thread>
#include <atomic>

using toylib::fixed_mem_pool;

bool TestMemPool_SimpleTest() {
    fixed_mem_pool<8, 32> pool(1);

    auto obj = pool.alloc_as<uint64_t>();
    (*obj) = 0x1234;

    TOYTEST_ASSERT(*obj == 0x1234, "obj value mismatch");

    pool.free(obj);
    return true;
}

bool TestMemPool_SanityTest() {
    fixed_mem_pool<8, 32> pool(1);

    // 分配测试
    auto p1 = pool.alloc(false);
    auto p2 = pool.alloc(false);
    auto p3 = pool.alloc(false);
    auto p4 = pool.alloc(false);
    auto p5 = pool.alloc(false);
    TOYTEST_ASSERT(p1 && p2 && p3 && p4, "mempool should be able to alloc 4 items");
    TOYTEST_ASSERT(!p5, "p5 should be null");

    // 释放测试，然后再次获取，两个对象地址应该相同
    pool.free(p3);
    auto p6 = pool.alloc(false);
    TOYTEST_ASSERT(p6 == p3, "p6 should be equal to p3");

    // 全部释放再分配
    pool.free(p1);
    pool.free(p2);
    pool.free(p4);
    pool.free(p6);

    p1 = pool.alloc(false);
    p2 = pool.alloc(false);
    p3 = pool.alloc(false);
    p4 = pool.alloc(false);
    p5 = pool.alloc(false);

    TOYTEST_ASSERT(p1 && p2 && p3 && p4, "mempool should be able to alloc 4 items");
    TOYTEST_ASSERT(!p5, "p5 should be null");

    pool.free(p1);
    // 测试alloc_as<T>
    struct TestStruct {
        char data[8];
    };

    TestStruct* t1 = pool.alloc_as<TestStruct>();
    TOYTEST_ASSERT(t1 == reinterpret_cast<TestStruct*>(p1), "t1 should be equal to p1");
    for (int i = 0; i < 8; i++) {
        t1->data[i] = static_cast<char>(i);
    }
    pool.free(reinterpret_cast<char*>(t1));

    return true;
}

bool TestMemPool_AllocTest() {
    fixed_mem_pool<8, 32> pool(1);

    // 分配满
    auto p1 = pool.alloc(false);
    auto p2 = pool.alloc(false);
    auto p3 = pool.alloc(false);
    auto p4 = pool.alloc(false);
    TOYTEST_ASSERT(p1 && p2 && p3 && p4, "first chunk nodes should be valid");

    auto p4_none = pool.alloc(false);
    TOYTEST_ASSERT(!p4_none, "pool should be empty now");

    // 自动扩容
    auto p5 = pool.alloc(true);
    TOYTEST_ASSERT(p5, "chunk allocation is not triggered");

    // 继续分配
    auto p6 = pool.alloc(false);
    auto p7 = pool.alloc(false);
    auto p8 = pool.alloc(false);
    TOYTEST_ASSERT(p6 && p7 && p8, "newly allocated chunk is not correctly set");

    auto p9_none = pool.alloc(false);
    TOYTEST_ASSERT(!p9_none, "pool should be empty now");

    return true;
}

bool TestMemPool_NonfixedTest() {
    fixed_mem_pool<8, 36> pool(1);
    // 分配满
    auto p1 = pool.alloc(false);
    auto p2 = pool.alloc(false);
    auto p3 = pool.alloc(false);
    auto p4 = pool.alloc(false);
    TOYTEST_ASSERT(p1 && p2 && p3 && p4, "mempool should be able to alloc 4 items");

    auto p4_none = pool.alloc(false);
    TOYTEST_ASSERT(!p4_none, "pool should be empty now");

    // 自动扩容
    auto p5 = pool.alloc(true);
    TOYTEST_ASSERT(p5, "chunk allocation should be triggered");

    // 继续分配
    auto p6 = pool.alloc(false);
    auto p7 = pool.alloc(false);
    auto p8 = pool.alloc(false);
    TOYTEST_ASSERT(p6 && p7 && p8, "newly allocated chunk is not correctly set");

    return true;
}

bool TestMemPool_ConcurrentTest() {
    fixed_mem_pool<8, 4096> pool(4);
    int writer = 16;    // 4096 / 8 / 16 == 32
    int round_max = 1000;
    int item_hold = 32;
    std::atomic_bool start{false};
    auto write_fn = [=, &pool, &start](int tid) -> bool {
        bool good = true;
        while (!start);
        for (int round = 0; round < round_max; round++) {
            std::vector<char*> ptrs;
            ptrs.reserve(item_hold);
            for (int i = 0; i < item_hold; i++) {
                auto ptr = pool.alloc(false);
                if (!ptr) {
                    i--;
                    continue;
                }
                ptrs.push_back(ptr);
                // 写入数据，将获得的地址
                uint64_t* p = reinterpret_cast<uint64_t*>(ptr);
                *p = tid;
            }
            for (int i = 0; i < item_hold; i++) {
                // 检查一下
                uint64_t* p = reinterpret_cast<uint64_t*>(ptrs[i]);
                if (*p != tid) {
                    if (good) {
                        std::cerr << "Data corrupted in writer " << tid << std::endl;
                    }
                    good = false;
                }
                // 释放回去
                pool.free(ptrs[i]);
            }
        }
        return good;
    };
    std::vector<std::thread> writers;
    std::vector<char> results(writer, 0);
    for (int i = 0; i < writer; i++) {
        writers.emplace_back([&results, &write_fn, i]() {
            results[i] = write_fn(i + 1);
        });
    }
    start.store(true);
    for (auto& t : writers) {
        t.join();
    }
    for (int i = 0; i < writer; i++) {
        TOYTEST_ASSERT(results[i], "writer " + std::to_string(i) + " failed");
    }
    return true;
}

bool TestMemPool_ExpandPressureTest() {
    fixed_mem_pool<8, 32> pool(1);
    int writer = 16;
    int item_hold = 64; // 4096 / 4 / 16 = 64
    std::atomic_bool start{false};
    std::atomic_int done{writer};
    auto write_fn = [=, &pool, &start, &done](int tid) -> bool {
        bool good = true;
        while (!start);
        
        std::vector<char*> ptrs;
        ptrs.reserve(item_hold);
        for (int i = 0; i < item_hold; i++) {
            auto ptr = pool.alloc(true);
            if (!ptr) {
                i--;
                continue;
            }
            ptrs.push_back(ptr);
            // 写入数据，将获得的地址
            uint64_t* p = reinterpret_cast<uint64_t*>(ptr);
            *p = tid;
        }
        for (int i = 0; i < item_hold; i++) {
            // 检查一下
            uint64_t* p = reinterpret_cast<uint64_t*>(ptrs[i]);
            if (*p != tid) {
                if (good) {
                    std::cerr << "Data corrupted in writer " << tid << std::endl;
                }
                good = false;
            }
        }
        done.fetch_sub(1);
        while (done.load() > 0) {
            std::this_thread::yield();
        }
        for (int i = 0; i < item_hold; i++) {
            pool.free(ptrs[i]);
        }
        return good;
        
    };
    std::vector<std::thread> writers;
    std::vector<char> results(writer, 0);
    for (int i = 0; i < writer; i++) {
        writers.emplace_back([&results, &write_fn, i]() {
            results[i] = write_fn(i + 1);
        });
    }
    start.store(true);
    for (auto& t : writers) {
        t.join();
    }
    for (int i = 0; i < writer; i++) {
        TOYTEST_ASSERT(results[i], "writer " + std::to_string(i) + " failed");
    }
    return true;
}

int main() {
    std::vector<std::string> passed, failed;
    RUN_TEST("MemPool_SimpleTest", TestMemPool_SimpleTest, passed, failed);
    RUN_TEST("MemPool_SanityTest", TestMemPool_SanityTest, passed, failed);
    RUN_TEST("MemPool_AllocTest", TestMemPool_AllocTest, passed, failed);
    RUN_TEST("MemPool_NonfixedTest", TestMemPool_NonfixedTest, passed, failed);
    RUN_TEST_TIMER("MemPool_ConcurrentTest", TestMemPool_ConcurrentTest, passed, failed);
    RUN_TEST_TIMER("MemPool_ExpandPressureTest", TestMemPool_ExpandPressureTest, passed, failed);

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