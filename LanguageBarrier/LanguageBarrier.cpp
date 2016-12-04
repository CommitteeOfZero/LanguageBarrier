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
  BGM_CLEAR = config["gamedef"]["bgmClear"].get<uint32_t>();
  MAX_LOADED_SCRIPTS = config["gamedef"]["maxLoadedScripts"].get<uint8_t>();
  MPK_ID_SCRIPT_MPK = config["gamedef"]["mpkIdScriptMpk"].get<uint8_t>();
  MPK_ID_BGM_MPK = config["gamedef"]["mpkIdBgmMpk"].get<uint8_t>();
  AUDIO_PLAYER_ID_BGM1 = config["gamedef"]["audioPlayerIdBgm1"].get<uint8_t>();

  // GameText.h
  FIRST_FONT_ID = config["gamedef"]["firstFontId"].get<uint8_t>();
  COORDS_MULTIPLIER = config["gamedef"]["coordsMultiplier"].get<float>();
  FONT_CELL_WIDTH = config["gamedef"]["fontCellWidth"].get<uint8_t>();
  FONT_CELL_HEIGHT = config["gamedef"]["fontCellHeight"].get<uint8_t>();
  FONT_ROW_LENGTH = config["gamedef"]["fontRowLength"].get<uint8_t>();
  GLYPH_RANGE_FULLWIDTH_START =
      config["gamedef"]["glyphRangeFullwidthStart"].get<uint16_t>();
  DEFAULT_LINE_LENGTH = config["gamedef"]["defaultLineLength"].get<uint16_t>();
  DEFAULT_MAX_CHARACTERS =
      config["gamedef"]["defaultMaxCharacters"].get<uint16_t>();
  UNDERLINE_GLYPH_X = config["gamedef"]["underlineGlyphX"].get<float>();
  UNDERLINE_GLYPH_Y = config["gamedef"]["underlineGlyphY"].get<float>();
  DIALOGUE_REDESIGN_YOFFSET_SHIFT =
      config["patch"]["dialogueRedesignYOffsetShift"].get<int>();
  DIALOGUE_REDESIGN_LINEHEIGHT_SHIFT =
      config["patch"]["dialogueRedesignLineHeightShift"].get<int>();
  BACKLOG_HIGHLIGHT_DEFAULT_HEIGHT =
      config["gamedef"]["backlogHighlightDefaultHeight"].get<int8_t>();
  BACKLOG_HIGHLIGHT_HEIGHT_SHIFT =
      config["patch"]["backlogHighlightHeightShift"].get<int8_t>();
  OUTLINE_EXTRA_X = config["patch"]["outlineExtraX"].get<float>();
  OUTLINE_TEXTURE_ID = config["patch"]["outlineTextureId"].get<uint8_t>();
  SGHD_PHONE_X_PADDING = config["patch"]["sghdPhoneXPadding"].get<int>();
  GLYPH_ID_FULLWIDTH_SPACE =
      config["gamedef"]["glyphIdFullwidthSpace"].get<uint16_t>();
  GLYPH_ID_HALFWIDTH_SPACE =
      config["gamedef"]["glyphIdHalfwidthSpace"].get<uint16_t>();
}
void LanguageBarrierInit() {
  if (isInitialised) {
    LanguageBarrierLog("LanguageBarrierInit() called twice...");
    return;
  }
  isInitialised = true;

  configInit();

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
