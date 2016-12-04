#include "LanguageBarrier.h"
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <d3d9.h>
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

typedef const char *(__cdecl *GetStringFromScriptProc)(int scriptId,
                                                       int stringId);
static GetStringFromScriptProc gameExeGetStringFromScript = NULL;
static GetStringFromScriptProc gameExeGetStringFromScriptReal = NULL;

typedef void(__thiscall *MpkConstructorProc)(void *pThis);
static MpkConstructorProc gameExeMpkConstructor = NULL;

typedef int(__thiscall *CloseAllSystemsProc)(void *pThis);
static CloseAllSystemsProc gameExeCloseAllSystems = NULL;
static CloseAllSystemsProc gameExeCloseAllSystemsReal = NULL;

// correct prototype chosen at runtime
typedef int(__cdecl *SghdGslPngLoadProc)(int textureId, void *png, int size);
typedef int(__cdecl *Sg0GslPngLoadProc)(int textureId, void *png, int size, int unused0, int unused1);
static uintptr_t gameExeGslPngload = NULL;

typedef FILE *(__cdecl *FopenProc)(const char *filename, const char *mode);
static FopenProc gameExeClibFopen = NULL;
static FopenProc gameExeClibFopenReal = NULL;

typedef struct {
  int position;
  char gap4[1];
  char byte5;
  char gap6[5];
  char shouldLoop;
  __declspec(align(8)) char byte10;
  char gap11[11];
  float volume;
  char gap20[20];
  int dword34;
  void *pvoid38;
  int playbackState;
  char gap40[4];
  void *file;
  char gap48[352];
  int dword1A8;
  char gap1AC[80];
  int dword1FC;
  char gap200[8];
  int dword208;
  char gap20C[388];
  int dword390;
  int dword394;
  char byte398;
  char gap399[11];
  int nativePlayer;
  char gap3A8[4];
  int filesize;
  char gap3B0[32];
  char byte3D0;
  char gap3D1[3];
  int dword3D4;
  char gap3D8[8];
  int dword3E0;
  char byte3E4;
  char gap3E5[3];
  int dword3E8;
  int dword3EC;
  char gap3F0[8];
  char decoderState[720];  // vorbisfile OggVorbis_File
  __int64 filePosition;
  char gap6D0[16];
  void *pcmOutput;
  char gap6E4[4];
  __int64 qword6E8;
  char gap6F0[26];
  char byte70A;
  char field_70B;
  char field_70C;
  char field_70D;
  char field_70E;
  char field_70F;
} CPlayer;
static CPlayer *gameExeAudioPlayers = NULL;

struct __declspec(align(4)) MgsD3D9State {
  IDirect3DSurface9 *backbuffer;
  int field_4;
  int field_8;
  IDirect3DDevice9Ex *device;
};
static MgsD3D9State *gameExePMgsD3D9State = NULL;
static IDirect3D9Ex **gameExePpD3D9Ex = NULL;
static D3DPRESENT_PARAMETERS *gameExePPresentParameters = NULL;

static uintptr_t gameExeTextureLoadInit1 = NULL;
static uintptr_t gameExeTextureLoadInit2 = NULL;
static uintptr_t gameExeMpkMount = NULL;
static uintptr_t gameExePCurrentBgm = NULL;
static uintptr_t gameExePLoopBgm = NULL;
static uintptr_t gameExePShouldPlayBgm = NULL;
// scroll height is +6A78
static int* gameExeScriptIdsToFileIds = NULL;

static std::string stringReplacementTable;
static void *c0dataMpk = NULL;

namespace lb {
int __cdecl earlyInitHook(int unk0, int unk1);
int __fastcall mpkFopenByIdHook(void *pThis, void *EDX, void *mpkObject,
                                int fileId, int unk3);
const char *__cdecl getStringFromScriptHook(int scriptId, int stringId);
int __fastcall closeAllSystemsHook(void *pThis, void *EDX);
FILE *__cdecl clibFopenHook(const char *filename, const char *mode);

void gameInit() {
  std::ifstream in("languagebarrier\\stringReplacementTable.bin",
                   std::ios::in | std::ios::binary);
  in.seekg(0, std::ios::end);
  stringReplacementTable.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&stringReplacementTable[0], stringReplacementTable.size());
  in.close();

