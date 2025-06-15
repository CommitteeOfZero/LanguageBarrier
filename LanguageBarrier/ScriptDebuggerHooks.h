#ifndef __SCRIPTDEBUGGERHOOKS_H__
#define __SCRIPTDEBUGGERHOOKS_H__

#include <vector>

#include "ScriptDebugger.h"
#include "Script.h"
#include "Config.h"
#include "SigScan.h"
#include "Game.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace lb {

inline ScriptThreadState* gameExeSc3ThreadPool = nullptr;
inline uint32_t* gameExeSc3ScriptBuffers = nullptr;

typedef char(__cdecl* GameMainLoopProc)();
inline GameMainLoopProc gameExeGameMainLoop = nullptr;
inline GameMainLoopProc gameExeGameMainLoopReal = nullptr;

typedef void(__cdecl* Sc3ExecProc)(ScriptThreadState* thread);
inline Sc3ExecProc gameExeSc3Exec = nullptr;
inline Sc3ExecProc gameExeSc3ExecReal = nullptr;

typedef LRESULT(__stdcall* GameWndProcProc)(HWND hWnd, UINT Msg, WPARAM wParam,
                                            LPARAM lParam);
inline GameWndProcProc gameExeGameWndProc = nullptr;
inline GameWndProcProc gameExeGameWndProcReal = nullptr;

typedef int(__cdecl* GamePresentProc)();
inline GamePresentProc gameExeGamePresent = nullptr;
inline GamePresentProc gameExeGamePresentReal = nullptr;

inline int* gameExeScriptIdsToFileIds = nullptr;

inline bool DummyFrame = true;

void __cdecl sc3ExecHook(ScriptThreadState* thread);

inline char __cdecl gameMainLoopHook() {
  if (gameExePMgsD3D9State) {
    ImGui_ImplDX9_NewFrame();
  } else if (gameExePMgsD3D11State) {
    ImGui_ImplDX11_NewFrame();
  }

  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
  DummyFrame = false;

  char retval = gameExeGameMainLoopReal();
  return retval;
}

inline LRESULT __stdcall gameExeGameWndProcHook(HWND hWnd, UINT Msg,
                                                WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam)) return true;

  return gameExeGameWndProcReal(hWnd, Msg, wParam, lParam);
}

inline int __cdecl gamePresentHook() {
  if (!DummyFrame) {
    ShowDebugMenu();
    ImGui::Render();
    if (gameExePMgsD3D9State) {
      ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    } else if (gameExePMgsD3D11State) {
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
  }

  int retval = gameExeGamePresentReal();

  if (!DummyFrame &&
      ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  return retval;
}

inline void initScriptDebuggerHooks() {

  gameExeSc3ThreadPool = (ScriptThreadState*)sigScan("game", "sc3ThreadPool");
  gameExeSc3ScriptBuffers = (uint32_t*)sigScan("game", "sc3ScriptBuffers");

  gameExeScriptIdsToFileIds = (int*)sigScan("game", "useOfScriptIdsToFileIds");

  scanCreateEnableHook("game", "gameMainLoop", (uintptr_t*)&gameExeGameMainLoop,
                       (LPVOID)gameMainLoopHook,
                       (LPVOID*)&gameExeGameMainLoopReal);
  scanCreateEnableHook("game", "gameWndProc", (uintptr_t*)&gameExeGameWndProc,
                       (LPVOID)gameExeGameWndProcHook,
                       (LPVOID*)&gameExeGameWndProcReal);
  scanCreateEnableHook("game", "gamePresent", (uintptr_t*)&gameExeGamePresent,
                       (LPVOID)gamePresentHook,
                       (LPVOID*)&gameExeGamePresentReal);
  if (gameExeSc3ThreadPool && gameExeSc3ScriptBuffers &&
      gameExeScriptIdsToFileIds) {
    scanCreateEnableHook("game", "sc3Exec", (uintptr_t*)&gameExeSc3Exec,
                         (LPVOID)&sc3ExecHook, (LPVOID*)&gameExeSc3ExecReal);
  }
}

inline void earlyInitScriptDebuggerHooks() {

  auto findWindow = []() -> HWND {
    HWND window = NULL;
    EnumWindows(
        [](HWND hWnd, LPARAM lParam) -> BOOL {
          DWORD processId = GetCurrentProcessId();
          DWORD windowThreadProcessId;
          GetWindowThreadProcessId(hWnd, &windowThreadProcessId);
          if (processId == windowThreadProcessId) {
            *(HWND*)lParam = hWnd;
            return FALSE;
          }
          return TRUE;
        },
        (LPARAM)&window);
    return window;
  };

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui_ImplWin32_Init(findWindow());
  if (config["gamedef"]["gameDxVersion"].get<std::string>() == "dx9") {
    ImGui_ImplDX9_Init(gameExePMgsD3D9State->device);
  } else if (config["gamedef"]["gameDxVersion"].get<std::string>() == "dx11") {
    ImGui_ImplDX11_Init(gameExePMgsD3D11State->pid3d11deviceC,
                        gameExePMgsD3D11State->pid3d11devicecontext18);
  }
}

}  // namespace lb
#endif  // !__SCRIPTDEBUGGERHOOKS_H__