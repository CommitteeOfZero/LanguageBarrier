#include "LanguageBarrier.h"
#include <string>
#include <vector>
#include <map>
#include "MinHook.h"
#include "Game.h"
#include "SigScan.h"
#include "BinkMod.h"
#include "Config.h"
#include "GameText.h"

typedef int(__cdecl *EarlyInitProc)(int unk0, int unk1);
static EarlyInitProc gameExeEarlyInit = NULL;
static EarlyInitProc gameExeEarlyInitReal = NULL;

typedef int(__thiscall *MpkFopenByIdProc)(void *pThis, void *mpkObject,
                                          int fileId, int unk3);
static MpkFopenByIdProc gameExeMpkFopenById = NULL;
static MpkFopenByIdProc gameExeMpkFopenByIdReal = NULL;

typedef int(__cdecl *MpkFslurpByIdProc)(uint8_t mpkId, int fileId,
                                        void **ppOutData);
static MpkFslurpByIdProc gameExeMpkFslurpById = NULL;
static MpkFslurpByIdProc gameExeMpkFslurpByIdReal = NULL;

typedef const char *(__cdecl *GetStringFromScriptProc)(int scriptId,
                                                       int stringId);
static GetStringFromScriptProc gameExeGetStringFromScript = NULL;
static GetStringFromScriptProc gameExeGetStringFromScriptReal = NULL;

typedef void(__thiscall *MpkConstructorProc)(void *pThis);
static MpkConstructorProc gameExeMpkConstructor = NULL;

static uintptr_t gameExeTextureLoadInit1 = NULL;
static uintptr_t gameExeTextureLoadInit2 = NULL;
static uintptr_t gameExeGslPngload = NULL;
static uintptr_t gameExeMpkMount = NULL;
static uintptr_t gameExePpLotsOfState = NULL;
static uintptr_t gameExePCurrentBgm = NULL;
// scroll height is +6A78

static void **gameExePpLoadedScripts = NULL;
// I'm assuming we can rely on the game always loading scripts with
// mpkFslurpById at some point
static int scriptIdsToFiles[lb::MAX_LOADED_SCRIPTS] = {0};

static std::string stringReplacementTable;
static void *c0dataMpk = NULL;

namespace lb {
int __cdecl earlyInitHook(int unk0, int unk1);
int __fastcall mpkFopenByIdHook(void *pThis, void *EDX, void *mpkObject,
                                int fileId, int unk3);
int __cdecl mpkFslurpByIdHook(uint8_t mpkId, int fileId, void **pOutData);
const char *__cdecl getStringFromScriptHook(int scriptId, int stringId);

void gameInit() {
    std::ifstream in("languagebarrier\\stringReplacementTable.bin",
        std::ios::in | std::ios::binary);
    in.seekg(0, std::ios::end);
    stringReplacementTable.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&stringReplacementTable[0], stringReplacementTable.size());
    in.close();

  // TODO: maybe just scan for all signatures inside SigScan?
  // auto-initialisation, similarly to Config.h
  gameExeTextureLoadInit1 = sigScan("game", "textureLoadInit1");
  gameExeTextureLoadInit2 = sigScan("game", "textureLoadInit2");
  gameExeGslPngload = sigScan("game", "gslPngload");
  gameExeMpkMount = sigScan("game", "mpkMount");
  gameExeEarlyInit = (EarlyInitProc)sigScan("game", "earlyInit");
  gameExePCurrentBgm = *((uint32_t *)sigScan("game", "useOfPCurrentBgm"));
  gameExeMpkConstructor = (MpkConstructorProc)sigScan("game", "mpkConstructor");

  // TODO: fault tolerance - we don't need to call it quits entirely just
  // because one *feature* can't work
  if (!scanCreateEnableHook("game", "earlyInit", (uintptr_t *)&gameExeEarlyInit,
                            (LPVOID)&earlyInitHook,
                            (LPVOID *)&gameExeEarlyInitReal) ||
      !scanCreateEnableHook(
          "game", "mpkFslurpById", (uintptr_t *)&gameExeMpkFslurpById,
          (LPVOID)mpkFslurpByIdHook, (LPVOID *)&gameExeMpkFslurpByIdReal) ||
      !scanCreateEnableHook("game", "getStringFromScript",
                            (uintptr_t *)&gameExeGetStringFromScript,
                            (LPVOID)getStringFromScriptHook,
                            (LPVOID *)&gameExeGetStringFromScriptReal))
    return;

  gameExePpLotsOfState = *((uint32_t *)sigScan("game", "useOfPpLotsOfState"));
  gameExePpLoadedScripts = *(void ***)sigScan("game", "useOfLoadedScripts");

  binkModInit();
}