  gameExeTextureLoadInit1 = sigScan("game", "textureLoadInit1");
  gameExeTextureLoadInit2 = sigScan("game", "textureLoadInit2");
  gameExeGslPngload = sigScan("game", "gslPngload");
  gameExeMpkMount = sigScan("game", "mpkMount");
  gameExeEarlyInit = (EarlyInitProc)sigScan("game", "earlyInit");
  gameExePCurrentBgm = sigScan("game", "useOfPCurrentBgm");
  gameExePLoopBgm = sigScan("game", "useOfPLoopBgm");
  gameExePShouldPlayBgm = sigScan("game", "useOfPShouldPlayBgm");
  gameExeMpkConstructor = (MpkConstructorProc)sigScan("game", "mpkConstructor");

  // TODO: fault tolerance - we don't need to call it quits entirely just
  // because one *feature* can't work
  if (!scanCreateEnableHook("game", "earlyInit", (uintptr_t *)&gameExeEarlyInit,
                            (LPVOID)&earlyInitHook,
                            (LPVOID *)&gameExeEarlyInitReal) ||
      !scanCreateEnableHook("game", "getStringFromScript",
                            (uintptr_t *)&gameExeGetStringFromScript,
                            (LPVOID)getStringFromScriptHook,
                            (LPVOID *)&gameExeGetStringFromScriptReal) ||
      !scanCreateEnableHook("game", "clibFopen", (uintptr_t *)&gameExeClibFopen,
                            (LPVOID)clibFopenHook,
                            (LPVOID *)&gameExeClibFopenReal))
    return;

  if (config["patch"]["exitBlackScreenFix"].get<bool>() == true) {
    if (!scanCreateEnableHook(
            "game", "closeAllSystems", (uintptr_t *)&gameExeCloseAllSystems,
            (LPVOID)closeAllSystemsHook, (LPVOID *)&gameExeCloseAllSystemsReal))
      return;

    gameExePMgsD3D9State =
        *((MgsD3D9State **)sigScan("game", "useOfMgsD3D9State"));
    gameExePpD3D9Ex = *((IDirect3D9Ex ***)sigScan("game", "useOfD3D9Ex"));
    gameExePPresentParameters =
        *((D3DPRESENT_PARAMETERS **)sigScan("game", "useOfPresentParameters"));
  }

  gameExeScriptIdsToFileIds = (int *)sigScan("game", "useOfScriptIdsToFileIds");
  gameExeAudioPlayers = *(CPlayer **)sigScan("game", "useOfAudioPlayers");

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

  if (config["patch"]["hookText"].get<bool>() == true) {
    gameTextInit();
  }

