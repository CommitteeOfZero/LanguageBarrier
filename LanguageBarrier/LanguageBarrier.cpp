#define DEFINE_JSON_CONSTANTS
#include "LanguageBarrier.h"
#include <ctime>
#include <fstream>
#include "MinHook.h"
#include "Config.h"
#include "Game.h"
#include "SigScan.h"
#include "GameText.h"

static bool isInitialised = false;

namespace lb {
void loadJsonConstants() {
  LanguageBarrierLog("loading constants from gamedef.json/patchdef.json...");

  // Game.h
  BGM_CLEAR = Config::gamedef().j["bgmClear"].get<uint32_t>();
  MAX_LOADED_SCRIPTS = Config::gamedef().j["maxLoadedScripts"].get<uint8_t>();
  MPK_ID_SCRIPT_MPK = Config::gamedef().j["mpkIdScriptMpk"].get<uint8_t>();
  MPK_ID_BGM_MPK = Config::gamedef().j["mpkIdBgmMpk"].get<uint8_t>();
  AUDIO_PLAYER_ID_BGM1 =
      Config::gamedef().j["audioPlayerIdBgm1"].get<uint8_t>();

  // GameText.h
  FIRST_FONT_ID = Config::gamedef().j["firstFontId"].get<uint8_t>();
  COORDS_MULTIPLIER = Config::gamedef().j["coordsMultiplier"].get<float>();
  FONT_CELL_WIDTH = Config::gamedef().j["fontCellWidth"].get<uint8_t>();
  FONT_CELL_HEIGHT = Config::gamedef().j["fontCellHeight"].get<uint8_t>();
  FONT_ROW_LENGTH = Config::gamedef().j["fontRowLength"].get<uint8_t>();
  GLYPH_RANGE_FULLWIDTH_START =
      Config::gamedef().j["glyphRangeFullwidthStart"].get<uint16_t>();
  DEFAULT_LINE_LENGTH =
      Config::gamedef().j["defaultLineLength"].get<uint16_t>();
  DEFAULT_MAX_CHARACTERS =
      Config::gamedef().j["defaultMaxCharacters"].get<uint16_t>();
  UNDERLINE_GLYPH_X = Config::gamedef().j["underlineGlyphX"].get<float>();
  UNDERLINE_GLYPH_Y = Config::gamedef().j["underlineGlyphY"].get<float>();
  DIALOGUE_REDESIGN_YOFFSET_SHIFT =
      Config::patchdef().j["dialogueRedesignYOffsetShift"].get<int>();
  DIALOGUE_REDESIGN_LINEHEIGHT_SHIFT =
      Config::patchdef().j["dialogueRedesignLineHeightShift"].get<int>();
  BACKLOG_HIGHLIGHT_DEFAULT_HEIGHT =
      Config::gamedef().j["backlogHighlightDefaultHeight"].get<int8_t>();
  BACKLOG_HIGHLIGHT_HEIGHT_SHIFT =
      Config::patchdef().j["backlogHighlightHeightShift"].get<int8_t>();
  OUTLINE_EXTRA_X = Config::patchdef().j["outlineExtraX"].get<float>();
  OUTLINE_TEXTURE_ID = Config::patchdef().j["outlineTextureId"].get<uint8_t>();
  SGHD_PHONE_X_PADDING = Config::patchdef().j["sghdPhoneXPadding"].get<int>();
  GLYPH_ID_FULLWIDTH_SPACE =
      Config::gamedef().j["glyphIdFullwidthSpace"].get<uint16_t>();
  GLYPH_ID_HALFWIDTH_SPACE =
      Config::gamedef().j["glyphIdHalfwidthSpace"].get<uint16_t>();
}
void LanguageBarrierInit() {
  if (isInitialised) {
    LanguageBarrierLog("LanguageBarrierInit() called twice...");
    return;
  }
  isInitialised = true;

  std::remove("languagebarrier\\log.txt");
  // TODO: proper versioning
  LanguageBarrierLog("LanguageBarrier for STEINS;GATE v1.01");
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
	loadJsonConstants();
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
