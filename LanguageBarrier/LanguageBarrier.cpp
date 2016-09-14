#include "LanguageBarrier.h"
#include <fstream>
#include "Config.h"
#include "Game.h"

static bool isInitialised = false;

namespace lb {
void LanguageBarrierInit() {
  if (isInitialised) {
    LanguageBarrierLog("LanguageBarrierInit() called twice...");
    return;
  }
  isInitialised = true;
  LanguageBarrierLog("**** Start apprication ****");

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
  // gameLoadTexture(0xF7, (void *)0x12345678, 0x200);
}

// TODO: make this better
void LanguageBarrierLog(const std::string &text) {
  std::ofstream logFile("languagebarrier\\log.txt", std::ios_base::out | std::ios_base::app);
  logFile << text << std::endl;
}
}