  if (config["patch"]["textureFiltering"].get<bool>() == true) {
    LanguageBarrierLog("Forcing bilinear filtering");
    uint8_t *branch = (uint8_t *)sigScan("game", "textureFilteringBranch");
    if (branch != NULL) {
      // original code: if (stuff) { setTextureFiltering(Point) } else {
      // setTextureFiltering(Linear) }
      // patch 'je' to 'jmp' -> always go to else block
      DWORD oldProtect;
      VirtualProtect(branch, 1, PAGE_READWRITE, &oldProtect);
      *branch = 0xEB;
      VirtualProtect(branch, 1, oldProtect, &oldProtect);
    }
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

  if (config["patch"].count("fileRedirection") == 1 &&
      config["patch"]["fileRedirection"].count((char *)mpkFilename) > 0) {
    std::string key = std::to_string(fileId);
    if (config["patch"]["fileRedirection"][(char *)mpkFilename].count(key) ==
        1) {
      int newFileId =
          config["patch"]["fileRedirection"][(char *)mpkFilename][key]
              .get<int>();
      logstr << " redirected to c0data.mpk, 0x" << std::hex << newFileId;
      LanguageBarrierLog(logstr.str());
      return gameExeMpkFopenByIdReal(pThis, c0dataMpk, newFileId, unk3);
    }
  }

  return gameExeMpkFopenByIdReal(pThis, mpkObject, fileId, unk3);
}

const char *__cdecl getStringFromScriptHook(int scriptId, int stringId) {
  int fileId = gameExeScriptIdsToFileIds[scriptId];
  if (config["patch"].count("stringRedirection") == 1) {
    const json &targets = config["patch"]["stringRedirection"];
    std::string sFileId = std::to_string(fileId);
    if (targets.count(sFileId) > 0) {
      std::string sStringId = std::to_string(stringId);
      if (targets[sFileId].count(sStringId) == 1) {
#ifdef _DEBUG
        std::stringstream logstr;
        logstr << "redirecting string " << stringId << " in file " << fileId;
        LanguageBarrierLog(logstr.str());
#endif

        uint32_t repId = targets[sFileId][sStringId].get<uint32_t>();
        uint32_t offset = ((uint32_t *)stringReplacementTable.c_str())[repId];
        return &(stringReplacementTable.c_str()[offset]);
      }
    }
  }
  return gameExeGetStringFromScriptReal(scriptId, stringId);
}

int __fastcall closeAllSystemsHook(void *pThis, void *EDX) {
  // IDA thinks this is thiscall
  // I'm not so sure it actually takes a parameter, but better safe than sorry,
  // right?

  int retval = gameExeCloseAllSystemsReal(pThis);

  if (config["patch"]["exitBlackScreenFix"].get<bool>() == true) {
    // Workaround for "user gets stuck on black screen when exiting while in
    // fullscreen mode": Just switch to windowed mode first.
    gameExePPresentParameters->Windowed = TRUE;
    gameExePPresentParameters->FullScreen_RefreshRateInHz = 0;
    gameExePPresentParameters->PresentationInterval =
        D3DPRESENT_INTERVAL_IMMEDIATE;
    gameExePMgsD3D9State->device->ResetEx(gameExePPresentParameters, NULL);
    // The following is left over from my previous attempt at a fix. Figured I
    // might as well be a good citizen and leave it in.
    gameExePMgsD3D9State->device->Release();
    (*gameExePpD3D9Ex)->Release();
  }

  return retval;
}

FILE *clibFopenHook(const char *filename, const char *mode) {
  const char *tmp = filename;
  if (strrchr(tmp, '\\')) tmp = strrchr(tmp, '\\') + 1;
  if (strrchr(tmp, '/')) tmp = strrchr(tmp, '/') + 1;

  if (config["patch"].count("physicalFileRedirection") == 1 &&
      config["patch"]["physicalFileRedirection"].count(tmp) == 1) {
    std::stringstream newPath;
    newPath
        << "languagebarrier\\"
        << config["patch"]["physicalFileRedirection"][tmp].get<std::string>();

    std::stringstream logstr;
    logstr << "redirecting physical fopen " << tmp << " to " << newPath.str();
    LanguageBarrierLog(logstr.str());

    return gameExeClibFopenReal(newPath.str().c_str(), mode);
  }

  return gameExeClibFopenReal(filename, mode);
}

// TODO: I probably shouldn't be writing these in assembly given it looks like
// they're all cdecl or thiscall anyway
void gameLoadTexture(uint8_t textureId, void *buffer, size_t sz) {
	if (config["gamedef"]["gslPngLoadVersion"].get<std::string>() == "sghd")
	{
		((SghdGslPngLoadProc)gameExeGslPngload)(textureId, buffer, sz);
	}
	else if (config["gamedef"]["gslPngLoadVersion"].get<std::string>() == "sg0")
	{
		((Sg0GslPngLoadProc)gameExeGslPngload)(textureId, buffer, sz, 0, 0);
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
void gameSetBgm(uint32_t fileId, bool shouldLoop) {
  // There are probably nicer ways of doing this, but this is the easiest one
  // that avoids a race
  *(uint32_t *)gameExePCurrentBgm = fileId;
  *(uint32_t *)gameExePLoopBgm = shouldLoop;
}
uint32_t gameGetBgm() { return *(uint32_t *)gameExePCurrentBgm; }
bool gameGetBgmShouldLoop() { return *(uint32_t *)gameExePLoopBgm; }
void gameSetBgmShouldPlay(bool shouldPlay) {
  *(uint32_t *)gameExePShouldPlayBgm = shouldPlay;
}
bool gameGetBgmShouldPlay() { return *(uint32_t *)gameExePShouldPlayBgm; }
void gameSetBgmPaused(bool paused) {
  // TODO: make this instantaneous
  if (gameExeAudioPlayers[AUDIO_PLAYER_ID_BGM1].playbackState != 4 &&
      gameExeAudioPlayers[AUDIO_PLAYER_ID_BGM1].playbackState != 2)
    return;
  gameExeAudioPlayers[AUDIO_PLAYER_ID_BGM1].playbackState = paused ? 4 : 2;
}
}