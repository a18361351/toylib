// FixedMemPool.hpp
// Header file for memory pool of fixed block size
// 简单内存池头文件

#ifndef TOYLIB_FIXED_MEMPOOL_HEADER
#define TOYLIB_FIXED_MEMPOOL_HEADER

#include <cstddef>
#include <memory>
#include <vector>
#include <mutex>

namespace toylib {

struct mem_node {
    mem_node* next_;
};

template <size_t Size>
struct chunk {
    char data_[Size];
};

// 简单的内存池实现
// 注：
// 1. 对象是连续分配的，发生越界时我们无法检测到，是未定义行为
// 2. 指定的item_size需要考虑内存对齐
template <size_t item_size, size_t chunk_size = 4096>
class fixed_mem_pool {
private:
    static_assert(item_size >= sizeof(mem_node*), "item_size must be greater than a pointer's size");
    static_assert(item_size <= chunk_size, "item_size must be less than or equal to chunk_size");
    std::mutex chunk_latch_;    // protects chunks
    std::vector<std::unique_ptr<char[]>> chunks_;

    std::mutex node_latch_;     // protects nodes
    mem_node* head_;

    mem_node* alloc_chunk_impl() {
        auto new_chunk = std::make_unique<char[]>(chunk_size);
        mem_node* tail_node;
        for (int i = 0; i < chunk_size / item_size; i++) {
            mem_node* node = reinterpret_cast<mem_node*>(new_chunk.get() + i * item_size);
            mem_node* next;
            if (i == chunk_size / item_size - 1) {
                next = nullptr;
                tail_node = node;
            } else {
                next = reinterpret_cast<mem_node*>(new_chunk.get() + (i + 1) * item_size);
            }
            node->next_ = next;
        }

        // latch for chunks_
        std::unique_lock cl(chunk_latch_);
        chunks_.push_back(std::move(new_chunk));
        cl.unlock();

        return tail_node;
    }

    bool debug_check_free_align(void* ptr) {
        std::vector<char*> snapshot;
        std::unique_lock cl(chunk_latch_);
        snapshot.reserve(chunks_.size());
        for (const auto& c : chunks_) {
            snapshot.push_back(c.get());
        }
        cl.unlock();

        // check if ptr is in any chunk
        for (const auto& c : snapshot) {
            if (ptr >= c && ptr < c + chunk_size) {
                // check alignment
                size_t offset = reinterpret_cast<char*>(ptr) - c;
                if (offset % item_size != 0) {
                    continue;
                } else {
                    return true;
                }
            }
        }
        return false;
    }

public:
    fixed_mem_pool(size_t chunks_count) : chunks_(0), head_(nullptr) {
        // 预分配内存
        for (int c = 0; c < chunks_count; c++) {
            chunks_.emplace_back(new char[chunk_size]);
        }

        // 手动进行空间分割
        for (int c = 0; c < chunks_count; c++) {
            for (int i = 0; i < chunk_size / item_size; i++) {
                mem_node* node = reinterpret_cast<mem_node*>(chunks_[c].get() + i * item_size);
                mem_node* next;
                if (i == chunk_size / item_size - 1) {
                    if (c == chunks_count - 1) {
                        next = nullptr;
                    } else {
                        next = reinterpret_cast<mem_node*>(chunks_[c + 1].get());
                    }
                } else {
                    next = reinterpret_cast<mem_node*>(chunks_[c].get() + (i + 1) * item_size);
                }
                node->next_ = next;
            }
        }
        head_ = reinterpret_cast<mem_node*>(chunks_[0].get());
    }

    // don't think mem pool should be copied or moved
    fixed_mem_pool(const fixed_mem_pool&) = delete;
    fixed_mem_pool& operator=(const fixed_mem_pool&) = delete;
    fixed_mem_pool(fixed_mem_pool&&) = delete;
    fixed_mem_pool& operator=(fixed_mem_pool&&) = delete;

    // interfaces

    void alloc_new_chunk() {
        auto new_tail = alloc_chunk_impl();
        
        // modify tail node
        std::unique_lock nl(node_latch_);
        if (head_) {
            // 我们可以将新chunk的尾部节点和旧head连接起来
            new_tail->next_ = head_;
        }
        head_ = reinterpret_cast<mem_node*>(chunks_.back().get());

    }

    // @param alloc_when_exhausted Whether to allocate new chunks if there is no free node. If set to false, function will return nullptr when exhausted.
    char* alloc(bool alloc_when_exhausted = true) {
        std::unique_lock nl(node_latch_);
        if (!head_) {
            if (alloc_when_exhausted) {
                alloc_chunk_impl();
                head_ = reinterpret_cast<mem_node*>(chunks_.back().get());
            } else {
                return nullptr;
            }
        }
        auto ptr = head_;
        head_ = head_->next_;
        return reinterpret_cast<char*>(ptr);
    }

    
    template <typename T>
    T* alloc_as() {
        static_assert(sizeof(T) <= item_size, "Type size is larger than item_size");
        return reinterpret_cast<T*>(alloc());
    }

    // @message This function only checks ptr's alignment and whether it was allocated from this pool in debug mode.
    // @message And it will not check double free.
    void free(void* ptr) {
        #ifndef NDEBUG
        assert(debug_check_free_align(ptr) && "Pointer to free is not allocated from this pool or not aligned");
        #endif

        auto node = reinterpret_cast<mem_node*>(ptr);

        std::unique_lock nl(node_latch_);

        node->next_ = head_;
        head_ = node;
    }
};

}
#endif