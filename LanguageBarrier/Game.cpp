#define GAME_H_IMPORT
#include "Game.h"
#include <d3d9.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include "BinkMod.h"
#include "Config.h"
#include "DialogueWordwrap.h"
#include "GameText.h"
#include "LanguageBarrier.h"
#include "MemoryManagement.h"
#include "MinHook.h"
#include "Script.h"
#include "Shlobj.h"
#include "SigScan.h"

typedef int(__cdecl *EarlyInitProc)(int unk0, int unk1);
static EarlyInitProc gameExeEarlyInit = NULL;
static EarlyInitProc gameExeEarlyInitReal = NULL;

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
typedef int(__cdecl *Sg0GslPngLoadProc)(int textureId, void *png, int size,
                                        int unused0, int unused1);
static uintptr_t gameExeGslPngload = NULL;

typedef void(__cdecl *SetSamplerStateWrapperProc)(int sampler, int flags);
static SetSamplerStateWrapperProc gameExeSetSamplerStateWrapper = NULL;
static SetSamplerStateWrapperProc gameExeSetSamplerStateWrapperReal = NULL;

typedef FILE *(__cdecl *FopenProc)(const char *filename, const char *mode);
static FopenProc gameExeClibFopen = NULL;
static FopenProc gameExeClibFopenReal = NULL;

typedef BOOL(__cdecl *OpenMyGamesProc)(char *outPath);
static OpenMyGamesProc gameExeOpenMyGames = NULL;  // = 0x48EE50 (C;C)

static mpkObject *gameExeMpkObjects = NULL;
typedef int(__thiscall *MpkFopenByIdProc)(void *pThis, mpkObject *mpk,
                                          int fileId, int unk3);
static MpkFopenByIdProc gameExeMpkFopenById = NULL;
static MpkFopenByIdProc gameExeMpkFopenByIdReal = NULL;

typedef int(__cdecl *SNDgetPlayLevelProc)(int a1);
static SNDgetPlayLevelProc gameExeSNDgetPlayLevel = NULL;
static SNDgetPlayLevelProc gameExeSNDgetPlayLevelReal = NULL;

static void **gameExeVoiceTable = (void **)NULL;

#pragma pack(push, 1)
struct mgsFileHandle {
  char gap0[8];
  int mpkFileId;
  char isVirtual;
  char charD;
  int gapE;
  char gap12[22];
  __int64 size;
  char gap30[17];
  char isLoaded;
  char gap42[206];
  int dword110;
  char gap114[4];
  int filesize;
  int dword11C;
  int dword120;
  int dword124;
  int dword128;
  int dword12C;
  char byte130;
  char gap131[3];
  mpkObject *pCurrentMpk;
  char char138;
  char gap139[3];
  char char13C;
  char gap13D[11];
  FILE *cFile;
};
#pragma pack(pop)

