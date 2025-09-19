# Toylib

Some simple toy header-only libraries for C++. All classes are header-only and only depend on C++ standard library.

Requires at least C++11.

## How-to-use

Simply copy them to your include folder and include them in your project. All header files doesn't rely on other files in this repo.

## Libraries

### IntrusiveNodeList

A simple STL-like intrusive linked list header. Not thread-safe.

Interfaces of `intrusive_list<T, NodeMember>`:

- `void push_front(Node* node)`: Push a node to the front of the list.
- `void push_back(Node* node)`: Push a node to the back of the list.
- `void pop_front()`: Pop a node from the front of the list.
- `void pop_back()`: Pop a node from the back of the list.
- `T* front()`/`T* back()`: Get the front/back node.
- `void insert(iterator pos, T* item)`: Insert an item before the position.
- `iterator erase(iterator pos)`: Erase the item at the position, return the next item.
- `iterator begin()/end()`: Get the begin/end iterator.
- `bool empty()`: Check if the list is empty.
- `size_t size()`: Get the size of the list.
- `void clear()`: Clear the list.
- `void swap(intrusive_list&)`: Swap two lists.

Usage example:

```C++

```

### FixedMemPool

Header file of a simple memory pool. It supports fixed-size memory blocks and thread-safe allocation and deallocation.

Interfaces of `fixed_mem_pool<item_size, chunk_size>`:

- `void alloc_new_chunk()`: Manually allocate a new chunk of memory.
- `char* alloc(bool alloc_when_exhausted = true)`: Allocate a memory block. `alloc_when_exhausted` specifies whether to allocate a new chunk when there is no free block. If set to false, it will return nullptr when exhausted.
- `template <typename T> T* alloc_as()`: Allocate a memory block and cast it to T*.
- `void free(void* ptr)`: Free a memory block.

Usage example:

```C++

```

### RingBuffer

A simple lock-free ring buffer header. Has SPSC ~~and MPMC~~ variants. Thread-safe.

Interfaces of `ring_buffer_spsc<T>`:

- `bool pop(T& out)`: Pop an item from buffer, return true and set `out` if successful, return false if buffer is empty.
- `template <typename U> bool push(U&& item)`: Push an item to buffer, return true if successful, return false if buffer is full.
- `bool empty()`: Check if buffer is empty.
- `bool full()`: Check if buffer is full.
- `size_t size()`: Get number of items in buffer.

Usage example:

```C++

```

### BlockingQueue

to be completed

## Tests

Each header has its own test file. You can compile them and run tests for each header.