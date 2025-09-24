#include "../include/FlatSet.hpp"
#include "../include/ToyTest.hpp"
#include <iostream>
#include <set>
#include <unordered_set>
#include <cstdlib>

using toylib::flat_set;

bool TestFlatSet_SimpleTest() {
    flat_set<int> fs;
    fs.insert(1);
    TOYTEST_ASSERT_EQ(fs.count(1), 1, "count return 0 for exist elem");
    fs.erase(1);
    TOYTEST_ASSERT_EQ(fs.count(1), 0, "count return 1 for non-exist item")

    return true;
}

bool TestFlatSet_SanityTest() {
    flat_set<int> fs;

    TOYTEST_ASSERT(fs.empty(), "constructed flat_set is not empty");
    // insert
    TOYTEST_ASSERT(fs.insert(1).second, "insert failed");
    TOYTEST_ASSERT_EQ(fs.count(1), 1, "count return 0 for exist elem");

    // hinted insert
    TOYTEST_ASSERT_EQ(*fs.insert(fs.end(), 3), 3, "insert with hint failed");
    
    TOYTEST_ASSERT_EQ(fs.count(1), 1, "count return 0 for exist elem");
    TOYTEST_ASSERT_EQ(fs.count(2), 0, "count return 1 for non-exist elem");
    TOYTEST_ASSERT_EQ(fs.count(3), 1, "count return 0 for exist elem");
    
    // ranged insert
    std::vector<int> inserted = {0, 2, 3, 5};
    fs.insert(inserted.begin(), inserted.end());
    TOYTEST_ASSERT_EQ(fs.count(0), 1, "count return 0 for exist elem");
    TOYTEST_ASSERT_EQ(fs.count(1), 1, "count return 0 for exist elem");
    TOYTEST_ASSERT_EQ(fs.count(2), 1, "count return 0 for exist elem");
    TOYTEST_ASSERT_EQ(fs.count(3), 1, "count return 0 for exist elem");
    TOYTEST_ASSERT_EQ(fs.count(4), 0, "count return 1 for non-exist elem");
    TOYTEST_ASSERT_EQ(fs.count(5), 1, "count return 0 for exist elem");
    
    TOYTEST_ASSERT_EQ(fs.size(), 5, "size incorrect after insertions");
    TOYTEST_ASSERT(!fs.empty(), "empty incorrect after insertions");

    // dup insert
    TOYTEST_ASSERT(!fs.insert(2).second, "duplicate insert returned true");

    // erase one elem
    TOYTEST_ASSERT_EQ(fs.erase(2), 1, "erase item return value incorrect");
    TOYTEST_ASSERT_EQ(fs.erase(2), 0, "erase non-exist return value incorrect");

    TOYTEST_ASSERT_EQ(fs.count(2), 0, "count return 1 for non-exist elem");
    TOYTEST_ASSERT(fs.find(2) == fs.end(), "find non-exist elem should return end()");

    // erase by iterator
    TOYTEST_ASSERT(fs.erase(fs.begin()) == fs.begin(), "erase by iterator failed");
    TOYTEST_ASSERT_EQ(fs.count(0), 0, "count return 1 for non-exist elem");

    // by now this set is {1, 3, 5}
    // test range remove
    fs.erase(fs.begin(), --fs.end());

    TOYTEST_ASSERT_EQ(fs.size(), 1, "size incorrect after erasures");
    TOYTEST_ASSERT_EQ(fs.count(1), 0, "count return 1 for non-exist elem");
    TOYTEST_ASSERT_EQ(fs.count(3), 0, "count return 1 for non-exist elem");
    TOYTEST_ASSERT_EQ(fs.count(5), 1, "count return 0 for exist elem");

    std::vector<int> inserted2 = {4, 1, 6, 10};
    std::vector<int> expected = {1, 4, 5, 6, 10};
    fs.insert(inserted2.begin(), inserted2.end());
    // range for test
    int idx = 0;
    for (int i : fs) {
        TOYTEST_ASSERT_EQ(i, expected[idx], "iteration order incorrect");
        idx++;
    }

    fs.clear();
    TOYTEST_ASSERT_EQ(fs.size(), 0, "size incorrect after clear");
    TOYTEST_ASSERT(fs.empty(), "empty incorrect after clear");

    return true;
}

bool TestFlatSet_Benchmark() {
    flat_set<int> fs;
    std::set<int> s;
    std::unordered_set<int> us;
    std::vector<int> inserted;
    for (int i = 0; i < 1000000; i++) {
        int rnd = rand() % 2000000;
        inserted.push_back(rnd);
    }
    int insert_ms, lookup_ms, iter_ms;

    // flat_set
    std::chrono::high_resolution_clock::time_point start, end;
    start = std::chrono::high_resolution_clock::now();
    for (int v : inserted) {
        fs.insert(v);
    }
    end = std::chrono::high_resolution_clock::now();
    insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    start = std::chrono::high_resolution_clock::now();
    for (int i : inserted) {
        TOYTEST_ASSERT(fs.count(i) == 1, "count return 0 for exist elem");
    }
    end = std::chrono::high_resolution_clock::now();
    lookup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    for (int i : fs) {
        // do something to num
        i++;
    }
    end = std::chrono::high_resolution_clock::now();
    iter_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "\t Insert \\ Lookup \\ Iterate" << std::endl;
    std::cout << "flat_set " << insert_ms << " ms, " << lookup_ms << " ms, " << iter_ms << " ms" << std::endl;

    // set
    start = std::chrono::high_resolution_clock::now();
    for (int v : inserted) {
        s.insert(v);
    }
    end = std::chrono::high_resolution_clock::now();
    insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    start = std::chrono::high_resolution_clock::now();
    for (int i : inserted) {
        TOYTEST_ASSERT(s.count(i) == 1, "count return 0 for exist elem");
    }
    end = std::chrono::high_resolution_clock::now();
    lookup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    start = std::chrono::high_resolution_clock::now();
    for (int i : s) {
        // do something to num
        i++;
    }
    end = std::chrono::high_resolution_clock::now();
    iter_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "std::set " << insert_ms << " ms, " << lookup_ms << " ms, " << iter_ms << " ms" << std::endl;
    
    // unordered_set
    start = std::chrono::high_resolution_clock::now();
    for (int v : inserted) {
        us.insert(v);
    }
    end = std::chrono::high_resolution_clock::now();
    insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    start = std::chrono::high_resolution_clock::now();
    for (int i : inserted) {
        TOYTEST_ASSERT(us.count(i) == 1, "count return 0 for exist elem");
    }
    end = std::chrono::high_resolution_clock::now();
    lookup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    start = std::chrono::high_resolution_clock::now();
    for (int i : us) {
        // do something to num
        i++;
    }
    end = std::chrono::high_resolution_clock::now();
    iter_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "std::unordered_set " << insert_ms << " ms, " << lookup_ms << " ms, " << iter_ms << " ms" << std::endl;

    return true;
}

int main() {
    std::vector<std::string> passed, failed;
    RUN_TEST("FlatSet Simple Test", TestFlatSet_SimpleTest, passed, failed);
    RUN_TEST("FlatSet Sanity Test", TestFlatSet_SanityTest, passed, failed);
    RUN_TEST_TIMER("FlatSet Benchmark Test", TestFlatSet_Benchmark, passed, failed);

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