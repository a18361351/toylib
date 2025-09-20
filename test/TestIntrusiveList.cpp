#include "../include/IntrusiveNodeList.hpp"
#include "../include/ToyTest.hpp"

#include <iostream>
#include <vector>


using toylib::intrusive_list;
using toylib::intrusive_node;

struct TestNode {
    int x_;
    intrusive_node node_;
    TestNode(int x = 0) : x_(x) {}
};

bool TestIntrusiveList_SimpleTest() {
    intrusive_list<TestNode, &TestNode::node_> list;
    
    TestNode n1; n1.x_ = 1;
    list.push_back(&n1);
    TestNode n2; n2.x_ = 2;
    list.push_back(&n2);

    // std::cout << "front = " << list.front()->x_ << ", back = " << list.back()->x_ << std::endl;
    TOYTEST_ASSERT(list.front()->x_ == 1, "front should be 1");
    TOYTEST_ASSERT(list.back()->x_ == 2, "back should be 2");
    return true;
}

bool TestIntrusiveList_SanityTest() {
    toylib::intrusive_list<TestNode, &TestNode::node_> list;
    TestNode t0(0);
    TestNode t1(1);
    TestNode t2(2);
    TestNode t3(3);

    TOYTEST_ASSERT(list.empty(), "list should be empty");

    // push test
    list.push_back(&t0);

    TOYTEST_ASSERT(list.back()->x_ == 0, "back should be 0");
    TOYTEST_ASSERT(list.front()->x_ == 0, "front should be 0");
    TOYTEST_ASSERT(list.size() == 1, "size() mismatch");
    TOYTEST_ASSERT(!list.empty(), "list should not be empty");

    list.push_back(&t1);
    TOYTEST_ASSERT(list.back()->x_ == 1, "back should be 1");
    TOYTEST_ASSERT(list.front()->x_ == 0, "front should be 0");
    TOYTEST_ASSERT(list.size() == 2, "size() mismatch");

    // pop test
    list.pop_front();
    TOYTEST_ASSERT(list.back()->x_ == 1, "back should be 1");
    TOYTEST_ASSERT(list.front()->x_ == 1, "front should be 1");
    TOYTEST_ASSERT(list.size() == 1, "size() mismatch");

    // then return it back
    list.push_front(&t0);
    TOYTEST_ASSERT(list.back()->x_ == 1, "back should be 1");
    TOYTEST_ASSERT(list.front()->x_ == 0, "front should be 0");
    TOYTEST_ASSERT(list.size() == 2, "size() mismatch");

    // insert test
    list.insert(list.begin(), &t2); // should be t2 t0 t1
    TOYTEST_ASSERT(list.front()->x_ == 2, "front should be 2");
    TOYTEST_ASSERT(list.size() == 3, "size() mismatch");
    std::vector<int> expected = {2, 0, 1};
    int idx = 0;

    // and iterator test
    for (auto it = list.begin(); it != list.end(); ++it) {
        TOYTEST_ASSERT(it->x_ == expected[idx++], "iterator not work correctly");
    }
    
    auto iter = list.begin(); ++iter; ++iter;

    list.insert(iter, &t3); // should be t2 t0 t3 t1
    TOYTEST_ASSERT(list.size() == 4, "size() mismatch");

    expected = {2, 0, 3, 1};
    idx = 0;

    for (auto it = list.begin(); it != list.end(); ++it) {
        TOYTEST_ASSERT(it->x_ == expected[idx++], "front erase not work correctly");
    }
    
    // erase test
    list.erase(list.begin());
    TOYTEST_ASSERT(list.front()->x_ == 0, "front should be 0");
    TOYTEST_ASSERT(list.size() == 3, "size() mismatch");

    iter = list.begin(); ++iter;
    list.erase(iter); // remove t3
    TOYTEST_ASSERT(list.size() == 2, "size() mismatch");
    expected = {0, 1};
    idx = 0;
    for (auto it = list.begin(); it != list.end(); ++it) {
        TOYTEST_ASSERT(it->x_ == expected[idx++], "mid erase not work correctly");
    }

    list.erase(--list.end());
    TOYTEST_ASSERT(list.front()->x_ == 0 && list.back()->x_ == 0, "front & back should be 0");
    TOYTEST_ASSERT(list.size() == 1, "size() mismatch");

    list.pop_back();
    TOYTEST_ASSERT(list.empty(), "list should be empty");
    TOYTEST_ASSERT(list.size() == 0, "size() mismatch");

    // empty insert
    list.insert(list.end(), &t1);
    TOYTEST_ASSERT(list.front()->x_ == 1 && list.back()->x_ == 1, "empty insert failed");
    TOYTEST_ASSERT(list.size() == 1, "size() mismatch");

    // only node erase
    list.erase(list.begin());
    TOYTEST_ASSERT(list.empty(), "list should be empty");
    TOYTEST_ASSERT(list.size() == 0, "size() mismatch");

    // clear test
    list.push_front(&t0);
    list.push_front(&t1);
    list.push_front(&t2);
    
    list.clear();
    TOYTEST_ASSERT(list.empty(), "list should be empty after clear");
    TOYTEST_ASSERT(list.size() == 0, "size() mismatch");

    return true;

}

