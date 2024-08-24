#define GAME_H_IMPORT
#include "Game.h"
#include <d3d9.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <filesystem>
#include "BinkMod.h"
#include "CriManaMod.h"
#include "Config.h"
#include "DialogueWordwrap.h"
#include "GameText.h"
#include "LanguageBarrier.h"
#include "MemoryManagement.h"
#include "MinHook.h"
#include "Script.h"
#include "Shlobj.h"
#include "SigScan.h"
#include "TextReplace.h"
#include "TextRendering.h"
#include "CustomInputRNE.h"
#include "CustomInputRND.h"
#include "ScriptDebuggerHooks.h"

typedef int(__cdecl* EarlyInitProc)(int unk0, int unk1);
static EarlyInitProc gameExeEarlyInit = NULL;
static EarlyInitProc gameExeEarlyInitReal = NULL;
struct RNEVertex;

typedef const char*(__cdecl* GetStringFromScriptProc)(int scriptId,
                                                      int stringId);
static GetStringFromScriptProc gameExeGetStringFromScript = NULL;
static GetStringFromScriptProc gameExeGetStringFromScriptReal = NULL;

typedef void(__thiscall* MpkConstructorProc)(void* pThis);
static MpkConstructorProc gameExeMpkConstructor = NULL;

typedef int(__cdecl* MountArchiveRNEProc)(int id, const char* mountPoint,
                                          const char* archiveName, int unk01);
static MountArchiveRNEProc gameExeMountArchiveRNE = NULL;
static MountArchiveRNEProc gameExeMountArchiveRNEReal = NULL;

typedef int(__cdecl* MountArchiveRNDProc)(int id, const char* mountPoint,
                                          int unk01, int unk02, int unk03);
static MountArchiveRNDProc gameExeMountArchiveRND = NULL;
static MountArchiveRNDProc gameExeMountArchiveRNDReal = NULL;

typedef int(__fastcall * MountArchiveSGEProc)(int id, const char* archiveName);
static MountArchiveSGEProc gameExeMountArchiveSGE = NULL;
static MountArchiveSGEProc gameExeMountArchiveSGEReal = NULL;

typedef int(__thiscall* CloseAllSystemsProc)(void* pThis);
static CloseAllSystemsProc gameExeCloseAllSystems = NULL;
static CloseAllSystemsProc gameExeCloseAllSystemsReal = NULL;

// correct prototype chosen at runtime
typedef int(__cdecl* SghdGslPngLoadProc)(int textureId, void* png, int size);
typedef int(__cdecl* Sg0GslPngLoadProc)(int textureId, void* png, int size,
                                        int unused0, int unused1);
typedef int(__cdecl* RnGslPngLoadProc)(int textureId, void* png, int size,
                                       float opacity);
static uintptr_t gameExeGslPngload = NULL;

static uintptr_t gameExeGslDDSload = NULL;

static uintptr_t gameExeGslCreateTexture = NULL;
static uintptr_t gameExeGslCreateTextureCLUT = NULL;

typedef int(__thiscall* RnGslDDSLoadProc)(void* surface, void* rawData,
                                          void* surface2);

typedef int(__thiscall* RnGslLoadTextureProc)(void* surface, int width,
                                              int height, int a1, int a2,
                                              int a3, int texFormat, int a4,
                                              int a5, int a6, int a7, int a8,
                                              int a9, int a10, int a11, int a12,
                                              char a13);

typedef int(__thiscall* RnGslLoadTextureClutProc)(void* surface, void* data,
                                                  int width, int height,
                                                  int width2, int height2,
                                                  int texFormat, int a4,
                                                  int a5);

struct DDSFile {
  uint32_t magic;
  DDS_HEADER dds;
  uint8_t dataStart;
};

