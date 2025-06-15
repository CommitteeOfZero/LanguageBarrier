#include "LanguageBarrier.h"
#include "game.h"
#include <atomic>

static HMODULE hRealCryptbase = nullptr;
void LoadDll() {
  char systemPath[MAX_PATH];
  GetSystemDirectoryA(systemPath, MAX_PATH);
  strcat_s(systemPath, "\\cryptbase.dll");
  hRealCryptbase = LoadLibraryA(systemPath);
}

#define WRAPPER_LOAD_PTR(name)              \
  if (hRealCryptbase == nullptr) LoadDll(); \
  if (o##name == nullptr) o##name = GetProcAddress(hRealCryptbase, ## #name)

#define WRAPPER_GENFUNC(name)        \
  FARPROC o##name = nullptr;         \
  __declspec(naked) void _##name() { \
    WRAPPER_LOAD_PTR(name);          \
    __asm jmp[o##name]               \
  }
WRAPPER_GENFUNC(SystemFunction001);
WRAPPER_GENFUNC(SystemFunction002);
WRAPPER_GENFUNC(SystemFunction003);
WRAPPER_GENFUNC(SystemFunction004);
WRAPPER_GENFUNC(SystemFunction005);
WRAPPER_GENFUNC(SystemFunction028);
WRAPPER_GENFUNC(SystemFunction029);
WRAPPER_GENFUNC(SystemFunction034);
WRAPPER_GENFUNC(SystemFunction036);
WRAPPER_GENFUNC(SystemFunction040);
WRAPPER_GENFUNC(SystemFunction041);

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    try {
      lb::LanguageBarrierInit();
    } catch (std::exception& e) {
      // if we're here, next attempts to initialize will probably
      // throw the same exception, no sense to retry initialization
      lb::IsInitialised = true;
      MessageBoxA(NULL, e.what(), "LanguageBarrier exception", MB_ICONSTOP);
    }
  }
  return TRUE;
}