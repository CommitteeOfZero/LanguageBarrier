#ifdef _DEBUG
#include "ErrorHandling.h"

namespace lb {
void errorHandlingInit() {
  LanguageBarrierLog("Debug build, not installing crash handler...");
}
}  // namespace lb
#else
#include <CrashRpt.h>
#include <codecvt>
#include "Config.h"
#include "ErrorHandling.h"

typedef void(__cdecl* ShowErrorAndQuitProc)(LPCSTR lpCaption, char* Format,
                                            ...);
static ShowErrorAndQuitProc gameExeShowErrorAndQuit =
    NULL;  // = (ShowErrorAndQuitProc)0x48A140 (C;C)
static ShowErrorAndQuitProc gameExeShowErrorAndQuitReal = NULL;

// These need to be valid when the crash handler is called, hence global scope
static CR_INSTALL_INFOW info;
static std::wstring appName;
static std::wstring appVersion;
static std::wstring url;
static std::wstring errorReportDir;

namespace lb {
void __cdecl showErrorAndQuitHook(LPCSTR lpCaption, char* Format, ...);
void errorHandlingInit() {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;

  const std::wstring appDataDir = configGetAppDataDir();
  const std::wstring gameSaveDir = configGetGameSaveDir();

  appName = converter.from_bytes(configGetGameName());
  appVersion = converter.from_bytes(configGetPatchName());
  url = configGetCrashReportUrl();
  errorReportDir = appDataDir + L"\\error_reports";

  memset(&info, 0, sizeof(CR_INSTALL_INFOW));
  info.cb = sizeof(CR_INSTALL_INFOW);
  info.pszAppName = appName.c_str();
  info.pszAppVersion = appVersion.c_str();
  info.pszUrl = url.c_str();
  info.uPriorities[CR_HTTP] = 1;
  info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;
  info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY;
  info.dwFlags = CR_INST_ALL_POSSIBLE_HANDLERS | CR_INST_AUTO_THREAD_HANDLERS;
  info.pszErrorReportSaveDir = errorReportDir.c_str();
  info.uMiniDumpType = (MINIDUMP_TYPE)(MiniDumpWithDataSegs |
                                       MiniDumpWithIndirectlyReferencedMemory);
  if (crInstallW(&info) != 0) {
    char szErrorMsg[1024];
    crGetLastErrorMsgA(szErrorMsg, 1024);
    LanguageBarrierLog("Installing crash handler failed: " +
                       std::string(szErrorMsg));
    return;
  }

#define ADD_FILE(_addFilePath)                                 \
  {                                                            \
    const std::wstring addFilePath = _addFilePath;             \
    crAddFile2W(addFilePath.c_str(), NULL, NULL,               \
                CR_AF_MAKE_FILE_COPY | CR_AF_MISSING_FILE_OK); \
  }
  ADD_FILE(L"languagebarrier\\log.txt")
  ADD_FILE(appDataDir + L"\\dxdiag.txt")
  ADD_FILE(appDataDir + L"\\launcherdiag.txt")
  ADD_FILE(appDataDir + L"\\config.json")
  ADD_FILE(gameSaveDir + L"\\*")
#undef ADD_FILE

  if (!scanCreateEnableHook("game", "showErrorAndQuit",
                            (uintptr_t*)&gameExeShowErrorAndQuit,
                            (LPVOID)&showErrorAndQuitHook,
                            (LPVOID*)&gameExeShowErrorAndQuitReal)) {
    LanguageBarrierLog(
        "Couldn't hook showErrorAndQuit, high level game errors will not be "
        "reported");
  }
}
void __cdecl showErrorAndQuitHook(LPCSTR lpCaption, char* Format, ...) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;

  char Text[0x800];
  va_list args;
  va_start(args, Format);
  vsnprintf_s(Text, 0x800, Format, args);

  LanguageBarrierLog("[Game Error] " + std::string(Text));
  MessageBoxA(NULL, Text, "Game Error", MB_OK);

  CR_EXCEPTION_INFO exinfo;
  memset(&exinfo, 0, sizeof(CR_EXCEPTION_INFO));
  exinfo.cb = sizeof(CR_EXCEPTION_INFO);
  exinfo.bManual = TRUE;
  exinfo.exctype =
      CR_CPP_INVALID_PARAMETER;  // only thing that lets us attach a string
  exinfo.expression = converter.from_bytes(Text).c_str();

  if (crGenerateErrorReport(&exinfo) != 0) {
    char szErrorMsg[1024];
    crGetLastErrorMsgA(szErrorMsg, 1024);
    LanguageBarrierLog("Reporting game error failed: " +
                       std::string(szErrorMsg));
  }

  exit(1);
}
}  // namespace lb
#endif