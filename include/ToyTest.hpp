#ifndef TOYLIB_TEST_FRAMEWORK
#define TOYLIB_TEST_FRAMEWORK

#include <iostream>
#include <chrono>

#define TOYTEST_ASSERT(expr, fail_msg) \
    if (!(expr)) { \
        std::cerr << "Assertion failed: " << fail_msg << std::endl; \
        return false; \
    }

#define TOYTEST_THROW(expr, fail_msg) { \
        bool __caught = false; \
        try { \
            expr; \
        } catch (...) { \
            __caught = true; \
        } \
        if (!__caught) { \
            std::cerr << "Exception not thrown: " << fail_msg << std::endl; \
            return false; \
        } \
    }

#define TOYTEST_NOTHROW(expr, fail_msg) { \
        bool __caught = false; \
        try { \
            expr; \
        } catch (...) { \
            __caught = true; \
        } \
        if (__caught) { \
            std::cerr << "Exception thrown: " << fail_msg << std::endl; \
            return false; \
        } \
    }

#define RUN_TEST(name, fn, passed, failed) \
    std::cout << "Test for " << name << std::endl; \
    if (fn()) { \
        std::cout << "[PASSED] " << name << std::endl; \
        passed.push_back(name); \
    } else { \
        std::cout << "[FAILED] " << name << std::endl; \
        failed.push_back(name); \
    }

#define RUN_TEST_TIMER(name, fn, passed, failed) \
    std::cout << "Test for " << name << std::endl; \
    { \
        auto start = std::chrono::high_resolution_clock::now(); \
        if (fn()) { \
            auto end = std::chrono::high_resolution_clock::now(); \
            std::cout << "[PASSED] " << name << std::endl; \
            passed.push_back(name); \
            std::cout << "Time taken:" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl; \
        } else { \
            auto end = std::chrono::high_resolution_clock::now(); \
            std::cout << "[FAILED] " << name << std::endl; \
            failed.push_back(name); \
            std::cout << "Time taken:" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl; \
        } \
    }

#endif