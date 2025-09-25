#include "../include/FlatMap.hpp"
#include "../include/ToyTest.hpp"
#include <iostream>
#include <map>
#include <unordered_map>
using namespace toylib;

bool TestFlatMap_SimpleTest() {
    flat_map<int, std::string> fm;
    fm[0] = "Hello toylib";
    fm[1] = "Simple library";
    TOYTEST_ASSERT_EQ(fm[0], "Hello toylib", "insert failed");
    TOYTEST_ASSERT_EQ(fm.at(1), "Simple library", "insert failed");
    
    fm.erase(0);
    TOYTEST_ASSERT_EQ(fm.count(0), 0, "erase failed");
    return true;
}

bool TestFlatMap_SanityTest() {
    flat_map<int, int> fm;
    // normal insert
    fm.insert({2, 20});
    TOYTEST_ASSERT_EQ(fm.count(2), 1, "insert failed");
    TOYTEST_ASSERT_EQ(fm.at(2), 20, "inserted value not match");

    // hint insert
    fm.insert(fm.begin(), {1, 10});
    TOYTEST_ASSERT_EQ(fm.count(1), 1, "hint insert failed");
    TOYTEST_ASSERT_EQ(fm.at(1), 10, "hint inserted value not match");

    // duplicate insert
    auto res = fm.insert({2, 200});
    TOYTEST_ASSERT_EQ(res.second, false, "duplicate insert should return false");
    TOYTEST_ASSERT_EQ(fm.at(2), 20, "duplicate insert shouldn't modify existing value");

    // range insert
    std::vector<std::pair<int, int>> insert = {
        {4, 40}, {2, 25}, {3, 30}
    };
    fm.insert(insert.begin(), insert.end());
    // expect {1, 10}, {2, 20}, {3, 30}, {4, 40}
    TOYTEST_ASSERT_EQ(fm.at(1), 10, "range insert failed");
    TOYTEST_ASSERT_EQ(fm.at(2), 20, "insert shouldn't modify existing value");
    TOYTEST_ASSERT_EQ(fm.at(3), 30, "range insert failed");
    TOYTEST_ASSERT_EQ(fm.at(4), 40, "range insert failed");
    TOYTEST_ASSERT_EQ(fm.size(), 4, "size not match");

    // call at() at non-exist slot
    TOYTEST_THROW(fm.at(6), "access non-exist element should throw")
    
    // call [] at non-exist slot
    fm[8];
    TOYTEST_ASSERT_EQ(fm.count(8), 1, "operator[] should insert default value");
    fm[8] = 8888;
    TOYTEST_ASSERT_EQ(fm.at(8), 8888, "operator[] modify failed");

    // find
    auto it = fm.find(1);
    TOYTEST_ASSERT(it == fm.begin(), "find incorrect");
    auto it2 = fm.find(10);
    TOYTEST_ASSERT(it2 == fm.end(), "find non-exist key should return end()");

    // erase test
    // by key
    TOYTEST_ASSERT_EQ(fm.erase(8), 1, "erase by key failed");
    TOYTEST_ASSERT_EQ(fm.count(8), 0, "erase by key failed");

    // by iterator
    fm.erase(fm.begin());
    TOYTEST_ASSERT_EQ(fm.count(1), 0, "erase by iterator failed");

    // now fm = {(2,20), (3,30), (4,40)}
    // range erase
    fm.erase(fm.begin() + 1, fm.end());
    TOYTEST_ASSERT_EQ(fm.count(2), 1, "range erase incorrect");
    TOYTEST_ASSERT_EQ(fm.count(3), 0, "range erase incorrect");
    TOYTEST_ASSERT_EQ(fm.count(4), 0, "range erase incorrect");

    fm.clear();
    TOYTEST_ASSERT_EQ(fm.size(), 0, "clear failed");
    TOYTEST_ASSERT_EQ(fm.empty(), true, "clear failed");

    // insert again
    fm.insert(insert.begin(), insert.end());
    
    // range for
    int idx = 0;
    std::vector<std::pair<int, int>> expected = {
        {2, 25}, {3, 30}, {4, 40}
    };
    for (auto p : fm) {
        TOYTEST_ASSERT_EQ(p.first, expected[idx].first, "range for key incorrect");
        TOYTEST_ASSERT_EQ(p.second, expected[idx].second, "range for value incorrect");
        idx++;
    }

    return true;

}

