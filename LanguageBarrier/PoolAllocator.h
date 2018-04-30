#ifndef __POOLALLOCATOR_H__
#define __POOLALLOCATOR_H__

#include <cstdint>
#include <cstring>

namespace lb {

// Not threadsafe on its own
template <size_t Count, size_t MinSize, size_t MaxSize>
class PoolAllocator {
 public:
  explicit PoolAllocator() : _memory(malloc(Count * MaxSize)) {}
  ~PoolAllocator() { free(_memory); }

  void* tryAlloc(size_t size) {
    if (boundsCheck(size)) {
      for (int i = 0; i < Count; i++) {
        if (!_blockUsed[i]) {
          _blockUsed[i] = true;
          return (void*)((uintptr_t)_memory + i * MaxSize);
        }
      }
      printf("%d full!\n", MaxSize);
    }
    return nullptr;
  }

  void* tryCalloc(size_t num, size_t size) {
    void* memory = tryAlloc(num * size);
    if (memory != nullptr) {
      memset(memory, 0, MaxSize);
    }
    return memory;
  }

  bool tryFree(void* ptr) {
    if (contains(ptr)) {
      size_t block_id = blockOf(ptr);
      _blockUsed[block_id] = false;
      return true;
    }
    return false;
  }

  // When using with other allocators, beware of cross-allocator reallocs
  bool tryRealloc(void* ptr, size_t new_size) {
    return contains(ptr) && boundsCheck(new_size);
  }

  bool boundsCheck(size_t size) { return size >= MinSize && size <= MaxSize; }

  bool contains(void* ptr) {
    return ptr >= _memory &&
           ptr < (void*)((uintptr_t)_memory + Count * MaxSize);
  }

  size_t blockOf(void* ptr) {
    return ((uintptr_t)ptr - (uintptr_t)_memory) / MaxSize;
  }

  size_t getMaxSize() { return MaxSize; }

 private:
  void* _memory;
  bool _blockUsed[Count] = {0};
};

}  // namespace lb

#endif  // !__POOLALLOCATOR_H__
