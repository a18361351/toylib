#include "../include/SkipList.hpp"
#include "../include/ToyTest.hpp"
#include <iostream>
#include <map>
#include <unordered_map>

using namespace toylib;

bool TestSkipList_SimpleTest() {
    skip_list<int, int> skl;
    auto it1 = skl.insert(10, 30);
    TOYTEST_ASSERT(it1.second, "insert failed");
    TOYTEST_ASSERT_EQ(it1.first->first, 10, "wrong key");
    TOYTEST_ASSERT_EQ(it1.first->second, 30, "wrong value");
    
    auto it2 = skl.insert(20, 40);
    TOYTEST_ASSERT(it2.second, "insert failed");
    TOYTEST_ASSERT_EQ(it2.first->first, 20, "wrong key");
    TOYTEST_ASSERT_EQ(it2.first->second, 40, "wrong value");

    auto rm = skl.erase(10);
    TOYTEST_ASSERT_EQ(rm, 1, "erase failed");

    auto it3 = skl.find(10);
    TOYTEST_ASSERT(it3 == skl.end(), "erase failed");
    return true;
}

bool TestSkipList_SanityTest() {
    skip_list<int, int> skl;
    TOYTEST_ASSERT(skl.empty(), "newly constructed skip_list not empty");
    TOYTEST_ASSERT_EQ(skl.size(), 0, "newly constructed skip_list size not zero");
    TOYTEST_ASSERT(skl.begin() == skl.end(), "empty iterator behaviour incorrect");
    
    // insert test
    auto it1 = skl.insert(1, 10);
    TOYTEST_ASSERT(it1.second, "insert failed");
    TOYTEST_ASSERT_EQ(it1.first->first, 1, "wrong key");
    TOYTEST_ASSERT_EQ(it1.first->second, 10, "wrong value");
    TOYTEST_ASSERT_EQ(skl.size(), 1, "size incorrect after insert");

    auto find_it = skl.find(1);
    TOYTEST_ASSERT(it1.first == find_it, "find not correct");

    // head insert, tail insert, find
    auto it0 = skl.insert(0, 0);
    auto it2 = skl.insert(2, 20);
    TOYTEST_ASSERT(it0.second, "insert failed");
    TOYTEST_ASSERT(it2.second, "insert failed");
    TOYTEST_ASSERT_EQ(it0.first->first, 0, "wrong key");
    TOYTEST_ASSERT_EQ(it0.first->second, 0, "wrong value");
    TOYTEST_ASSERT_EQ(it2.first->first, 2, "wrong key");
    TOYTEST_ASSERT_EQ(it2.first->second, 20, "wrong value");
    TOYTEST_ASSERT_EQ(skl.size(), 3, "size incorrect after insert");

    TOYTEST_ASSERT(skl.find(0) == it0.first, "find not correct");
    TOYTEST_ASSERT(skl.find(1) == it1.first, "find not correct");
    TOYTEST_ASSERT(skl.find(2) == it2.first, "find not correct");

    // find non-exist
    TOYTEST_ASSERT(skl.find(3) == skl.end(), "find non-exist not correct");

    // at
    TOYTEST_ASSERT_EQ(skl.at(0), 0, "at not correct");
    TOYTEST_ASSERT_EQ(skl.at(1), 10, "at not correct");

    // modify by iterator
    it0.first->second = 1333;
    TOYTEST_ASSERT_EQ(skl.at(0), 1333, "modify by iterator not correct");

    // iterate
    std::vector<std::pair<int, int>> expected = {
        {0, 1333}, {1, 10}, {2, 20}
    };
    int idx = 0;
    for (auto it = skl.begin(); it != skl.end(); ++it) {
        TOYTEST_ASSERT_EQ(it->first, expected[idx].first, "iteration incorrect");
        TOYTEST_ASSERT_EQ(it->second, expected[idx].second, "iteration incorrect");
        idx++;
    }

    // erase test
    TOYTEST_ASSERT_EQ(skl.erase(1), 1, "erase failed");
    TOYTEST_ASSERT(skl.find(0) == it0.first, "erase not correct");
    TOYTEST_ASSERT(skl.find(1) == skl.end(), "erase not correct");
    TOYTEST_ASSERT(skl.find(2) == it2.first, "erase not correct");
    TOYTEST_ASSERT_EQ(skl.size(), 2, "size incorrect after erase");

    TOYTEST_ASSERT_EQ(skl.erase(3), 0, "erase non-exist failed");
    TOYTEST_ASSERT_EQ(skl.size(), 2, "size incorrect after erase non-exist");

    TOYTEST_ASSERT_EQ(skl.erase(2), 1, "erase failed");
    TOYTEST_ASSERT(skl.find(0) == it0.first, "erase not correct");
    TOYTEST_ASSERT(skl.find(1) == skl.end(), "erase not correct");
    TOYTEST_ASSERT(skl.find(2) == skl.end(), "erase not correct");
    TOYTEST_ASSERT_EQ(skl.size(), 1, "size incorrect after erase");


    // erase all elem
    TOYTEST_ASSERT_EQ(skl.erase(0), 1, "erase failed");
    TOYTEST_ASSERT(skl.find(0) == skl.end(), "erase not correct");
    TOYTEST_ASSERT_EQ(skl.size(), 0, "size incorrect after erase");
    TOYTEST_ASSERT(skl.empty(), "empty incorrect after erase");

    // empty list
    TOYTEST_ASSERT(skl.begin() == skl.end(), "empty iterator behaviour incorrect");
    TOYTEST_ASSERT_EQ(skl.erase(0), 0, "erase non-exist not correct");

    // insert again
    it0 = skl.insert(0, 10);
    TOYTEST_ASSERT(it0.second, "insert again failed");
    TOYTEST_ASSERT_EQ(it0.first->first, 0, "wrong key");
    TOYTEST_ASSERT_EQ(it0.first->second, 10, "wrong value");
    TOYTEST_ASSERT(skl.begin() == it0.first, "begin not correct");
    TOYTEST_ASSERT_EQ(skl.size(), 1, "size incorrect after insert again");
    TOYTEST_ASSERT(!skl.empty(), "empty incorrect after insert again");

    // operator[] test
    TOYTEST_ASSERT_EQ(skl[0], 10, "operator[] get failed");
    
    skl[10] = 30;
    TOYTEST_ASSERT_EQ(skl.at(10), 30, "operator[] insert failed");
    
    skl[10] = 50;
    TOYTEST_ASSERT_EQ(skl.at(10), 50, "operator[] modify failed");

    // move test
    skl.insert(1, 20);
    skl.insert(2, 30);
    TOYTEST_ASSERT_EQ(skl.at(1), 20, "insert again failed");
    TOYTEST_ASSERT_EQ(skl.at(2), 30, "insert again failed");

    skip_list<int, int> skl2 = std::move(skl);
    TOYTEST_ASSERT_EQ(skl2.size(), 3, "move size incorrect");
    TOYTEST_ASSERT_EQ(skl2.at(0), 10, "move content incorrect");
    TOYTEST_ASSERT_EQ(skl2.at(1), 20, "move content incorrect");
    TOYTEST_ASSERT_EQ(skl2.at(2), 30, "move content incorrect");

    return true;
}

