#include "../include/RingBuffer.hpp"
#include "../include/ToyTest.hpp"
#include <iostream>
#include <vector>
#include <thread>

using namespace toylib;

bool TestRingBufferSPSC_SanityTest() {
    ring_buffer_spsc<int> rb(4);
    TOYTEST_ASSERT(rb.empty(), "ring buffer should be empty");

    TOYTEST_ASSERT(rb.push(0), "push failed");
    TOYTEST_ASSERT(!rb.empty(), "ring buffer empty() should be false");
    TOYTEST_ASSERT(rb.size() == 1, "ring buffer size should be 1");

    TOYTEST_ASSERT(rb.push(1), "push failed");
    TOYTEST_ASSERT(rb.push(2), "push failed");
    TOYTEST_ASSERT(rb.push(3), "push failed");
    TOYTEST_ASSERT(!rb.push(4), "push expected to fail when full");

    TOYTEST_ASSERT(rb.size() == 4, "ring buffer size should be 4");
    TOYTEST_ASSERT(rb.full(), "ring buffer should be full");

    int idx = 0;
    std::vector<int> expect = {0, 1, 2, 3};

    int x;
    while (rb.pop(x)) {
        TOYTEST_ASSERT(x == expect[idx++], "popped value mismatch");
    }
    TOYTEST_ASSERT(idx == expect.size(), "popped count mismatch");
    TOYTEST_ASSERT(rb.empty(), "ring buffer should be empty after pop all");

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
    ring_buffer_spsc<TestNode> rb2(2);
    
    rb2.emplace(&flag);
    TOYTEST_ASSERT(flag == 1, "emplace constructor not called");

    int tmp_flag;
    TestNode tmp(&tmp_flag);
    rb2.pop(tmp);
    TOYTEST_ASSERT(flag == -1, "destructor not called after pop");

    return true;
}

bool TestRingBufferSPSC_ReaderWriterTest() {
    ring_buffer_spsc<int> rb(8192);
    auto writer = [&rb]() -> bool {
        for (int i = 0; i < 100000; i++) {
            while (!rb.push(i)) {
                std::this_thread::yield();
            }
        }
        return true;
    };
    auto reader = [&rb]() -> bool {
        int counter = 100000;
        bool good = true;
        for (int i = 0; i < counter; ) {
            int x;
            if (rb.pop(x)) {
                if (x != i) {
                    if (good) {
                        std::cerr << "Data corrupted: expected " << i << ", got " << x << std::endl;
                    }
                    good = false;
                }
                i++;
            } else {
                std::this_thread::yield();
            }
        }
        return good;
    };
    std::atomic<bool> result{true};
    std::thread tw([&result, writer]() {
        if (!writer()) {
            result.store(false);
        }
    });
    std::thread tr([&result, reader]() {
        if (!reader()) {
            result.store(false);
        }
    });

    tw.join();
    tr.join();

    TOYTEST_ASSERT(result.load(), "reader/writer test failed");
    return true;
}

int main() {
    std::vector<std::string> passed, failed;
    RUN_TEST("RingBufferSPSC Sanity Test", TestRingBufferSPSC_SanityTest, passed, failed);
    RUN_TEST_TIMER("RingBufferSPSC Reader/Writer Test", TestRingBufferSPSC_ReaderWriterTest, passed, failed);

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