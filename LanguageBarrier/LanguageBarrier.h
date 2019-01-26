#ifndef __LANGUAGEBARRIER_H__
#define __LANGUAGEBARRIER_H__

#define INST_NOP (0x90)
#define INST_CALL_LEN (5)
#define INST_JMP_SHORT (0xEB)

#ifdef DEFINE_JSON_CONSTANTS
#define LB_GLOBAL
#else
#define LB_GLOBAL extern
#endif

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wchar.h>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>
#include "MinHook.h"

namespace lb {
template <typename T>
void write_perms(T *address, T val) {
  DWORD oldProtect;
  VirtualProtect(address, sizeof(val), PAGE_READWRITE, &oldProtect);
  *address = val;
  VirtualProtect(address, sizeof(val), oldProtect, &oldProtect);
}
void *memset_perms(void *dst, int val, size_t size);
size_t alignCeil(size_t val, size_t align);
void LanguageBarrierInit();
void LanguageBarrierLog(const std::string &text);
bool scanCreateEnableHook(char *category, char *name, uintptr_t *ppTarget,
                          LPVOID pDetour, LPVOID *ppOriginal);
bool createEnableApiHook(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour,
                         LPVOID *ppOriginal);
void slurpFile(const std::string &fileName, std::string **ppBuffer);

extern bool IsConfigured;
extern bool IsInitialised;
}  // namespace lb

#endif  // !__LANGUAGEBARRIER_H__
