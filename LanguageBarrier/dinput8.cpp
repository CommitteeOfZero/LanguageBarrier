#include "LanguageBarrier.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "game.h"

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

namespace lb {
typedef HRESULT(__stdcall* DirectInput8CreateProc)(HINSTANCE hinst,
                                                   DWORD dwVersion,
                                                   REFIID riidltf,
                                                   LPVOID* ppvOut,
                                                   LPUNKNOWN punkOuter);

DirectInput8CreateProc realDirectInput8Create = NULL;
HINSTANCE hRealDinput8 = NULL;

#pragma comment(linker, "/EXPORT:DirectInput8Create=_DirectInput8CreateHook@20")

extern "C" HRESULT __stdcall DirectInput8CreateHook(HINSTANCE hinst,
                                                    DWORD dwVersion,
                                                    REFIID riidltf,
                                                    LPVOID* ppvOut,
                                                    LPUNKNOWN punkOuter) {
  if (!hRealDinput8) {
    TCHAR expandedPath[MAX_PATH];
    UINT dirLen = GetWindowsDirectory(expandedPath, MAX_PATH);
    if (!dirLen || dirLen >= MAX_PATH - sizeof("\\System32\\dinput8.dll")) return DIERR_OUTOFMEMORY;
    if (expandedPath[dirLen - 1] != L'\\')
        expandedPath[dirLen++] = L'\\';
    memcpy(expandedPath + dirLen, __TEXT("System32\\dinput8.dll"), sizeof(__TEXT("System32\\dinput8.dll")));
    hRealDinput8 = LoadLibrary(expandedPath);
    if (!hRealDinput8) return DIERR_OUTOFMEMORY;
    realDirectInput8Create = (DirectInput8CreateProc)GetProcAddress(
        hRealDinput8, "DirectInput8Create");
    if (!realDirectInput8Create) return DIERR_OUTOFMEMORY;
  }

  return realDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}
}  // namespace lb