#include "LanguageBarrier.h"
#include <ctime>
#include <fstream>
#include "MinHook.h"
#include "Config.h"
#include "Game.h"
#include "SigScan.h"

static bool isInitialised = false;

namespace lb {
void LanguageBarrierInit() {
  if (isInitialised) {
    LanguageBarrierLog("LanguageBarrierInit() called twice...");
    return;
  }
  isInitialised = true;

  std::remove("languagebarrier\\log.txt");
  // TODO: proper versioning
  LanguageBarrierLog("LanguageBarrier for STEINS;GATE");
  LanguageBarrierLog("**** Start apprication ****");

  MH_STATUS mhStatus = MH_Initialize();
  if (mhStatus != MH_OK) {
    std::stringstream logstr;
    logstr << "MinHook failed to initialize!" << MH_StatusToString(mhStatus);
    LanguageBarrierLog(logstr.str());
    return;
  }

  Config::init();

  WCHAR path[MAX_PATH], exeName[_MAX_FNAME];
  GetModuleFileNameW(NULL, path, MAX_PATH);
  _wsplitpath_s(path, NULL, 0, NULL, 0, exeName, _MAX_FNAME, NULL, 0);
  if (_wcsicmp(exeName, L"Launcher") != 0) {
    {
      std::stringstream logstr;
      logstr << "Game.exe detected";
      LanguageBarrierLog(logstr.str());
    }
    gameInit();
  }
}
// TODO: make this better
void LanguageBarrierLog(const std::string &text) {
  std::ofstream logFile("languagebarrier\\log.txt",
                        std::ios_base::out | std::ios_base::app);
  std::time_t t = std::time(NULL);
  logFile << std::put_time(std::gmtime(&t), "[%D %r] ");
  logFile << text << std::endl;
}
bool scanCreateEnableHook(char *category, char *name, uintptr_t *ppTarget,
                          LPVOID pDetour, LPVOID *ppOriginal) {
  *ppTarget = sigScan(category, name);
  if (*ppTarget == NULL) return false;

  MH_STATUS mhStatus;
  mhStatus = MH_CreateHook((LPVOID)*ppTarget, pDetour, ppOriginal);
  if (mhStatus != MH_OK) {
    std::stringstream logstr;
    logstr << "Failed to create hook " << name << ": "
           << MH_StatusToString(mhStatus);
    LanguageBarrierLog(logstr.str());
    return false;
  }
  mhStatus = MH_EnableHook((LPVOID)*ppTarget);
  if (mhStatus != MH_OK) {
    std::stringstream logstr;
    logstr << "Failed to enable hook " << name << ": "
           << MH_StatusToString(mhStatus);
    LanguageBarrierLog(logstr.str());
    return false;
  }

  std::stringstream logstr;
  logstr << "Successfully hooked " << name;
  LanguageBarrierLog(logstr.str());

  return true;
}
bool createEnableApiHook(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour,
                         LPVOID *ppOriginal) {
  MH_STATUS mhStatus;
  LPVOID pTarget;
  mhStatus =
      MH_CreateHookApiEx(pszModule, pszProcName, pDetour, ppOriginal, &pTarget);
  if (mhStatus != MH_OK) {
    std::stringstream logstr;
    logstr << "Failed to create API hook " << pszModule << "." << pszProcName
           << ": " << MH_StatusToString(mhStatus);
    LanguageBarrierLog(logstr.str());
    return false;
  }
  mhStatus = MH_EnableHook(pTarget);
  if (mhStatus != MH_OK) {
    std::stringstream logstr;
    logstr << "Failed to enable API hook " << pszModule << "." << pszProcName
           << ": " << MH_StatusToString(mhStatus);
    LanguageBarrierLog(logstr.str());
    return false;
  }

  std::stringstream logstr;
  logstr << "Successfully hooked " << pszModule << "." << pszProcName;
  LanguageBarrierLog(logstr.str());

  return true;
}
//-------------------------------------------------------------------------
MH_STATUS MH_CreateHookApiEx(LPCWSTR pszModule, LPCSTR pszProcName,
                             LPVOID pDetour, LPVOID *ppOriginal,
                             LPVOID *ppTarget) {
  HMODULE hModule;
  LPVOID pTarget;

  hModule = GetModuleHandleW(pszModule);
  if (hModule == NULL) return MH_ERROR_MODULE_NOT_FOUND;

  pTarget = (LPVOID)GetProcAddress(hModule, pszProcName);
  if (pTarget == NULL) return MH_ERROR_FUNCTION_NOT_FOUND;

  if (ppTarget != NULL) *ppTarget = pTarget;

  return MH_CreateHook(pTarget, pDetour, ppOriginal);
}
}