unsigned int __fastcall ReCreateLoadTextureDDS(void* surface, void* edx,
                                               DDSFile* rawData, int a3) {
  unsigned int fourCC;  // eax
  int height;           // ecx MAPDST
  unsigned int width;   // ebx
  int texFormat;        // esi
  unsigned int result;  // eax

  fourCC = rawData->dds.ddspf.dwFourCC;
  height = rawData->dds.dwHeight;
  width = rawData->dds.dwWidth;
  height = rawData->dds.dwHeight;
  switch (fourCC) {
    case '1TXD':
      texFormat = 384;
      break;
    case '3TXD':
      texFormat = 400;
      break;
    case '5TXD':
      texFormat = 416;
      break;
    case 0:
      if (rawData->dds.ddspf.dwRGBBitCount == 32) {
        if (rawData->dds.ddspf.dwRBitMask == 0xFF)
          texFormat = 1;
        else
          texFormat = 2;
      } else {
        if (rawData->dds.ddspf.dwRGBBitCount == 8) texFormat = 112;
      }
      break;
    default:
      texFormat = height;
      break;
  }
  result = ((RnGslLoadTextureProc)gameExeGslCreateTexture)(
      surface, width, height, 0, 0, 0, texFormat, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
  if (result) return result;
  if ((texFormat & 0xFFF0) == 224 || (texFormat & 0xFFF0) == 240)
    result = 0x80000000;
  else
    result = ((RnGslLoadTextureClutProc)gameExeGslCreateTextureCLUT)(
        surface, &rawData->dataStart, width, height, width, height, texFormat,
        0, 0);
  return result;
}

typedef void(__cdecl* SetSamplerStateWrapperProc)(int sampler, int flags);
static SetSamplerStateWrapperProc gameExeSetSamplerStateWrapper = NULL;
static SetSamplerStateWrapperProc gameExeSetSamplerStateWrapperReal = NULL;

typedef FILE*(__cdecl* FopenProc)(const char* filename, const char* mode);
static FopenProc gameExeClibFopen = NULL;
static FopenProc gameExeClibFopenReal = NULL;

typedef HANDLE(__cdecl* OpenFileProc)(
    LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
static OpenFileProc gameExeOpenFile = NULL;
static OpenFileProc gameExeOpenFileReal = NULL;

typedef BOOL(__cdecl* OpenMyGamesProc)(char* outPath);
static OpenMyGamesProc gameExeOpenMyGames = NULL;  // = 0x48EE50 (C;C)

static mpkObject* gameExeMpkObjects = NULL;
typedef int(__thiscall* MpkFopenByIdProc)(void* pThis, mpkObject* mpk,
                                          int fileId, int unk3);
static MpkFopenByIdProc gameExeMpkFopenById = NULL;
static MpkFopenByIdProc gameExeMpkFopenByIdReal = NULL;

static mgsVFSObject* gameExeFileObjects = NULL;
typedef int(__thiscall* MgsFileOpenProc)(void* pThis, int unused);
static MgsFileOpenProc gameExeMgsFileOpen = NULL;
static MgsFileOpenProc gameExeMgsFileOpenReal = NULL;

typedef int(__cdecl* SNDgetPlayLevelProc)(int a1);
static SNDgetPlayLevelProc gameExeSNDgetPlayLevel = NULL;
static SNDgetPlayLevelProc gameExeSNDgetPlayLevelReal = NULL;

typedef int(__cdecl* PadUpdateDeviceProc)();
static PadUpdateDeviceProc gameExePadUpdateDevice = NULL;
static PadUpdateDeviceProc gameExePadUpdateDeviceReal = NULL;

typedef void(__cdecl* SetAreaParams)(int areaId, short params[24]);
static SetAreaParams gameExeSetAreaParams = NULL;
static SetAreaParams gameExeSetAreaParamsReal = NULL;

static int* gameExeWavData = (int*)NULL;
static void** gameExeVoiceTable = (void**)NULL;

static uintptr_t gameExeControllerGuid = NULL;

struct RNEVertex {
  float x, y, z;
  float r, g, b, a;
  float u, v;
};

typedef int(__cdecl* DrawTriangleListProc)(int a1, RNEVertex* a2, int a3,
                                           int a4);
static DrawTriangleListProc gameExeDrawTriangleList = NULL;
static DrawTriangleListProc gameExeDrawTriangleListReal = NULL;

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
  mpkObject* pCurrentMpk;
  char char138;
  char gap139[3];
  char char13C;
  char gap13D[11];
  FILE* cFile;
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
  void* pvoid38;
  int playbackState;
  char gap40[4];
  mgsFileHandle* file;
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
  void* pcmOutput;
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
static CPlayer* gameExeAudioPlayers = NULL;

typedef int(__thiscall* ReadOggMetadataProc)(CPlayer* pThis);
static ReadOggMetadataProc gameExeReadOggMetadata = NULL;
static ReadOggMetadataProc gameExeReadOggMetadataReal = NULL;

MgsD3D9State* gameExePMgsD3D9State = NULL;
MgsD3D11State* gameExePMgsD3D11State = NULL;

static IDirect3D9Ex** gameExePpD3D9Ex = NULL;
static D3DPRESENT_PARAMETERS* gameExePPresentParameters = NULL;

static uint16_t* gameExeScreenWidth = NULL;
static uint16_t* gameExeScreenHeight = NULL;
static uintptr_t gameExeTextureLoadInit1 = NULL;
static uintptr_t gameExeTextureLoadInit2 = NULL;
static uintptr_t gameExeMpkMount = NULL;
static uintptr_t gameExePCurrentBgm = NULL;
static uintptr_t gameExePLoopBgm = NULL;
static uintptr_t gameExePShouldPlayBgm = NULL;
// scroll height is +6A78
static int* gameExeScriptIdsToFileIds = NULL;

static std::string stringReplacementTable;
static mpkObject* c0dataMpk = NULL;
static mgsVFSObject* c0dataCpk = NULL;

void* surfaceArray;

int* gameExeScrWork = (int*)NULL;

namespace lb {

GameID SurfaceWrapper::game = GameID::SG;

int __cdecl earlyInitHook(int unk0, int unk1);
int __fastcall mpkFopenByIdHook(void* pThis, void* EDX, mpkObject* mpk,
                                int fileId, int unk3);
int __fastcall mgsFileOpenHook(mgsFileLoader* pThis, void* dummy, int unused);
const char* __cdecl getStringFromScriptHook(int scriptId, int stringId);
int __fastcall closeAllSystemsHook(void* pThis, void* EDX);
FILE* __cdecl clibFopenHook(const char* filename, const char* mode);
HANDLE __cdecl openFileHook(LPCSTR lpFileName, DWORD dwDesiredAccess,
                            DWORD dwShareMode,
                            LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                            DWORD dwCreationDisposition,
                            DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
void __cdecl setSamplerStateWrapperHook(int sampler, int flags);
int __fastcall readOggMetadataHook(CPlayer* pThis, void* EDX);
BOOL __cdecl openMyGamesHook(char* outPath);
int __cdecl SNDgetPlayLevelHook(int a1);
int __cdecl PadUpdateDeviceHook();
int __cdecl mountArchiveHookRNE(int id, const char* mountPoint,
                                const char* archiveName, int unk01);
int __cdecl mountArchiveHookRND(int id, const char* mountPoint, int unk01,
                                int unk02, int unk03);
unsigned int __cdecl DrawTriangleList(int a1, RNEVertex* a2, int a3, int a4);
int __fastcall mountArchiveHookSGE(int id, const char* archiveName);
void gameInit() {
  SetProcessDPIAware();
  std::ifstream in("languagebarrier\\stringReplacementTable.bin",
                   std::ios::in | std::ios::binary);
  in.seekg(0, std::ios::end);
  stringReplacementTable.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&stringReplacementTable[0], stringReplacementTable.size());
  in.close();

  globalTextReplacementsInit();
  if (config["gamedef"].count("gameVideoMiddleware") &&
      config["gamedef"]["gameVideoMiddleware"].get<std::string>() == "cri") {
    criManaModInit();
  } else {
    binkModInit();
  }
  gameExeTextureLoadInit1 = sigScan("game", "textureLoadInit1");
  gameExeTextureLoadInit2 = sigScan("game", "textureLoadInit2");
  gameExeGslPngload = sigScan("game", "gslPngload");

  gameExeGslCreateTexture = sigScan("game", "createTexture");
  gameExeGslCreateTextureCLUT = sigScan("game", "createTextureCLUT");
  if (config["gamedef"]["signatures"]["game"].count("surfaceArray") == 1)
    surfaceArray = *(void**)sigScan("game", "surfaceArray");

  gameExeEarlyInit = (EarlyInitProc)sigScan("game", "earlyInit");
  if (config["gamedef"]["signatures"]["game"].count("useOfPCurrentBgm") == 1)
    gameExePCurrentBgm = sigScan("game", "useOfPCurrentBgm");
  if (config["gamedef"]["signatures"]["game"].count("useOfPLoopBgm") == 1)
    gameExePLoopBgm = sigScan("game", "useOfPLoopBgm");
  if (config["gamedef"]["signatures"]["game"].count("useOfPShouldPlayBgm") == 1)
    gameExePShouldPlayBgm = sigScan("game", "useOfPShouldPlayBgm");

  if (true) {
    if (config["gamedef"].count("gameArchiveMiddleware") == 1 &&
        config["gamedef"]["gameArchiveMiddleware"].get<std::string>() ==
            "cri") {
      if (config["gamedef"]["signatures"]["game"].count("mountArchiveRNE") ==
          1) {
        if (!scanCreateEnableHook("game", "mountArchiveRNE",
                                  (uintptr_t*)&gameExeMountArchiveRNE,
                                  (LPVOID)mountArchiveHookRNE,
                                  (LPVOID*)&gameExeMountArchiveRNEReal))
          return;
      } else if (config["gamedef"]["signatures"]["game"].count(
                     "mountArchiveRND") == 1) {
        if (!scanCreateEnableHook("game", "mountArchiveRND",
                                  (uintptr_t*)&gameExeMountArchiveRND,
                                  (LPVOID)mountArchiveHookRND,
                                  (LPVOID*)&gameExeMountArchiveRNDReal))
          return;
      } else if (config["gamedef"]["signatures"]["game"].count(
                     "mountArchiveSGE") == 1) {
        lb::SurfaceWrapper::game = SGE;

        if (!scanCreateEnableHook("game", "mountArchiveSGE",
                                  (uintptr_t*)&gameExeMountArchiveSGE,
                                  (LPVOID)mountArchiveHookSGE,
                                  (LPVOID*)&gameExeMountArchiveSGEReal))
          return;
      }
    } else {
      gameExeMpkMount = sigScan("game", "mpkMount");
      gameExeMpkConstructor =
          (MpkConstructorProc)sigScan("game", "mpkConstructor");
    }
  }
  gameExeGetFlag = (GetFlagProc)sigScan("game", "getFlag");
  gameExeSetFlag = (SetFlagProc)sigScan("game", "setFlag");
  gameExeChkViewDic = (ChkViewDicProc)sigScan("game", "chkViewDic");

  scanCreateEnableHook("game", "recreateDDSSurface",
                       (uintptr_t*)&gameExeGslDDSload,
                       (LPVOID)ReCreateLoadTextureDDS, NULL);

  // TODO: fault tolerance - we don't need to call it quits entirely just
  // because one *feature* can't work
  if (!scanCreateEnableHook("game", "earlyInit", (uintptr_t*)&gameExeEarlyInit,
                            (LPVOID)&earlyInitHook,
                            (LPVOID*)&gameExeEarlyInitReal) ||
      !scanCreateEnableHook("game", "getStringFromScript",
                            (uintptr_t*)&gameExeGetStringFromScript,
                            (LPVOID)getStringFromScriptHook,
                            (LPVOID*)&gameExeGetStringFromScriptReal) ||
      !scanCreateEnableHook("game", "clibFopen", (uintptr_t*)&gameExeClibFopen,
                            (LPVOID)clibFopenHook,
                            (LPVOID*)&gameExeClibFopenReal))

    scanCreateEnableHook("game", "openMyGames", (uintptr_t*)&gameExeOpenMyGames,
                         (LPVOID)openMyGamesHook, NULL);

  scanCreateEnableHook("game", "openFile", (uintptr_t*)&gameExeOpenFile,
                       (LPVOID)openFileHook, (LPVOID*)&gameExeOpenFileReal);

  memoryManagementInit();

  if (lb::SurfaceWrapper::game != SGE) {
    scriptInit();
    if (config["gamedef"]["gameDxVersion"].get<std::string>() == "dx9") {
      gameExePMgsD3D9State =
          *((MgsD3D9State**)sigScan("game", "useOfMgsD3D9State"));
      gameExePpD3D9Ex = *((IDirect3D9Ex***)sigScan("game", "useOfD3D9Ex"));
      gameExePPresentParameters =
          *((D3DPRESENT_PARAMETERS**)sigScan("game", "useOfPresentParameters"));
    }
  }

  if (config["patch"]["textureFiltering"].get<bool>() == true) {
    /*LanguageBarrierLog("Forcing bilinear filtering");
    uint8_t* branch = (uint8_t*)sigScan("game", "textureFilteringBranch");
    if (branch != NULL) {
      // original code: if (stuff) { setTextureFiltering(Point) } else {
      // setTextureFiltering(Linear) }
      // patch 'je' to 'jmp' -> always go to else block
      memset_perms(branch, INST_JMP_SHORT, 1);
    }*/
    scanCreateEnableHook("game", "setSamplerStateWrapper",
                         (uintptr_t*)&gameExeSetSamplerStateWrapper,
                         (LPVOID)setSamplerStateWrapperHook,
                         (LPVOID*)&gameExeSetSamplerStateWrapperReal);
  }

  if (config["patch"]["exitBlackScreenFix"].get<bool>() == true) {
    if (!scanCreateEnableHook(
            "game", "closeAllSystems", (uintptr_t*)&gameExeCloseAllSystems,
            (LPVOID)closeAllSystemsHook, (LPVOID*)&gameExeCloseAllSystemsReal))
      return;
  }

  if (config["gamedef"]["hasAutoSkipHide"].get<bool>() == true &&
      config["patch"]["shouldHideAutoSkip"].get<bool>() == true) {
    LanguageBarrierLog("Hiding auto/skip buttons");
    uint8_t* branch = (uint8_t*)sigScan("game", "autoSkipHideBranch");
    if (branch != NULL) {
      memset_perms(branch, INST_JMP_SHORT, 1);
    }
  }
  if (config["patch"].count("overrideLoopMetadata") == 1 &&
      config["patch"]["overrideLoopMetadata"].get<bool>() == true) {
    scanCreateEnableHook(
        "game", "readOggMetadata", (uintptr_t*)&gameExeReadOggMetadata,
        (LPVOID)readOggMetadataHook, (LPVOID*)&gameExeReadOggMetadataReal);
  }

  if (config["patch"].count("fixPlayLevelNullDeref") == 1 &&
      config["patch"]["fixPlayLevelNullDeref"].get<bool>() == true) {
    gameExeVoiceTable = (void**)sigScan("game", "useOfVoiceTable");
    gameExeWavData = (int*)sigScan("game", "useOfWavData");

    if (gameExeVoiceTable == 0 || gameExeWavData == 0 ||
        !scanCreateEnableHook(
            "game", "SNDgetPlayLevel", (uintptr_t*)&gameExeSNDgetPlayLevel,
            (LPVOID)SNDgetPlayLevelHook, (LPVOID*)&gameExeSNDgetPlayLevelReal))
      return;
  }
    gameExeScriptIdsToFileIds =
        (int*)sigScan("game", "useOfScriptIdsToFileIds");
    if (config["gamedef"]["signatures"]["game"].count("useOfAudioPlayers") == 1)
      gameExeAudioPlayers = *(CPlayer**)sigScan("game", "useOfAudioPlayers");
    if (config["gamedef"]["signatures"]["game"].count("useOfMpkObjects") == 1)
      gameExeMpkObjects = (mpkObject*)sigScan("game", "useOfMpkObjects");
    if (config["gamedef"]["signatures"]["game"].count("useOfFileObjects") == 1)
      gameExeFileObjects = (mgsVFSObject*)sigScan("game", "useOfFileObjects");

    if (config["patch"].value<bool>("disableUnconfiguredControllers", true)) {
      gameExeControllerGuid = sigScan("game", "useOfControllerGuid");
      if (gameExeControllerGuid != NULL) {  // signatures present
        scanCreateEnableHook(
            "game", "PadUpdateDevice", (uintptr_t*)&gameExePadUpdateDevice,
            (LPVOID)PadUpdateDeviceHook, (LPVOID*)&gameExePadUpdateDeviceReal);
      }
    }

  if (config["gamedef"].count("gameVideoMiddleware") &&
      config["gamedef"]["gameVideoMiddleware"].get<std::string>() == "cri") {
    criManaModInit();
  } else {
    binkModInit();
  }

  if (config["patch"].count("overrideAreaParams")) {
    scanCreateEnableHook(
        "game", "setAreaParams", (uintptr_t*)&gameExeSetAreaParams,
        (LPVOID)setAreaParamsHook, (LPVOID*)&gameExeSetAreaParamsReal);
  }

  if (config["patch"].value<bool>("enableDebugger", false)) {
    initScriptDebuggerHooks();
  }
}

// earlyInit is called after all the subsystems have been initialised but before
// the game 'starts' proper
// so, probably a good place to do all of our initialisation that requires
// interacting with them
int __cdecl earlyInitHook(int unk0, int unk1) {
  int retval = gameExeEarlyInitReal(unk0, unk1);

  try {
    if (config["patch"]["hookText"].get<bool>() == true) {
      gameTextInit();
    }

    if (config["gamedef"]["gameDxVersion"].get<std::string>() == "dx11") {
      gameExePMgsD3D11State =
          (**(MgsD3D11State***)sigScan("game", "useOfMgsD3D11State"));
    }

    if (config["patch"].count("useNewTextSystem") == 1 &&
        config["patch"]["useNewTextSystem"].get<bool>() == true) {
      TextRendering::Get().loadCache();
      TextRendering::Get().enableReplacement();
    }

    gameExeScrWork = (int*)sigScan("game", "useOfScrWork");
    scanCreateEnableHook(
        "game", "exitApplication", (uintptr_t*)&gameExeCloseAllSystems,
        (LPVOID)closeAllSystemsHook, (LPVOID*)&gameExeCloseAllSystemsReal);

    std::string lbDir =
        WideTo8BitPath(GetGameDirectoryPath() + L"\\languagebarrier");

    if (config["gamedef"].count("gameArchiveMiddleware") == 1 &&
        config["gamedef"]["gameArchiveMiddleware"].get<std::string>() ==
            "cri") {
      // 15 is just a random ID, let's hope it won't ever get used
      if (config["gamedef"]["signatures"]["game"].count("mountArchiveRNE") ==
          1) {
        gameExeMountArchiveRNEReal(C0DATA_MOUNT_ID, "C0DATA",
                                   "languagebarrier\\C0DATA", 0);
      } else if (config["gamedef"]["signatures"]["game"].count(
                     "mountArchiveRND") == 1) {
        gameExeMountArchiveRNDReal(C0DATA_MOUNT_ID, "languagebarrier\\c0data",
                                   0, 1, 0);
      } else if (config["gamedef"]["signatures"]["game"].count(
                     "mountArchiveSGE") == 1) {
        C0DATA_MOUNT_ID++;
        gameExeMountArchiveSGEReal(C0DATA_MOUNT_ID, "..\\languagebarrier\\c0data");
      }
      LanguageBarrierLog("c0data mounted");

      c0dataCpk = &gameExeFileObjects[C0DATA_MOUNT_ID];

      if (!scanCreateEnableHook(
              "game", "mgsFileOpen", (uintptr_t*)&gameExeMgsFileOpen,
              (LPVOID)&mgsFileOpenHook, (LPVOID*)&gameExeMgsFileOpenReal))
        return retval;
    } else {
      c0dataMpk = gameMountMpk("C0DATA", lbDir.c_str(), "c0data.mpk");
      LanguageBarrierLog("c0data.mpk mounted");

      if (!scanCreateEnableHook(
              "game", "mpkFopenById", (uintptr_t*)&gameExeMpkFopenById,
              (LPVOID)&mpkFopenByIdHook, (LPVOID*)&gameExeMpkFopenByIdReal))
        return retval;
    }

    if (config["patch"]["redoDialogueWordwrap"].get<bool>() == true) {
      dialogueWordwrapInit();
    }

    if (config["patch"].count("RNEMouseInput") == 1 &&
        config["patch"]["RNEMouseInput"].get<bool>() == true) {
      rne::customInputInit();
    }

    if (config["patch"].count("RNDMouseInput") == 1 &&
        config["patch"]["RNDMouseInput"].get<bool>() == true) {
      rnd::customInputInit();
    }

    if (config["patch"].count("RNENameTagFix") == 1 &&
        config["patch"]["RNENameTagFix"].get<bool>() == true) {
      float* dotX = (float*)sigScan("game", "useOfRNENameTagDotX");
      DWORD oldProtect;
      VirtualProtect(dotX, sizeof(float), PAGE_READWRITE, &oldProtect);
      *dotX = 1638.0f;
      VirtualProtect(dotX, sizeof(float), oldProtect, &oldProtect);
    }

    if (config["patch"].count("rndMiscFixes") == 1 &&
        config["patch"]["rndMiscFixes"].get<bool>() == true) {
      // Make config voice names area wider
      float* width = (float*)sigScan("game", "useOfRNDConfigVoiceNameWidth");
      DWORD oldProtect;
      VirtualProtect(width, sizeof(float), PAGE_READWRITE, &oldProtect);
      *width = 311.0f;
      VirtualProtect(width, sizeof(float), oldProtect, &oldProtect);
      width = (float*)sigScan("game", "useOfRNDConfigVoiceNameUnderlineWidth");
      VirtualProtect(width, sizeof(float), PAGE_READWRITE, &oldProtect);
      *width = 515.0f;
      VirtualProtect(width, sizeof(float), oldProtect, &oldProtect);
    }

    if (config["patch"].count("RNERenderTargetClearColourFix") == 1 &&
        config["patch"]["RNERenderTargetClearColourFix"].get<bool>() == true) {
      float* r = (float*)sigScan("game", "useOfRNEClearColourR");
      DWORD oldProtect;
      VirtualProtect(r, sizeof(float), PAGE_READWRITE, &oldProtect);
      *r = 0.0f;
      VirtualProtect(r, sizeof(float), oldProtect, &oldProtect);
    }

    if (config["patch"].count("RNEAddOriginalOPToMovieLibrary") == 1 &&
        config["patch"]["RNEAddOriginalOPToMovieLibrary"].get<bool>() == true) {
      int* ids = (int*)sigScan("game", "useOfRNEMovieLibraryIndexes");
      ids[1] = 1;
      ids[2] = 22;
    }

    if (config["patch"].value<bool>("enableDebugger", false)) {
      earlyInitScriptDebuggerHooks();
    }

    gameExeScreenHeight =
        reinterpret_cast<uint16_t*>(sigScan("game", "ScreenHeight"));
    gameExeScreenWidth =
        reinterpret_cast<uint16_t*>(sigScan("game", "ScreenWidth"));
    if (!scanCreateEnableHook("game", "DrawTriangleListHook",
                              (uintptr_t*)&gameExeDrawTriangleList,
                              (LPVOID)&DrawTriangleList,
                              (LPVOID*)&gameExeDrawTriangleListReal))
      return retval;
  } catch (std::exception& e) {
    MessageBoxA(NULL, e.what(), "LanguageBarrier exception", MB_ICONSTOP);
  }

  return retval;
}

int __fastcall mpkFopenByIdHook(void* pThis, void* EDX, mpkObject* mpk,
                                int fileId, int unk3) {
  char* mpkFilename = (char*)&mpk->filename;
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

std::string mountArchiveHookPart(const char* mountPoint) {
  if (config["patch"].count("archiveRedirection") == 1 &&
      config["patch"]["archiveRedirection"].count(mountPoint) == 1) {
    std::stringstream newPath;
    newPath
        << "languagebarrier\\"
        << config["patch"]["archiveRedirection"][mountPoint].get<std::string>();

    std::stringstream logstr;
    if (!std::filesystem::exists(newPath.str())) {
      logstr << newPath.str() << " does not exist!";
      LanguageBarrierLog(logstr.str());
      return mountPoint;
    }
    logstr << "redirecting physical fopen " << mountPoint << " to "
           << newPath.str();
    LanguageBarrierLog(logstr.str());

    return newPath.str();
  }

  return mountPoint;
}

int __fastcall mountArchiveHookSGE(int id, const char* archiveFile) {
  return gameExeMountArchiveSGEReal(id, archiveFile);
}

int __cdecl mountArchiveHookRNE(int id, const char* mountPoint,
                                const char* archiveName, int unk01) {
  std::string path = mountArchiveHookPart(mountPoint);
  return gameExeMountArchiveRNEReal(id, mountPoint, path.c_str(), unk01);
}

int __cdecl mountArchiveHookRND(int id, const char* mountPoint, int unk01,
                                int unk02, int unk03) {
  std::string path = mountArchiveHookPart(mountPoint);
  return gameExeMountArchiveRNDReal(id, path.c_str(), unk01, unk02, unk03);
}

int __fastcall mgsFileOpenHook(mgsFileLoader* pThis, void* dummy, int unused) {
  char* fileName = pThis->fileName;
  int fileId = pThis->fileId;
  char* archiveName = pThis->vfsObject->archiveName;

  if (pThis->vfsObject && (fileId > 0 || (fileName && fileName[0] != '\0'))) {
    std::stringstream logstr;
    logstr << "mgsFileOpen(" << archiveName << ", 0x" << std::hex << fileId
           << ")" << std::dec;
#ifdef _DEBUG
    LanguageBarrierLog(logstr.str());
#endif
    if (config["patch"].count("fileRedirection") == 1 &&
        config["patch"]["fileRedirection"].count(archiveName) > 0) {
      std::string key;
      if (fileId == -1) {
        key = fileName;
      } else {
        key = std::to_string(fileId);
      }
      if (config["patch"]["fileRedirection"][archiveName].count(key) == 1) {
        auto red = config["patch"]["fileRedirection"][archiveName][key];
        if (red.type() == json::value_t::number_integer ||
            red.type() == json::value_t::number_unsigned) {
          int newFileId = red.get<int>();
          logstr << " redirected to c0data, 0x" << std::hex << newFileId;
#ifdef _DEBUG
          LanguageBarrierLog(logstr.str());
#endif
          pThis->fileId = newFileId;
          pThis->vfsObject = c0dataCpk;
          if (fileId == -1) pThis->loadMode = 2;
          return gameExeMgsFileOpenReal(pThis, unused);
        }
      }
    }
  }

  return gameExeMgsFileOpenReal(pThis, unused);
}

const char* __cdecl getStringFromScriptHook(int scriptId, int stringId) {
  int fileId = gameExeScriptIdsToFileIds[scriptId];
  const char* result = gameExeGetStringFromScriptReal(scriptId, stringId);
  if (config["patch"].count("stringRedirection") == 1) {
    const json& targets = config["patch"]["stringRedirection"];
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
        uint32_t offset = ((uint32_t*)stringReplacementTable.c_str())[repId];
        result = &(stringReplacementTable.c_str()[offset]);
      }
    }
  }
  return processTextReplacements(result, fileId, stringId);
}

int __fastcall closeAllSystemsHook(void* pThis, void* EDX) {
  // IDA thinks this is thiscall
  // I'm not so sure it actually takes a parameter, but better safe than sorry,
  // right?

  if (config["patch"].count("useNewTextSystem") == 1 &&
      config["patch"]["useNewTextSystem"].get<bool>() == true) {
    TextRendering::Get().saveCache();
  }

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

FILE* clibFopenHook(const char* filename, const char* mode) {
  const char* tmp = filename;
  if (strrchr(tmp, '\\')) tmp = strrchr(tmp, '\\') + 1;
  if (strrchr(tmp, '/')) tmp = strrchr(tmp, '/') + 1;

  if (config["patch"].count("physicalFileRedirection") == 1 &&
      config["patch"]["physicalFileRedirection"].count(tmp) == 1) {
    std::stringstream newPath;
    newPath
        << "languagebarrier\\"
        << config["patch"]["physicalFileRedirection"][tmp].get<std::string>();

    std::stringstream logstr;
    if (!std::filesystem::exists(newPath.str())) {
      logstr << newPath.str() << " does not exist!";
      LanguageBarrierLog(logstr.str());
      return gameExeClibFopenReal(filename, mode);
    }
    logstr << "redirecting physical fopen " << tmp << " to " << newPath.str();
    LanguageBarrierLog(logstr.str());

    return gameExeClibFopenReal(newPath.str().c_str(), mode);
  }

  return gameExeClibFopenReal(filename, mode);
}

HANDLE openFileHook(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                    DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                    HANDLE hTemplateFile) {
  const char* tmp = lpFileName;
  if (strrchr(tmp, '\\')) tmp = strrchr(tmp, '\\') + 1;
  if (strrchr(tmp, '/')) tmp = strrchr(tmp, '/') + 1;

  if (config["patch"].count("physicalFileRedirection") == 1 &&
      config["patch"]["physicalFileRedirection"].count(tmp) == 1) {
    std::stringstream newPath;
    newPath
        << "languagebarrier\\"
        << config["patch"]["physicalFileRedirection"][tmp].get<std::string>();

    std::stringstream logstr;
    logstr << "redirecting physical openFile " << tmp << " to "
           << newPath.str();
    LanguageBarrierLog(logstr.str());

    return gameExeOpenFileReal(newPath.str().c_str(), dwDesiredAccess,
                               dwShareMode, lpSecurityAttributes,
                               dwCreationDisposition, dwFlagsAndAttributes,
                               hTemplateFile);
  }

  return gameExeOpenFileReal(lpFileName, dwDesiredAccess, dwShareMode,
                             lpSecurityAttributes, dwCreationDisposition,
                             dwFlagsAndAttributes, hTemplateFile);
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
			*(DWORD*)&mipmapBias);
#endif
}

int __fastcall readOggMetadataHook(CPlayer* pThis, void* EDX) {
  int ret = gameExeReadOggMetadataReal(pThis);
  if (pThis->file != NULL && pThis->file->pCurrentMpk != NULL) {
    char* archiveFn = (char*)pThis->file->pCurrentMpk->filename;
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

BOOL __cdecl openMyGamesHook(char* outPath) {
  // The game knows only ANSI fopen(). Unfortunately, this prevents it from
  // finding save data for users with out-of-locale usernames. So let's use our
  // safe conversion.

  const std::wstring MyGames(L"\\My Games\\");

  PWSTR myDocumentsPath;
  SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &myDocumentsPath);

  std::wstring myGamesPath(std::wstring(myDocumentsPath) + MyGames);

  BOOL result = CreateDirectoryW(myGamesPath.c_str(), 0);

  std::string myGames8Bit = WideTo8BitPath(myGamesPath);
  int sz = myGames8Bit.size();
  if (sz > MAX_PATH - 1) {
    LanguageBarrierLog("My Games path too long, uh oh...");
  }
  strncpy_s(outPath, MAX_PATH - 1, myGames8Bit.c_str(), sz);
  outPath[sz] = '\0';

  CoTaskMemFree(myDocumentsPath);
  return result;
}

int __cdecl SNDgetPlayLevelHook(int a1) {
  // Bounds check from original code
  if (a1 < 3 || a1 > 6) return 0;

  // Bounds checks from S;G Steam
  // I'm putting this one back in because the null check below apparently didn't
  // work for a tester.
  // No idea if that's the actual issue, but...
  int v4 = gameExeWavData[38 * a1 + 25];
  int v5 = gameExeWavData[38 * a1 + 24];
  if (v4 <= 0 || v5 <= 0 || v4 > v5) return 0;

  // What we actually care about
  if (*gameExeVoiceTable == NULL) {
    return 0;
  } else {
    return gameExeSNDgetPlayLevelReal(a1);
  }
}

int __cdecl PadUpdateDeviceHook() {
  static const char emptyGUID[0x28] = {0};
  // if controller GUID is null, disable controllers
  if (memcmp((void*)gameExeControllerGuid, emptyGUID, 0x28) == 0) return 0;

  return gameExePadUpdateDeviceReal();
}

void __cdecl setAreaParamsHook(int areaId, short data[24]) {
  try {
    const json& hooked = config["patch"]["overrideAreaParams"];
    std::string key = std::to_string(areaId);
    json::const_iterator it = hooked.find(key);
    if (it != hooked.end()) {
      std::vector<short> patchedData = it->get<std::vector<short>>();
      gameExeSetAreaParamsReal(areaId, patchedData.data());
      return;
    }
  } catch (std::exception&) {
    // not an array of integers, just ignore it
  }
  gameExeSetAreaParamsReal(areaId, data);
}

// TODO: I probably shouldn't be writing these in assembly given it looks like
// they're all cdecl or thiscall anyway
void gameLoadTexture(uint16_t textureId, void* buffer, size_t sz) {
  if (config["gamedef"]["gslPngLoadVersion"].get<std::string>() == "sghd") {
    ((SghdGslPngLoadProc)gameExeGslPngload)(textureId, buffer, sz);
  } else if (config["gamedef"]["gslPngLoadVersion"].get<std::string>() ==
             "sg0") {
    ((Sg0GslPngLoadProc)gameExeGslPngload)(textureId, buffer, sz, 0, 0);
  } else if (config["gamedef"]["gslPngLoadVersion"].get<std::string>() ==
             "rn") {
    ((RnGslPngLoadProc)gameExeGslPngload)(textureId, buffer, sz, 1.0f);
    if (textureId == 400) {
      return;
    }
  }
}

// returns an archive object
mpkObject* gameMountMpk(char const* mountpoint, char const* directory,
                        char const* filename) {
  void* retval = calloc(1, 0x3a8);
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
  return (mpkObject*)retval;
}
void gameSetBgm(uint32_t fileId, bool shouldLoop) {
  // There are probably nicer ways of doing this, but this is the easiest one
  // that avoids a race
  *(uint32_t*)gameExePCurrentBgm = fileId;
  *(uint32_t*)gameExePLoopBgm = shouldLoop;
}
uint32_t gameGetBgm() { return *(uint32_t*)gameExePCurrentBgm; }
bool gameGetBgmShouldLoop() { return *(uint32_t*)gameExePLoopBgm; }
void gameSetBgmShouldPlay(bool shouldPlay) {
  *(uint32_t*)gameExePShouldPlayBgm = shouldPlay;
}
bool gameGetBgmShouldPlay() { return *(uint32_t*)gameExePShouldPlayBgm; }
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
void ApplyOrthographicProjection(RNEVertex* vertices, int count, float width,
                                 float height) {
  float scaleX = 2.0f / width;
  float scaleY = 2.0f / height;

  for (int i = 0; i < count; i++) {
    vertices[i].x = vertices[i].x * (scaleX)-1.0f;
    vertices[i].y = vertices[i].y * -(scaleY) + 1.0f;
    vertices[i].u /= 1024.0f;
    vertices[i].v /= 1024.0f;
  }
}

unsigned int __cdecl DrawTriangleList(int a1, RNEVertex* a2, int a3, int a4) {
  if (a1 == 82 && a4 == 1)
    // Use projection matrix to convert a2 position to clip space
    ApplyOrthographicProjection(a2, a3 * 3, *gameExeScreenWidth,
                                *gameExeScreenHeight);

  return gameExeDrawTriangleListReal(a1, a2, a3, a4);
}
}  // namespace lb