typedef struct {
  int position;
  char gap4[1];
  char byte5;
  char gap6[5];
  char shouldLoop;
  char gapC[4];
  char byte10;
  char gap11[11];
  float volume;
  char gap20[20];
  int dword34;
  void *pvoid38;
  int playbackState;
  char gap40[4];
  mgsFileHandle *file;
  char gap48[352];
  int dword1A8;
  char gap1AC[28];
  int loopStart1;
  int loopStart2;
  char gap1D0[44];
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

typedef int(__thiscall *ReadOggMetadataProc)(CPlayer *pThis);
static ReadOggMetadataProc gameExeReadOggMetadata = NULL;
static ReadOggMetadataProc gameExeReadOggMetadataReal = NULL;

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
static int *gameExeScriptIdsToFileIds = NULL;

static std::string stringReplacementTable;
static mpkObject *c0dataMpk = NULL;

namespace lb {
int __cdecl earlyInitHook(int unk0, int unk1);
int __fastcall mpkFopenByIdHook(void *pThis, void *EDX, mpkObject *mpk,
                                int fileId, int unk3);
const char *__cdecl getStringFromScriptHook(int scriptId, int stringId);
int __fastcall closeAllSystemsHook(void *pThis, void *EDX);
FILE *__cdecl clibFopenHook(const char *filename, const char *mode);
void __cdecl setSamplerStateWrapperHook(int sampler, int flags);
int __fastcall readOggMetadataHook(CPlayer *pThis, void *EDX);
BOOL __cdecl openMyGamesHook(char *outPath);
int __cdecl SNDgetPlayLevelHook(int a1);

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

  gameExeGetFlag = (GetFlagProc)sigScan("game", "getFlag");
  gameExeSetFlag = (SetFlagProc)sigScan("game", "setFlag");
  gameExeChkViewDic = (ChkViewDicProc)sigScan("game", "chkViewDic");

  scanCreateEnableHook("game", "openMyGames", (uintptr_t *)&gameExeOpenMyGames,
                       (LPVOID)openMyGamesHook, NULL);

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

  memoryManagementInit();
  scriptInit();

  gameExePMgsD3D9State =
      *((MgsD3D9State **)sigScan("game", "useOfMgsD3D9State"));
  gameExePpD3D9Ex = *((IDirect3D9Ex ***)sigScan("game", "useOfD3D9Ex"));
  gameExePPresentParameters =
      *((D3DPRESENT_PARAMETERS **)sigScan("game", "useOfPresentParameters"));

  if (config["patch"]["textureFiltering"].get<bool>() == true) {
    /*LanguageBarrierLog("Forcing bilinear filtering");
    uint8_t *branch = (uint8_t *)sigScan("game", "textureFilteringBranch");
    if (branch != NULL) {
    // original code: if (stuff) { setTextureFiltering(Point) } else {
    // setTextureFiltering(Linear) }
    // patch 'je' to 'jmp' -> always go to else block
    memset_perms(branch, INST_JMP_SHORT, 1);
    }*/
    scanCreateEnableHook("game", "setSamplerStateWrapper",
                         (uintptr_t *)&gameExeSetSamplerStateWrapper,
                         (LPVOID)setSamplerStateWrapperHook,
                         (LPVOID *)&gameExeSetSamplerStateWrapperReal);
  }

  if (config["patch"]["exitBlackScreenFix"].get<bool>() == true) {
    if (!scanCreateEnableHook(
            "game", "closeAllSystems", (uintptr_t *)&gameExeCloseAllSystems,
            (LPVOID)closeAllSystemsHook, (LPVOID *)&gameExeCloseAllSystemsReal))
      return;
  }

  if (config["gamedef"]["hasAutoSkipHide"].get<bool>() == true &&
      config["patch"]["shouldHideAutoSkip"].get<bool>() == true) {
    LanguageBarrierLog("Hiding auto/skip buttons");
    uint8_t *branch = (uint8_t *)sigScan("game", "autoSkipHideBranch");
    if (branch != NULL) {
      memset_perms(branch, INST_JMP_SHORT, 1);
    }
  }

  if (config["patch"].count("overrideLoopMetadata") == 1 &&
      config["patch"]["overrideLoopMetadata"].get<bool>() == true) {
    scanCreateEnableHook(
        "game", "readOggMetadata", (uintptr_t *)&gameExeReadOggMetadata,
        (LPVOID)readOggMetadataHook, (LPVOID *)&gameExeReadOggMetadataReal);
  }

  if (config["patch"].count("fixPlayLevelNullDeref") == 1 &&
      config["patch"]["fixPlayLevelNullDeref"].get<bool>() == true) {
    gameExeVoiceTable = (void **)sigScan("game", "useOfVoiceTable");

    if (gameExeVoiceTable == 0 ||
        !scanCreateEnableHook(
            "game", "SNDgetPlayLevel", (uintptr_t *)&gameExeSNDgetPlayLevel,
            (LPVOID)SNDgetPlayLevelHook, (LPVOID *)&gameExeSNDgetPlayLevelReal))
      return;
  }

  gameExeScriptIdsToFileIds = (int *)sigScan("game", "useOfScriptIdsToFileIds");
  gameExeAudioPlayers = *(CPlayer **)sigScan("game", "useOfAudioPlayers");
  if (config["gamedef"]["signatures"]["game"].count("useOfMpkObjects") == 1)
    gameExeMpkObjects = (mpkObject *)sigScan("game", "useOfMpkObjects");

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

  if (config["patch"]["redoDialogueWordwrap"].get<bool>() == true) {
    dialogueWordwrapInit();
  }
  if (config["patch"]["hookText"].get<bool>() == true) {
    gameTextInit();
  }

  return retval;
}

int __fastcall mpkFopenByIdHook(void *pThis, void *EDX, mpkObject *mpk,
                                int fileId, int unk3) {
  char *mpkFilename = (char *)&mpk->filename;
  std::stringstream logstr;
  logstr << "mpkFopenById(" << mpkFilename << ", 0x" << std::hex << fileId
         << ")" << std::dec;
#ifdef _DEBUG
  LanguageBarrierLog(logstr.str());
#endif
  if (config["patch"].count("fileRedirection") == 1 &&
      config["patch"]["fileRedirection"].count(mpkFilename) > 0) {
    std::string key = std::to_string(fileId);
    if (config["patch"]["fileRedirection"][mpkFilename].count(key) == 1) {
      auto red = config["patch"]["fileRedirection"][mpkFilename][key];
      if (red.type() == json::value_t::number_integer ||
          red.type() == json::value_t::number_unsigned) {
        int newFileId = red.get<int>();
        logstr << " redirected to c0data.mpk, 0x" << std::hex << newFileId;
#ifdef _DEBUG
        LanguageBarrierLog(logstr.str());
#endif
        return gameExeMpkFopenByIdReal(pThis, c0dataMpk, newFileId, unk3);
      } else if (red.type() == json::value_t::array) {
        int archiveId = red[0].get<int>();
        int newFileId = red[1].get<int>();
        logstr << " redirected to " << gameExeMpkObjects[archiveId].filename
               << ", 0x" << std::hex << newFileId;
#ifdef _DEBUG
        LanguageBarrierLog(logstr.str());
#endif
        return gameExeMpkFopenByIdReal(pThis, &gameExeMpkObjects[archiveId],
                                       newFileId, unk3);
      }
    }
  }

  return gameExeMpkFopenByIdReal(pThis, mpk, fileId, unk3);
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

void setSamplerStateWrapperHook(int sampler, int flags) {
  gameExeSetSamplerStateWrapperReal(sampler, flags);
  gameExePMgsD3D9State->device->SetSamplerState(sampler, D3DSAMP_MINFILTER,
                                                D3DTEXF_LINEAR);
  gameExePMgsD3D9State->device->SetSamplerState(sampler, D3DSAMP_MAGFILTER,
                                                D3DTEXF_LINEAR);
  // TODO: Implement mipmapping specifically for font textures
#if 0
  gameExePMgsD3D9State->device->SetSamplerState(sampler, D3DSAMP_MIPFILTER,
                                                D3DTEXF_LINEAR);
  gameExePMgsD3D9State->device->SetSamplerState(sampler, D3DSAMP_MAXMIPLEVEL, 0);
  float mipmapBias = 0.0f;
  gameExePMgsD3D9State->device->SetSamplerState(sampler, D3DSAMP_MIPMAPLODBIAS,
                                                *(DWORD *)&mipmapBias);
#endif
}

int __fastcall readOggMetadataHook(CPlayer *pThis, void *EDX) {
  int ret = gameExeReadOggMetadataReal(pThis);
  if (pThis->file != NULL && pThis->file->pCurrentMpk != NULL) {
    char *archiveFn = (char *)pThis->file->pCurrentMpk->filename;
    if (config["patch"].count("loopMetadata") == 1 &&
        config["patch"]["loopMetadata"].count(archiveFn) > 0) {
      std::string fileId = std::to_string(pThis->file->mpkFileId);
      if (config["patch"]["loopMetadata"][archiveFn].count(fileId) == 1) {
        int loopStart =
            config["patch"]["loopMetadata"][archiveFn][fileId]["loopStart"]
                .get<int>();
        // there appear to be *loop end* fields in CPlayer, but the looplength
        // comment is simply ignored by default, so we're not touching them
        // either
        // int loopLength =
        // config["patch"]["loopMetadata"][archiveFn][fileId]["loopLength"].get<int>();
        pThis->loopStart1 = loopStart;
        pThis->loopStart2 = loopStart;
      }
    }
  }
  return ret;
}

BOOL __cdecl openMyGamesHook(char *outPath) {
  // The game knows only ANSI fopen(). Unfortunately, this prevents it from
  // finding save data for users with out-of-locale usernames. So, let's first
  // try to convert from Unicode...

  const wchar_t MyGames[] = L"\\My Games\\";

  PWSTR myDocumentsPath;
  SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &myDocumentsPath);

  wchar_t *myGamesPath = (wchar_t *)calloc(
      wcslen(myDocumentsPath) + wcslen(MyGames) + 1, sizeof(wchar_t));
  wcscpy(myGamesPath, myDocumentsPath);
  wcscat(myGamesPath, MyGames);
  BOOL result = CreateDirectoryW(myGamesPath, 0);

  BOOL failedToConvert;
  WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, myGamesPath, -1, outPath,
                      MAX_PATH, NULL, &failedToConvert);
  if (failedToConvert) {
    // ...or if that fails, use 8.3 (MSDOS/FAT) short paths, which always fit in
    // ANSI.
    LanguageBarrierLog(
        "Couldn't cleanly convert My Games path to 8-bit, falling back to 8.3 "
        "paths");
    wchar_t wideShortPath[MAX_PATH];
    // Short paths (DOS 8.3 naming convention) can always be converted to ANSI
    // properly
    GetShortPathNameW(myGamesPath, wideShortPath, MAX_PATH);
    WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP, NULL,
                        wideShortPath, -1, outPath, MAX_PATH, NULL, NULL);
  }

  free(myGamesPath);
  CoTaskMemFree(myDocumentsPath);
  return result;
}

