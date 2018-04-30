#include "MemoryManagement.h"
#include "Config.h"
#include "LanguageBarrier.h"
#include "PoolAllocator.h"

// This part of the patch hooks malloc() and friends and first tries to serve
// allocation requests for 4-36MB blocks with custom pool allocators before
// going to system malloc. Why? At 2GB address space, the engine tends to
// experience too much memory fragmentation, and often crashes as a result.
// Enabling Large Address Awareness on the executable solves this problem, but
// we can't always do that (e.g. for DRM'd Steam releases - which may also
// receive updates). So to help in those situations, we reserve some space for
// loading resources.

static HANDLE allocatorMutex;

static lb::PoolAllocator<7, 4 * 1024 * 1024, 10 * 1024 * 1024>
    *poolAllocatorSmall;
static lb::PoolAllocator<4, (10 * 1024 * 1024) + 1, 36 * 1024 * 1024>
    *poolAllocatorLarge;

typedef void *(__cdecl *MallocProc)(size_t size);
static MallocProc gameExeMalloc = NULL;  // 00478CB5 (Chaos;Child)
static MallocProc gameExeMallocReal = NULL;
typedef void *(__cdecl *CallocProc)(size_t num, size_t size);
static CallocProc gameExeCalloc = NULL;  // 0049FDE1 (Chaos;Child)
static CallocProc gameExeCallocReal = NULL;
static CallocProc gameExeCallocCrt = NULL;  // 00472CC5 (Chaos;Child)
static CallocProc gameExeCallocCrtReal = NULL;
typedef size_t(__cdecl *MsizeProc)(void *ptr);
static MsizeProc gameExeMsize = NULL;  // 00472C94 (Chaos;Child)
static MsizeProc gameExeMsizeReal = NULL;
typedef void(__cdecl *FreeProc)(void *ptr);
static FreeProc gameExeFree = NULL;  // 00470975 (Chaos;Child)
static FreeProc gameExeFreeReal = NULL;
typedef void *(__cdecl *ReallocProc)(void *ptr, size_t new_size);
static ReallocProc gameExeRealloc = NULL;  // 00478D47 (Chaos;Child)
static ReallocProc gameExeReallocReal = NULL;

