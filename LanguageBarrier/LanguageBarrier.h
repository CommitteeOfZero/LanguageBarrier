#ifndef __LANGUAGEBARRIER_H__
#define __LANGUAGEBARRIER_H__

#define INST_NOP (0x90)
#define INST_CALL_LEN (5)

#ifdef DEFINE_JSON_CONSTANTS
#define LB_GLOBAL
#else
#define LB_GLOBAL extern
#endif

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>
#include <cstdlib>
#include <wchar.h>
#include <string>
#include <sstream>
#include "MinHook.h"

namespace lb {
void *memset_perms(void *dst, int val, size_t size);
void LanguageBarrierInit();
void LanguageBarrierLog(const std::string &text);
bool scanCreateEnableHook(char *category, char *name, uintptr_t *ppTarget,
                          LPVOID pDetour, LPVOID *ppOriginal);
bool createEnableApiHook(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour,
                         LPVOID *ppOriginal);
// until TsudaKageyu updates his NuGet package...
MH_STATUS MH_CreateHookApiEx(LPCWSTR pszModule, LPCSTR pszProcName,
                             LPVOID pDetour, LPVOID *ppOriginal,
                             LPVOID *ppTarget);
}

#endif  // !__LANGUAGEBARRIER_H__
