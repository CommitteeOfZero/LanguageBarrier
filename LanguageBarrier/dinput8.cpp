#include "LanguageBarrier.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "game.h"


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) lb::LanguageBarrierInit();
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
    ExpandEnvironmentStrings(L"%WINDIR%\\System32\\dinput8.dll", expandedPath,
                             MAX_PATH);
    hRealDinput8 = LoadLibrary(expandedPath);
    if (!hRealDinput8) return DIERR_OUTOFMEMORY;
    realDirectInput8Create = (DirectInput8CreateProc)GetProcAddress(
        hRealDinput8, "DirectInput8Create");
    if (!realDirectInput8Create) return DIERR_OUTOFMEMORY;
  }

  return realDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}
}