bool TestSkipList_Benchmark() {
    skip_list<int, int, 10, 1, 4> skl; // 1000000 ~= 4^10
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

    // skip_list
    std::chrono::high_resolution_clock::time_point start, end;
    start = std::chrono::high_resolution_clock::now();
    for (auto p : inserted) {
        skl.insert(p.first, p.second);
    }
    end = std::chrono::high_resolution_clock::now();
    insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    start = std::chrono::high_resolution_clock::now();
    for (int idx : random_acc) {
        TOYTEST_ASSERT_EQ(skl.at(idx), idx * 2, "element not match");
    }
    end = std::chrono::high_resolution_clock::now();
    lookup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    for (auto i : skl) {
        TOYTEST_ASSERT_EQ(i.second, i.first * 2, "element not match");
    }
    end = std::chrono::high_resolution_clock::now();
    iter_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "\t Insert \\ Lookup \\ Iterate" << std::endl;
    std::cout << "skip_list " << insert_ms << " ms, " << lookup_ms << " ms, " << iter_ms << " ms" << std::endl;

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
    RUN_TEST("SkipList Simple Test", TestSkipList_SimpleTest, passed, failed);
    RUN_TEST("SkipList Sanity Test", TestSkipList_SanityTest, passed, failed);
    RUN_TEST("SkipList Benchmark Test", TestSkipList_Benchmark, passed, failed);

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
    return 0;
}