bool TestIntrusiveList_SwapTest() {
    toylib::intrusive_list<TestNode, &TestNode::node_> l1;
    toylib::intrusive_list<TestNode, &TestNode::node_> l2;

    std::vector<TestNode> nodes;
    for (int i = 0; i < 12; i++) {
        nodes.emplace_back(i);
    } 

    for (int i = 0 ; i < 12; i++) {
        if (i % 2) {
            l1.push_back(&nodes[i]);
        } else {
            l2.push_back(&nodes[i]);
        }
    }

    std::vector<int> expect1 = {1, 3, 5, 7, 9, 11};
    std::vector<int> expect2 = {0, 2, 4, 6, 8, 10};

    auto it = l1.begin();
    for (int i = 0; i < expect1.size(); i++, ++it) {
        TOYTEST_ASSERT(it != l1.end(), "l1 iterator out of range");
        TOYTEST_ASSERT(it->x_ == expect1[i], "l1 content mismatch before swap");
    }
    TOYTEST_ASSERT(it == l1.end(), "l1 iterator should reach end");
    it = l2.begin();
    for (int i = 0; i < expect2.size(); i++, ++it) {
        TOYTEST_ASSERT(it != l2.end(), "l2 iterator out of range");
        TOYTEST_ASSERT(it->x_ == expect2[i], "l2 content mismatch before swap");
    }

    // swap
    l1.swap(l2);

    it = l1.begin();
    for (int i = 0; i < expect2.size(); i++, ++it) {
        TOYTEST_ASSERT(it != l1.end(), "l1 iterator out of range");
        TOYTEST_ASSERT(it->x_ == expect2[i], "l1 content mismatch after swap");
    }
    TOYTEST_ASSERT(it == l1.end(), "l1 iterator should reach end");
    it = l2.begin();
    for (int i = 0; i < expect1.size(); i++, ++it) {
        TOYTEST_ASSERT(it != l2.end(), "l2 iterator out of range");
        TOYTEST_ASSERT(it->x_ == expect1[i], "l2 content mismatch after swap");
    }

    // empty swap test
    l1.clear();
    TOYTEST_ASSERT(l1.empty(), "l1 should be empty after clear");

    l1.swap(l2);
    TOYTEST_ASSERT(!l1.empty(), "l1 should not be empty after swap");
    TOYTEST_ASSERT(l2.empty(), "l2 should be empty after swap");

    it = l1.begin();
    for (int i = 0; i < expect1.size(); i++, ++it) {
        TOYTEST_ASSERT(it != l1.end(), "l1 iterator out of range");
        TOYTEST_ASSERT(it->x_ == expect1[i], "l1 content mismatch after swap with empty");
    }

    // swap back
    l1.swap(l2);
    TOYTEST_ASSERT(l1.empty(), "l1 should be empty after swap back");
    TOYTEST_ASSERT(!l2.empty(), "l2 should not be empty after swap back");
    it = l2.begin();
    for (int i = 0; i < expect1.size(); i++, ++it) {
        TOYTEST_ASSERT(it != l2.end(), "l2 iterator out of range");
        TOYTEST_ASSERT(it->x_ == expect1[i], "l2 content mismatch after swap back");
    }

    l2.clear();
    TOYTEST_ASSERT(l2.empty(), "l2 should be empty after clear");

    l2.swap(l1);
    TOYTEST_ASSERT(l1.empty(), "l1 should be empty after swap with empty");
    TOYTEST_ASSERT(l2.empty(), "l2 should be empty after swap with empty");

    return true;
}

int main() {
    std::vector<std::string> passed, failed;

    RUN_TEST("IntrusiveList Simple Test", TestIntrusiveList_SimpleTest, passed, failed);
    RUN_TEST("IntrusiveList Sanity Test", TestIntrusiveList_SanityTest, passed, failed);
    RUN_TEST("IntrusiveList Swap Test", TestIntrusiveList_SwapTest, passed, failed);

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