int __cdecl SNDgetPlayLevelHook(int a1) {
  // Bounds check from original code (needed for below S;G Steam bounds check)
  // if (a1 < 3 || a1 > 6) return 0;

  // Bounds checks from S;G Steam
  // int v4 = gameExeWavData[38 * a1 + 25];
  // int v5 = gameExeWavData[38 * a1 + 24];
  // if (v4 <= 0 || v5 <= 0 || v4 > v5) return 0;

  // But what we actually care about is preventing this null deref
  if (*gameExeVoiceTable == NULL) {
    return 0;
  } else {
    return gameExeSNDgetPlayLevelReal(a1);
  }
}

// TODO: I probably shouldn't be writing these in assembly given it looks like
// they're all cdecl or thiscall anyway
void gameLoadTexture(uint16_t textureId, void *buffer, size_t sz) {
  if (config["gamedef"]["gslPngLoadVersion"].get<std::string>() == "sghd") {
    ((SghdGslPngLoadProc)gameExeGslPngload)(textureId, buffer, sz);
  } else if (config["gamedef"]["gslPngLoadVersion"].get<std::string>() ==
             "sg0") {
    ((Sg0GslPngLoadProc)gameExeGslPngload)(textureId, buffer, sz, 0, 0);
  }
}
// returns an archive object
mpkObject *gameMountMpk(char *mountpoint, char *directory, char *filename) {
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
  return (mpkObject *)retval;
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
bool gameGetBgmIsPlaying() {
  return gameExeAudioPlayers[AUDIO_PLAYER_ID_BGM1].playbackState == 2;
}
}  // namespace lb