// earlyInit is called after all the subsystems have been initialised but before
// the game 'starts' proper
// so, probably a good place to do all of our initialisation that requires
// interacting with them
int __cdecl earlyInitHook(int unk0, int unk1) {
  int retval = gameExeEarlyInitReal(unk0, unk1);

  std::stringstream ssMpk;
  CHAR path[MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
  GetModuleFileNameA(NULL, path, MAX_PATH);
  _splitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
  ssMpk << drive << "\\" << dir << "languagebarrier";
  std::string lbDir = ssMpk.str();

  c0dataMpk = gameMountMpk("C0DATA", &lbDir[0], "c0data.mpk");
  LanguageBarrierLog("c0data.mpk mounted");

  if (!scanCreateEnableHook(
          "game", "mpkFopenById", (uintptr_t *)&gameExeMpkFopenById,
          (LPVOID)&mpkFopenByIdHook, (LPVOID *)&gameExeMpkFopenByIdReal))
    return retval;

  if (Config::config().j["general"]["improveTextDisplay"].get<bool>() == true) {
    gameTextInit();
  }

  return retval;
}

int __fastcall mpkFopenByIdHook(void *pThis, void *EDX, void *mpkObject,
                                int fileId, int unk3) {
  uint8_t *mpkFilename = (uint8_t *)mpkObject + 1;
  std::stringstream logstr;
  logstr << "mpkFopenById(" << mpkFilename << ".mpk, 0x" << std::hex << fileId
         << ")" << std::dec;
#ifdef _DEBUG
  LanguageBarrierLog(logstr.str());
#endif

  std::vector<std::string> categories;
  if (Config::config().j["general"]["replaceCGs"].get<bool>() == true)
    categories.push_back("hqCG");
  if (Config::config().j["fmv"]["useHqAudio"].get<bool>() == true)
    categories.push_back("hqAudio");
  if (Config::config().j["general"]["fixTranslation"].get<bool>() == true)
    categories.push_back("fixTranslation");
  if (Config::config().j["general"]["improveTextDisplay"].get<bool>() == true)
    categories.push_back("font");

  for (const auto &i : categories) {
    if (Config::fileredirection().j[i].count((char *)mpkFilename) > 0) {
      std::string key = std::to_string(fileId);
      if (Config::fileredirection().j[i][(char *)mpkFilename].count(key) == 1) {
        int newFileId =
            Config::fileredirection().j[i][(char *)mpkFilename][key].get<int>();
        logstr << " redirected to c0data.mpk, 0x" << std::hex << newFileId;
        LanguageBarrierLog(logstr.str());
        return gameExeMpkFopenByIdReal(pThis, c0dataMpk, newFileId, unk3);
      }
    }
  }

  return gameExeMpkFopenByIdReal(pThis, mpkObject, fileId, unk3);
}

int __cdecl mpkFslurpByIdHook(uint8_t mpkId, int fileId, void **ppOutData) {
  if (mpkId == MPK_ID_SCRIPT_MPK && ppOutData >= gameExePpLoadedScripts &&
      ppOutData < gameExePpLoadedScripts + MAX_LOADED_SCRIPTS) {
    uint8_t scriptId = (ppOutData - gameExePpLoadedScripts);
    scriptIdsToFiles[scriptId] = fileId;
  }
  return gameExeMpkFslurpByIdReal(mpkId, fileId, ppOutData);
}

const char *__cdecl getStringFromScriptHook(int scriptId, int stringId) {
  int fileId = scriptIdsToFiles[scriptId];
  if (Config::config().j["general"]["fixTranslation"].get<bool>() == true) {
    json targets = Config::stringredirection().j["fixTranslation"];
    std::string sFileId = std::to_string(fileId);
    if (targets.count(sFileId) > 0) {
      std::string sStringId = std::to_string(stringId);
      if (targets[sFileId].count(sStringId) == 1) {
        std::stringstream logstr;
        logstr << "redirecting string " << stringId << " in file " << fileId;
        LanguageBarrierLog(logstr.str());

        uint32_t repId = targets[sFileId][sStringId].get<uint32_t>();
        uint32_t offset = ((uint32_t *)stringReplacementTable.c_str())[repId];
        return &(stringReplacementTable.c_str()[offset]);
      }
    }
  }
  return gameExeGetStringFromScriptReal(scriptId, stringId);
}

// TODO: I probably shouldn't be writing these in assembly given it looks like
// they're all cdecl or thiscall anyway
void gameLoadTexture(uint8_t textureId, void *buffer, size_t sz) {
  __asm {
    push ebx
    push [sz]
    push [buffer]
    movzx ebx, [textureId]
    push ebx
    call gameExeGslPngload
    add esp, 0xC
    pop ebx
  }
}
// returns an archive object
void *gameMountMpk(char *mountpoint, char *directory, char *filename) {
  void *retval = calloc(1, 0x3a8);
  gameExeMpkConstructor(retval);
  __asm {
    push ecx
    mov ecx, retval
    push 0
    push filename
    push directory
    push mountpoint
    call gameExeMpkMount
    pop ecx
  }
  return retval;
}
// TODO: figure out how to turn looping off
void gameSetBgm(uint32_t fileId) { *(uint32_t *)gameExePCurrentBgm = fileId; }
}