namespace lb {
void *__cdecl mallocHook(size_t size);
void *__cdecl callocHook(size_t num, size_t size);
void *__cdecl callocCrtHook(size_t num, size_t size);
size_t __cdecl msizeHook(void *ptr);
void __cdecl freeHook(void *ptr);
void *__cdecl reallocHook(void *ptr, size_t new_size);

void memoryManagementInit() {
  if (config["patch"].count("usePoolAllocators") == 1 &&
      config["patch"]["usePoolAllocators"].get<bool>() == true) {
    if (scanCreateEnableHook("game", "malloc", (uintptr_t *)&gameExeMalloc,
                             (LPVOID)mallocHook,
                             (LPVOID *)&gameExeMallocReal) &&
        scanCreateEnableHook("game", "calloc", (uintptr_t *)&gameExeCalloc,
                             (LPVOID)callocHook,
                             (LPVOID *)&gameExeCallocReal) &&
        scanCreateEnableHook(
            "game", "callocCrt", (uintptr_t *)&gameExeCallocCrt,
            (LPVOID)callocCrtHook, (LPVOID *)&gameExeCallocCrtReal) &&
        scanCreateEnableHook("game", "msize", (uintptr_t *)&gameExeMsize,
                             (LPVOID)msizeHook, (LPVOID *)&gameExeMsizeReal) &&
        scanCreateEnableHook("game", "free", (uintptr_t *)&gameExeFree,
                             (LPVOID)freeHook, (LPVOID *)&gameExeFreeReal) &&
        scanCreateEnableHook("game", "realloc", (uintptr_t *)&gameExeRealloc,
                             (LPVOID)reallocHook,
                             (LPVOID *)&gameExeReallocReal)) {
      std::stringstream logstr;
      logstr << "Enabling pool allocators: 7x 4MB <= allocation <= 10MB, 4x "
                "10MB < allocation <= 36MB";
      LanguageBarrierLog(logstr.str());
      poolAllocatorSmall =
          new PoolAllocator<7, 4 * 1024 * 1024, 10 * 1024 * 1024>();
      poolAllocatorLarge =
          new PoolAllocator<4, (10 * 1024 * 1024) + 1, 36 * 1024 * 1024>();
      allocatorMutex = CreateMutex(NULL, FALSE, NULL);
    } else {
      std::stringstream logstr;
      logstr << "Pool allocators configured on, but could not enable them";
      LanguageBarrierLog(logstr.str());
    }
  }
}

// Warning: This code does not implement the finer points of dynamic allocations
// on Windows (like _callnewh, or retries for callocCrt) Don't reuse unless
// you've made sure this is good enough
void *__cdecl mallocHook(size_t size) {
  WaitForSingleObject(allocatorMutex, 0);
  // printf("malloc(%d)\n", size);
  void *result;
  if ((result = poolAllocatorSmall->tryAlloc(size)) == nullptr &&
      (result = poolAllocatorLarge->tryAlloc(size)) == nullptr) {
    result = gameExeMallocReal(size);
  }
  ReleaseMutex(allocatorMutex);
  return result;
}
void *__cdecl callocHook(size_t num, size_t size) {
  WaitForSingleObject(allocatorMutex, 0);
  // printf("callocCrt(%d, %d)\n", num, size);
  void *result;
  if ((result = poolAllocatorSmall->tryCalloc(num, size)) == nullptr &&
      (result = poolAllocatorLarge->tryCalloc(num, size)) == nullptr) {
    result = gameExeCallocReal(num, size);
  }
  ReleaseMutex(allocatorMutex);
  return result;
}
void *__cdecl callocCrtHook(size_t num, size_t size) {
  WaitForSingleObject(allocatorMutex, 0);
  // printf("calloc(%d, %d)\n", num, size);
  void *result;
  if ((result = poolAllocatorSmall->tryCalloc(num, size)) == nullptr &&
      (result = poolAllocatorLarge->tryCalloc(num, size)) == nullptr) {
    result = gameExeCallocCrtReal(num, size);
  }
  ReleaseMutex(allocatorMutex);
  return result;
}
size_t __cdecl msizeHook(void *ptr) {
  WaitForSingleObject(allocatorMutex, 0);
  size_t result;
  if (poolAllocatorSmall->contains(ptr)) {
    result = poolAllocatorSmall->getMaxSize();
  } else if (poolAllocatorLarge->contains(ptr)) {
    result = poolAllocatorLarge->getMaxSize();
  } else {
    result = gameExeMsizeReal(ptr);
  }
  ReleaseMutex(allocatorMutex);
  return result;
}
void __cdecl freeHook(void *ptr) {
  WaitForSingleObject(allocatorMutex, 0);
  // printf("free() - %d\n", msizeHook(ptr));
  if (!poolAllocatorSmall->tryFree(ptr) && !poolAllocatorLarge->tryFree(ptr)) {
    gameExeFreeReal(ptr);
  }
  ReleaseMutex(allocatorMutex);
}
void *__cdecl reallocHook(void *ptr, size_t new_size) {
  WaitForSingleObject(allocatorMutex, 0);
  // printf("realloc(%d)\n", new_size);
  void *result;
  if (poolAllocatorSmall->tryRealloc(ptr, new_size) ||
      poolAllocatorLarge->tryRealloc(ptr, new_size)) {
    result = ptr;
  } else {
    if (poolAllocatorSmall->contains(ptr)) {
      result = mallocHook(new_size);
      memcpy(result, ptr, min(new_size, poolAllocatorSmall->getMaxSize()));
      poolAllocatorSmall->tryFree(ptr);
    } else if (poolAllocatorLarge->contains(ptr)) {
      result = mallocHook(new_size);
      memcpy(result, ptr, min(new_size, poolAllocatorLarge->getMaxSize()));
      poolAllocatorLarge->tryFree(ptr);
    } else {
      result = gameExeReallocReal(ptr, new_size);
    }
  }
  ReleaseMutex(allocatorMutex);
  return result;
}
}  // namespace lb