bool TestFlatSet_Benchmark() {
    flat_map<int, int> fm;
    std::map<int, int> m;
    std::unordered_map<int, int> hm;
    std::vector<std::pair<int, int>> inserted;
    std::vector<int> random_acc;
    for (int i = 0; i < 1000000; i++) {
        inserted.emplace_back(i, i * 2);
    }
    for (int i = 0; i < 4000000; i++) {
        random_acc.push_back(rand() % 1000000);
    }
    int insert_ms, lookup_ms, iter_ms;

    // flat_set
    std::chrono::high_resolution_clock::time_point start, end;
    start = std::chrono::high_resolution_clock::now();
    for (auto p : inserted) {
        fm.insert(p);
    }
    end = std::chrono::high_resolution_clock::now();
    insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    start = std::chrono::high_resolution_clock::now();
    for (int idx : random_acc) {
        TOYTEST_ASSERT_EQ(fm.at(idx), idx * 2, "element not match");
    }
    end = std::chrono::high_resolution_clock::now();
    lookup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    for (auto i : fm) {
        TOYTEST_ASSERT_EQ(i.second, i.first * 2, "element not match");
    }
    end = std::chrono::high_resolution_clock::now();
    iter_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "\t Insert \\ Lookup \\ Iterate" << std::endl;
    std::cout << "flat_map " << insert_ms << " ms, " << lookup_ms << " ms, " << iter_ms << " ms" << std::endl;

    // map
    start = std::chrono::high_resolution_clock::now();
    for (auto p : inserted) {
        m.insert(p);
    }
    end = std::chrono::high_resolution_clock::now();
    insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    start = std::chrono::high_resolution_clock::now();
    for (int idx : random_acc) {
        TOYTEST_ASSERT_EQ(m.at(idx), idx * 2, "element not match");
    }
    end = std::chrono::high_resolution_clock::now();
    lookup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    for (auto i : m) {
        TOYTEST_ASSERT_EQ(i.second, i.first * 2, "element not match");
    }
    end = std::chrono::high_resolution_clock::now();
    iter_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "\t Insert \\ Lookup \\ Iterate" << std::endl;
    std::cout << "map " << insert_ms << " ms, " << lookup_ms << " ms, " << iter_ms << " ms" << std::endl;

    // unordered_map
    start = std::chrono::high_resolution_clock::now();
    for (auto p : inserted) {
        hm.insert(p);
    }
    end = std::chrono::high_resolution_clock::now();
    insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    start = std::chrono::high_resolution_clock::now();
    for (int idx : random_acc) {
        TOYTEST_ASSERT_EQ(hm.at(idx), idx * 2, "element not match");
    }
    end = std::chrono::high_resolution_clock::now();
    lookup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    for (auto i : hm) {
        TOYTEST_ASSERT_EQ(i.second, i.first * 2, "element not match");
    }
    end = std::chrono::high_resolution_clock::now();
    iter_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "\t Insert \\ Lookup \\ Iterate" << std::endl;
    std::cout << "unordered_map " << insert_ms << " ms, " << lookup_ms << " ms, " << iter_ms << " ms" << std::endl;

    return true;
}

int main() {
    std::vector<std::string> passed, failed;
    RUN_TEST("FlatMap Simple Test", TestFlatMap_SimpleTest, passed, failed);
    RUN_TEST("FlatMap Sanity Test", TestFlatMap_SanityTest, passed, failed);
    RUN_TEST("FlatMap Benchmark", TestFlatSet_Benchmark, passed, failed);

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