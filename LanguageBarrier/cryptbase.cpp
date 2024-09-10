#include "LanguageBarrier.h"
#include "game.h"

#pragma comment( \
    linker,      \
    "/export:SystemFunction001=c:/windows/system32/cryptbase.dll.SystemFunction001")
#pragma comment( \
    linker,      \
    "/export:SystemFunction002=c:/windows/system32/cryptbase.dll.SystemFunction002")
#pragma comment( \
    linker,      \
    "/export:SystemFunction003=c:/windows/system32/cryptbase.dll.SystemFunction003")
#pragma comment( \
    linker,      \
    "/export:SystemFunction004=c:/windows/system32/cryptbase.dll.SystemFunction004")
#pragma comment( \
    linker,      \
    "/export:SystemFunction005=c:/windows/system32/cryptbase.dll.SystemFunction005")
#pragma comment( \
    linker,      \
    "/export:SystemFunction028=c:/windows/system32/cryptbase.dll.SystemFunction028")
#pragma comment( \
    linker,      \
    "/export:SystemFunction029=c:/windows/system32/cryptbase.dll.SystemFunction029")
#pragma comment( \
    linker,      \
    "/export:SystemFunction034=c:/windows/system32/cryptbase.dll.SystemFunction034")
#pragma comment( \
    linker,      \
    "/export:SystemFunction036=c:/windows/system32/cryptbase.dll.SystemFunction036")
#pragma comment( \
    linker,      \
    "/export:SystemFunction040=c:/windows/system32/cryptbase.dll.SystemFunction040")
#pragma comment( \
    linker,      \
    "/export:SystemFunction041=c:/windows/system32/cryptbase.dll.SystemFunction041")

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved) {
  if (!lb::IsInitialised) {
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