#include "CustomInputRNE.h"
#include "Config.h"
#include "Game.h"
#include "GameText.h"
#include "LanguageBarrier.h"
#include "SigScan.h"

typedef struct
{
  uint32_t mouseButtons1;
  uint32_t dword4;
  uint32_t dword8;
  uint32_t dwordC;
  uint32_t mouseButtonsHeld;
  uint32_t dword14;
  uint32_t dword18;
  uint32_t dword1C;
  uint32_t dword20;
  uint32_t dword24;
  uint32_t dword28;
  uint32_t dword2C;
  uint32_t mouseButtons;
  uint32_t dword34;
  int32_t mouseXAxis;
  int32_t mouseYAxis;
  int32_t mouseZAxis;
  uint32_t mouseX;
  uint32_t mouseY;
  uint32_t scaledMouseX;
  uint32_t scaledMouseY;
  uint32_t dword54;
  uint32_t dword58;
  uint32_t dword5C;
  uint32_t dword60;
  uint32_t dword64;
  uint32_t dword68;
  uint32_t dword6C;
  uint32_t dword70;
  uint32_t dword74;
  uint8_t gap78[4];
  uint32_t dword7C;
  uint32_t dword80;
  uint32_t dword84;
  uint32_t dword88;
  uint32_t dword8C;
  uint32_t dword90;
  uint8_t gap94[4];
  uint32_t dword98;
  uint32_t dword9C;
  uint8_t gapA0[4];
  uint32_t dwordA4;
  uint32_t dwordA8;
  uint32_t dwordAC;
  uint32_t dwordB0;
  uint32_t dwordB4;
  uint32_t dwordB8;
  uint8_t gapBC[4];
  uint32_t dwordC0;
  uint32_t dwordC4;
  uint8_t gapC8[8];
  uint32_t mouseButtonsHeld1;
  uint32_t dwordD4;
  uint32_t dwordD8;
  uint32_t dwordDC;
  uint8_t gapE0[256];
  uint8_t mouseMoving;
  uint8_t mouseFocused;
} MgsInputObj_t;

typedef int(__thiscall* MgsInputExecuteServerProc)(MgsInputObj_t* pThis);
static MgsInputExecuteServerProc gameExeMgsInputExecuteServer = NULL;
static MgsInputExecuteServerProc gameExeMgsInputExecuteServerReal = NULL;

typedef int(__cdecl* PADmainProc)();
static PADmainProc gameExePADmain = NULL;
static PADmainProc gameExePADmainReal = NULL;

typedef int(__cdecl* InstTitleMenuProc)(void*);
static InstTitleMenuProc gameExeInstTitleMenu = NULL;
static InstTitleMenuProc gameExeInstTitleMenuReal = NULL;

typedef int(__cdecl* InstSaveMenuProc)(uint8_t*);
static InstSaveMenuProc gameExeInstSaveMenu = NULL;
static InstSaveMenuProc gameExeInstSaveMenuReal = NULL;

typedef int(__cdecl* InstSystemMesProc)(uint8_t*);
static InstSystemMesProc gameExeInstSystemMes = NULL;
static InstSystemMesProc gameExeInstSystemMesReal = NULL;

typedef int(__cdecl* InstSystemMenuProc)(void*);
static InstSystemMenuProc gameExeInstSystemMenu = NULL;
static InstSystemMenuProc gameExeInstSystemMenuReal = NULL;

typedef int(__cdecl* InstTipsProc)(void*);
static InstTipsProc gameExeInstTips = NULL;
static InstTipsProc gameExeInstTipsReal = NULL;

typedef int(__cdecl* MovieModeDispProc)(void*);
static MovieModeDispProc gameExeMovieModeDisp = NULL;
static MovieModeDispProc gameExeMovieModeDispReal = NULL;

typedef int(__cdecl* AlbumMenuMainProc)();
static AlbumMenuMainProc gameExeAlbumMenuMain = NULL;
static AlbumMenuMainProc gameExeAlbumMenuMainReal = NULL;

typedef int(__cdecl* CGViewModeMainProc)();
static CGViewModeMainProc gameExeCGViewModeMain = NULL;
static CGViewModeMainProc gameExeCGViewModeMainReal = NULL;

typedef void(__cdecl* PokecomMainMenuMainProc)();
static PokecomMainMenuMainProc gameExePokecomMainMenuMain = NULL;
static PokecomMainMenuMainProc gameExePokecomMainMenuMainReal = NULL;

typedef int(__cdecl* MusicMenuMainProc)();
static MusicMenuMainProc gameExeMusicMenuMain = NULL;
static MusicMenuMainProc gameExeMusicMenuMainReal = NULL;

typedef int(__cdecl* BacklogMenuMainProc)();
static BacklogMenuMainProc gameExeBacklogMenuMain = NULL;
static BacklogMenuMainProc gameExeBacklogMenuMainReal = NULL;

typedef int(__cdecl* BacklogRecalcMovementProc)();

typedef int(__cdecl* GetScreenCoordsProc)(int, float*, float*, float, float, float);
typedef int(__cdecl* CountSC3CharactersProc)(uint8_t*, int, int);
typedef uint8_t*(__cdecl* GetSC3StringByIDProc)(int, int);

typedef void(__cdecl* TwipoMainProc)();
static TwipoMainProc gameExeTwipoMain = NULL;
static TwipoMainProc gameExeTwipoMainReal = NULL;

typedef int(__cdecl* PokecomDocMainProc)();
static PokecomDocMainProc gameExePokecomDocMain = NULL;
static PokecomDocMainProc gameExePokecomDocMainReal = NULL;

typedef int(__cdecl* PokecomMapMainProc)();
static PokecomMapMainProc gameExePokecomMapMain = NULL;
static PokecomMapMainProc gameExePokecomMapMainReal = NULL;

typedef int(__cdecl* PokecomARMainProc)();
static PokecomARMainProc gameExePokecomARMain = NULL;
static PokecomARMainProc gameExePokecomARMainReal = NULL;

bool PointedThisFrame = false;
bool LockMouseControls = false;
bool MouseExclusive = false;
uint32_t CarryInputToTheNextFrame = 0;

MgsInputObj_t* InputObject = NULL; //(MgsInputObj_t*)0x2A3F6F0;
uint8_t* MouseEnabled = NULL; //(uint8_t*)0x2706461;
HWND* WindowHandle = NULL; //(HWND*)0x6F8CC4;
uint8_t* MousePointing = NULL; //(uint8_t*)0x2E3F8B4;

int* TitleMenuSelectionIndex = NULL; //(int*)0xC6DA48;
int* SubMenuSelIndex = NULL; //(int*)0xC6DA64;
int* LoadMenuSelIndex = NULL; //(int*)0xC6DA50;
int* ExtrasMenuSelIndex = NULL; //(int*)0xC6DA54;
int* TitleSubMenuAnimCounter = NULL; //(int*)0xC6DA70;

int* MovieModeSelectionIndex = NULL; //(int*)0xC6DBD4;

int* SelectedSaveEntryIndex = NULL; //(int*)0xC6D628;
int* CurrentSaveMenuPage = NULL; //(int*)0xC6D62C;
int* DailyRecordsItemIndex = NULL; //(int*)0x6E2CCC;
int* DailyRecordsPageIndex = NULL; //(int*)0xC6D708;
int* DailyRecordsUnlockedPages = NULL; //(int*)0xC6D774;
int* DataCollectionMode = NULL; //(int*)0xC6D6F8;
int* DataCollectionItemIndex = NULL; //(int*)0x6E3140;

int* SysMesBoxMesNum = NULL; //(int*)0xC6D900;
int* SysMesBoxChoiceNum = NULL; //(int*)0xC6D904;
int* SysMesBoxChoiceIndex = NULL; //(int*)0xC6D908;
int* SysMesBoxMesNumAlt = NULL; //(int*)0xC6D9A0;
int* SysMesBoxChoiceNumAlt = NULL; //(int*)0xC6D9A4;
int* SysMesBoxChoiceIndexAlt = NULL; //(int*)0xC6D9A8;

int* CGLibraryPageIndex = NULL; //(int*)0x74CAB0;
int* CGLibraryItemIndex = NULL; //(int*)0x74CABC;

int* AlbumCGScale = NULL; //(int*)0x74CB34;
int* AlbumCGScaleMin = NULL; //(int*)0x74CB1C;
int* AlbumCGScaleMax = NULL; //(int*)0x74CB20;
int* AlbumCGX = NULL; //(int*)0x74CB2C;
int* AlbumCGY = NULL; //(int*)0x74CB30;

int* MusicModeCurrentIndex = NULL; //(int*)0xC6D4E4;
int* MusicModeStartIndex = NULL; //(int*)0xC6D4E8;

int* TipsMenuTabTipCount = NULL; //(int*)0x9524C8;
int* TipsMenuCurrentTab = NULL; //(int*)0x6E0AC8;
int* TipsMenuSelectedIndex = NULL; //(int*)0x9525C0;
int* TipsMenuStartIndex = NULL; //(int*)0x9525BC;
int* TipsMenuTipTextStartOffset = NULL; //(int*)0x9525C8;
int* TipMenuTipTextHeight = NULL; //(int*)0x9525CC;

int* TwipoTweepCountInTabs = NULL; //(int*)0x2706450;
int* TwipoDataPointers = NULL; //(int*)0x2706440;
int* TwipoData1 = NULL; //(int*)0x26EBE34;
int* TwipoData2 = NULL; //(int*)0x26FD7A0;

int* ARNumberOfGeoTags = NULL; //(int*)0x996B0C;
float* ARGeoTagsXYZCoords = NULL; //(float*)0x998BE0;
int* ARSelectedGeoTag = NULL; //(int*)0x74CC94;
int* ARSomeGeoTagArr = NULL; //(int*)0x999220;
int* ARSomeGeoTagArr2 = NULL; //(int*)0x996B10;
int* ARSomeGeoTagArr3 = NULL; //(int*)0x9961A8;
int* ARSomeGeoTagData = NULL; //(int*)0x995CF4;
GetScreenCoordsProc gameExeGetScreenCoords;
CountSC3CharactersProc gameExeCountSC3Characters;
GetSC3StringByIDProc gameExeGetSC3StringByID;

BacklogRecalcMovementProc gameExeBacklogRecalcMovement;
int* BacklogDispPosMax = NULL; //(int*)0x947358;
int* BacklogSelectedIndex = NULL; //(int*)0x947340;
int* BacklogFirstFullDispLine = NULL; //(int*)0x947350;
int* BacklogLastFullDispLine = NULL; //(int*)0x947354;

uint32_t* InputMask = NULL; //(uint32_t*)0xC71040;
uint32_t* InputMask3 = NULL; //(uint32_t*)0xC71044;
uint32_t* InputMask2 = NULL; //(uint32_t*)0xC71048;
uint32_t* InputMask4 = NULL; //(uint32_t*)0xC7104C;
LPDIRECTINPUTDEVICEA* MouseDevice = NULL; //(LPDIRECTINPUTDEVICEA*)0x2A3F684;

namespace lb {
  int __fastcall mgsInputExecuteServerHook(MgsInputObj_t* pThis);
  int __cdecl padMainHook();
  int __cdecl movieModeDispHook(void* thread);
  int __cdecl instTitleMenuHook(void* thread);
  int __cdecl instSaveMenuHook(uint8_t* thread);
  int __cdecl instSystemMesHook(uint8_t* thread);
  int __cdecl instSystemMenuHook(void* thread);
  int __cdecl instTipsHook(void* thread);
  int __cdecl albumMenuMainHook();
  int __cdecl cgViewModeMainHook();
  void __cdecl pokecomMainMenuMainHook();
  int __cdecl musicMenuMainHook();
  int __cdecl backlogMenuMainHook();
  void __cdecl twipoMainHook();
  int __cdecl pokecomDocMainHook();
  int __cdecl pokecomMapMainHook();
  int __cdecl pokecomARMainHook();

  bool containsPoint(int x, int y, int bx, int by, int w, int h) {
    return x > bx && x < bx + w && y < by && y > by - h;
  }

  bool menuButtonHitTest(int id, int x, int y, int bx, int by, int w, int h, int* selIndexVar) {
    if (containsPoint(x, y, bx, by, w, h)) {
      PointedThisFrame = true;
      *selIndexVar = id;
      return true;
    }
    return false;
  }

  bool mouseSelectHitTest(int x, int y, int bx, int by, int w, int h) {
    if (containsPoint(x, y, bx, by, w, h)) {
      PointedThisFrame = true;
      return true;
    }
    return false;
  }

  bool mouseHitTest(int x, int y, int bx, int by, int w, int h) {
    if (containsPoint(x, y, bx, by, w, h)) {
      return true;
    }
    return false;
  }

  bool customInputRNEInit() {
    if (config["patch"].count("RNEMouseInput") != 1 ||
        config["patch"]["RNEMouseInput"].get<bool>() == false) {
        return true;
    }

    ShowCursor(1);

    InputObject = (MgsInputObj_t*)sigScan("game", "useOfInputObject");
    MouseEnabled = (uint8_t*)sigScan("game", "useOfMouseEnabled");
    WindowHandle = (HWND*)sigScan("game", "useOfWindowHandle");
    MousePointing = (uint8_t*)sigScan("game", "useOfMousePointing");

    TitleMenuSelectionIndex = (int*)sigScan("game", "useOfTitleMenuSelectionIndex");
    SubMenuSelIndex = (int*)sigScan("game", "useOfSubMenuSelIndex");
    LoadMenuSelIndex = (int*)sigScan("game", "useOfLoadMenuSelIndex");
    ExtrasMenuSelIndex = (int*)sigScan("game", "useOfExtrasMenuSelIndex");
    TitleSubMenuAnimCounter = (int*)sigScan("game", "useOfTitleSubMenuAnimCounter");

    MovieModeSelectionIndex = (int*)sigScan("game", "useOfMovieModeSelectionIndex");

    SelectedSaveEntryIndex = (int*)sigScan("game", "useOfSelectedSaveEntryIndex");
    CurrentSaveMenuPage = (int*)sigScan("game", "useOfCurrentSaveMenuPage");
    DailyRecordsItemIndex = (int*)sigScan("game", "useOfDailyRecordsItemIndex");
    DailyRecordsPageIndex = (int*)sigScan("game", "useOfDailyRecordsPageIndex");
    DailyRecordsUnlockedPages = (int*)sigScan("game", "useOfDailyRecordsUnlockedPages");
    DataCollectionMode = (int*)sigScan("game", "useOfDataCollectionMode");
    DataCollectionItemIndex = (int*)sigScan("game", "useOfDataCollectionItemIndex");

    SysMesBoxMesNum = (int*)sigScan("game", "useOfSysMesBoxMesNum");
    SysMesBoxChoiceNum = (int*)sigScan("game", "useOfSysMesBoxChoiceNum");
    SysMesBoxChoiceIndex = (int*)sigScan("game", "useOfSysMesBoxChoiceIndex");
    SysMesBoxMesNumAlt = (int*)sigScan("game", "useOfSysMesBoxMesNumAlt");
    SysMesBoxChoiceNumAlt = (int*)sigScan("game", "useOfSysMesBoxChoiceNumAlt");
    SysMesBoxChoiceIndexAlt = (int*)sigScan("game", "useOfSysMesBoxChoiceIndexAlt");

    CGLibraryPageIndex = (int*)sigScan("game", "useOfCGLibraryPageIndex");
    CGLibraryItemIndex = (int*)sigScan("game", "useOfCGLibraryItemIndex");

    AlbumCGScale = (int*)sigScan("game", "useOfAlbumCGScale");
    AlbumCGScaleMin = (int*)sigScan("game", "useOfAlbumCGScaleMin");
    AlbumCGScaleMax = (int*)sigScan("game", "useOfAlbumCGScaleMax");
    AlbumCGX = (int*)sigScan("game", "useOfAlbumCGX");
    AlbumCGY = (int*)sigScan("game", "useOfAlbumCGY");

    MusicModeCurrentIndex = (int*)sigScan("game", "useOfMusicModeCurrentIndex");
    MusicModeStartIndex = (int*)sigScan("game", "useOfMusicModeStartIndex");

    TipsMenuTabTipCount = (int*)sigScan("game", "useOfTipsMenuTabTipCount");
    TipsMenuCurrentTab = (int*)sigScan("game", "useOfTipsMenuCurrentTab");
    TipsMenuSelectedIndex = (int*)sigScan("game", "useOfTipsMenuSelectedIndex");
    TipsMenuStartIndex = (int*)sigScan("game", "useOfTipsMenuStartIndex");
    TipsMenuTipTextStartOffset = (int*)sigScan("game", "useOfTipsMenuTipTextStartOffset");
    TipMenuTipTextHeight = (int*)sigScan("game", "useOfTipMenuTipTextHeight");

    TwipoTweepCountInTabs = (int*)sigScan("game", "useOfTwipoTweepCountInTabs");
    TwipoDataPointers = (int*)sigScan("game", "useOfTwipoDataPointers");
    TwipoData1 = (int*)sigScan("game", "useOfTwipoData1");
    TwipoData2 = (int*)sigScan("game", "useOfTwipoData2");

    ARNumberOfGeoTags = (int*)sigScan("game", "useOfARNumberOfGeoTags");
    ARGeoTagsXYZCoords = (float*)sigScan("game", "useOfARGeoTagsXYZCoords");
    ARSelectedGeoTag = (int*)sigScan("game", "useOfARSelectedGeoTag");
    ARSomeGeoTagArr = (int*)sigScan("game", "useOfARSomeGeoTagArr");
    ARSomeGeoTagArr2 = (int*)sigScan("game", "useOfARSomeGeoTagArr2");
    ARSomeGeoTagArr3 = (int*)sigScan("game", "useOfARSomeGeoTagArr3");
    ARSomeGeoTagData = (int*)sigScan("game", "useOfARSomeGeoTagData");

    BacklogDispPosMax = (int*)sigScan("game", "useOfBacklogDispPosMax");
    BacklogSelectedIndex = (int*)sigScan("game", "useOfBacklogSelectedIndex");
    BacklogFirstFullDispLine = (int*)sigScan("game", "useOfBacklogFirstFullDispLine");
    BacklogLastFullDispLine = (int*)sigScan("game", "useOfBacklogLastFullDispLine");

    InputMask = (uint32_t*)sigScan("game", "useOfInputMask");
    InputMask2 = (uint32_t*)sigScan("game", "useOfInputMask2");
    InputMask3 = (uint32_t*)sigScan("game", "useOfInputMask3");
    InputMask4 = (uint32_t*)sigScan("game", "useOfInputMask4");
    MouseDevice = (LPDIRECTINPUTDEVICEA*)sigScan("game", "useOfMouseDevice");
   
    if (!scanCreateEnableHook("game", "mgsInputExecuteServer", (uintptr_t*)&gameExeMgsInputExecuteServer,
                              (LPVOID)&mgsInputExecuteServerHook,
                              (LPVOID*)&gameExeMgsInputExecuteServerReal) ||
        !scanCreateEnableHook("game", "PADmain", (uintptr_t*)&gameExePADmain,
                              (LPVOID)&padMainHook,
                              (LPVOID*)&gameExePADmainReal))
        return false;

    scanCreateEnableHook("game", "InstTitleMenu", (uintptr_t*)&gameExeInstTitleMenu,
                         (LPVOID)&instTitleMenuHook,
                         (LPVOID*)&gameExeInstTitleMenuReal);
    scanCreateEnableHook("game", "MovieModeDisp", (uintptr_t*)&gameExeMovieModeDisp,
                         (LPVOID)&movieModeDispHook,
                         (LPVOID*)&gameExeMovieModeDispReal);
    scanCreateEnableHook("game", "InstSaveMenu", (uintptr_t*)&gameExeInstSaveMenu,
                         (LPVOID)&instSaveMenuHook,
                         (LPVOID*)&gameExeInstSaveMenuReal);
    scanCreateEnableHook("game", "InstSystemMes", (uintptr_t*)&gameExeInstSystemMes,
                         (LPVOID)&instSystemMesHook,
                         (LPVOID*)&gameExeInstSystemMesReal);
    scanCreateEnableHook("game", "InstSystemMenu", (uintptr_t*)&gameExeInstSystemMenu,
                         (LPVOID)&instSystemMenuHook,
                         (LPVOID*)&gameExeInstSystemMenuReal);
    scanCreateEnableHook("game", "InstTips", (uintptr_t*)&gameExeInstTips,
                         (LPVOID)&instTipsHook,
                         (LPVOID*)&gameExeInstTipsReal);
    scanCreateEnableHook("game", "AlbumMenuMain", (uintptr_t*)&gameExeAlbumMenuMain,
                         (LPVOID)&albumMenuMainHook,
                         (LPVOID*)&gameExeAlbumMenuMainReal);
    scanCreateEnableHook("game", "CGViewModeMain", (uintptr_t*)&gameExeCGViewModeMain,
                         (LPVOID)&cgViewModeMainHook,
                         (LPVOID*)&gameExeCGViewModeMainReal);
    scanCreateEnableHook("game", "PokecomMainMenuMain", (uintptr_t*)&gameExePokecomMainMenuMain,
                         (LPVOID)&pokecomMainMenuMainHook,
                         (LPVOID*)&gameExePokecomMainMenuMainReal);
    scanCreateEnableHook("game", "MusicMenuMain", (uintptr_t*)&gameExeMusicMenuMain,
                         (LPVOID)&musicMenuMainHook,
                         (LPVOID*)&gameExeMusicMenuMainReal);
    scanCreateEnableHook("game", "BacklogMenuMain", (uintptr_t*)&gameExeBacklogMenuMain,
                         (LPVOID)&backlogMenuMainHook,
                         (LPVOID*)&gameExeBacklogMenuMainReal);
    gameExeBacklogRecalcMovement = (BacklogRecalcMovementProc)sigScan("game", "BacklogRecalcMovement");
    scanCreateEnableHook("game", "TwipoMain", (uintptr_t*)&gameExeTwipoMain,
                         (LPVOID)&twipoMainHook,
                         (LPVOID*)&gameExeTwipoMainReal);
    scanCreateEnableHook("game", "PokecomDocMain", (uintptr_t*)&gameExePokecomDocMain,
                         (LPVOID)&pokecomDocMainHook,
                         (LPVOID*)&gameExePokecomDocMainReal);
    scanCreateEnableHook("game", "PokecomMapMain", (uintptr_t*)&gameExePokecomMapMain,
                         (LPVOID)&pokecomMapMainHook,
                         (LPVOID*)&gameExePokecomMapMainReal);
    scanCreateEnableHook("game", "PokecomARMain", (uintptr_t*)&gameExePokecomARMain,
                         (LPVOID)&pokecomARMainHook,
                         (LPVOID*)&gameExePokecomARMainReal);
    gameExeGetScreenCoords = (GetScreenCoordsProc)sigScan("game", "GetScreenCoords");
    gameExeCountSC3Characters = (CountSC3CharactersProc)sigScan("game", "CountSC3Characters");
    gameExeGetSC3StringByID = (GetSC3StringByIDProc)sigScan("game", "GetSC3StringByID");

    return true;
  }

  int __cdecl pokecomARMainHook() {
    // Flag 2903 - SF_Pokecon_ManualMode
    if (*MouseEnabled && gameExeGetFlag(2903)) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      bool dontMoveThisFrame = false;

      if (*ARNumberOfGeoTags) {
        int xyzCnt = 0;
        for (int i = 0; i < *ARNumberOfGeoTags; i++) {
          float x, y;
          // Oh shit, here we go again
          gameExeGetScreenCoords(0, &x, &y, ARGeoTagsXYZCoords[xyzCnt], 12.5f + ARGeoTagsXYZCoords[xyzCnt + 1], ARGeoTagsXYZCoords[xyzCnt + 2]);

          // Your guess is as good as mine
          int screenX = (-42 + x) * 2;
          int screenY = (-69 + y) * 2;

          int textWidth;
          if (ARSomeGeoTagArr2[ARSomeGeoTagArr[i]]) {
            textWidth = gameExeCountSC3Characters(gameExeGetSC3StringByID(13, *ARSomeGeoTagData + 200), 100, 0);
          } else {
            textWidth = getSc3StringDisplayWidthHook((char*)gameExeGetSC3StringByID(13, ARSomeGeoTagArr3[ARSomeGeoTagArr[i]]), 0, 22);
          }
          textWidth *= 2;

          if (menuButtonHitTest(i, mouseX, mouseY, screenX, screenY + 150, textWidth + 80, 150, ARSelectedGeoTag)) {
            if (InputObject->mouseButtons & 0x1) {
              dontMoveThisFrame = true;
              *InputMask |= 0x1000;
            }
            gameExeScrWork[6432] = *ARSelectedGeoTag;
            gameExeScrWork[6433] = ARSomeGeoTagArr[*ARSelectedGeoTag];
          }

          xyzCnt += 4;
        }
      }

      int axisMultiplier = gameExeScrWork[6428] / 500;
      if ((InputObject->mouseButtonsHeld & 0x1) && !dontMoveThisFrame) {
        gameExeScrWork[6404] += InputObject->mouseYAxis * axisMultiplier;
        gameExeScrWork[6405] -= InputObject->mouseXAxis * axisMultiplier;

        if (!MouseExclusive) {
          MouseExclusive = true;
          (*MouseDevice)->Unacquire();
          (*MouseDevice)->SetCooperativeLevel(*WindowHandle, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
          (*MouseDevice)->Acquire();
        }

        if (gameExeGetFlag(2817)) {
          if (gameExeScrWork[6404] < -80000)
            gameExeScrWork[6404] = -80000;
          if (gameExeScrWork[6404] >= 80000)
            gameExeScrWork[6404] = 80000;
          if (gameExeScrWork[6405] <= -180000)
            gameExeScrWork[6405] += 360000;
          if (gameExeScrWork[6405] > 180000)
            gameExeScrWork[6405] -= 360000;
        } else {
          if (gameExeScrWork[6404] < gameExeScrWork[6410])
            gameExeScrWork[6404] = gameExeScrWork[6410];
          if (gameExeScrWork[6404] > gameExeScrWork[6411])
            gameExeScrWork[6404] = gameExeScrWork[6411];
          if (gameExeScrWork[6405] < gameExeScrWork[6412])
            gameExeScrWork[6405] = gameExeScrWork[6412];
          if (gameExeScrWork[6405] > gameExeScrWork[6413])
            gameExeScrWork[6405] = gameExeScrWork[6413];
        }
      } else {
        if (MouseExclusive) {
          MouseExclusive = false;
          (*MouseDevice)->Unacquire();
          (*MouseDevice)->SetCooperativeLevel(*WindowHandle, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
          (*MouseDevice)->Acquire();
        }
      }

      if (InputObject->mouseButtonsHeld & 0x8) {
        gameExeScrWork[6428] = gameExeScrWork[6428] - 1000 > 10000 ? gameExeScrWork[6428] - 1000 : 10000;
      } else if (InputObject->mouseButtonsHeld & 0x10) {
        gameExeScrWork[6428] = gameExeScrWork[6428] + 1000 < gameExeScrWork[6427] - 500 ? gameExeScrWork[6428] + 1000 : gameExeScrWork[6427] - 500;
      }

      if (InputObject->mouseButtons & 0x2) {
        CarryInputToTheNextFrame |= 0x2000;
      }
    }

    return gameExePokecomARMainReal();
  }

  int __cdecl pokecomMapMainHook() {
    // Flag 2903 - SF_Pokecon_ManualMode
    if (*MouseEnabled && gameExeGetFlag(2903)) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      // 6510 - MapPosX
      // 6511 - MapPosY
      if (InputObject->mouseButtonsHeld & 0x1) {
        gameExeScrWork[6510] -= InputObject->mouseXAxis;
        gameExeScrWork[6511] -= InputObject->mouseYAxis;
      }

      // 6517 - MapSize
      if (InputObject->mouseButtonsHeld & 0x8) {
        gameExeScrWork[6517] = gameExeScrWork[6517] - 50 > 500 ? gameExeScrWork[6517] - 50 : gameExeScrWork[6517];
      } else if (InputObject->mouseButtonsHeld & 0x10) {
        gameExeScrWork[6517] = gameExeScrWork[6517] + 50 < 2000 ? gameExeScrWork[6517] + 50 : gameExeScrWork[6517];
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExePokecomMapMainReal();
  }

  int __cdecl pokecomDocMainHook() {
    // Flag 2903 - SF_Pokecon_ManualMode
    if (*MouseEnabled && gameExeGetFlag(2903)) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      // Doc mode - selection
      if (gameExeScrWork[6471] == 0) {
        // 6481 - Number of docs unlocked
        // 6479 - Currently selected doc
        for (int i = 0; i < gameExeScrWork[6481]; i++) {
          if (menuButtonHitTest(i, mouseX, mouseY, 72 + (i * 232), 346, 232, 207, &gameExeScrWork[6479]) && (InputObject->mouseButtons & 0x1))
            *InputMask |= 0x1000;
        }
      } else if (gameExeScrWork[6471] == 1) { // Doc mode - reading
        // 6476 - Doc current pos
        // 6473 - Doc max pos
        if (InputObject->mouseButtonsHeld & 0x8) {
          gameExeScrWork[6476] = gameExeScrWork[6476] <= 8 ? 8 : gameExeScrWork[6476] - 24;
        } else if (InputObject->mouseButtonsHeld & 0x10) {
          gameExeScrWork[6476] = gameExeScrWork[6476] + 24 >= 32 * gameExeScrWork[6473] ? 32 * gameExeScrWork[6473] : gameExeScrWork[6476] + 24;
        }
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }
    return gameExePokecomDocMainReal();
  }

  void __cdecl twipoMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      // Read mode
      if (gameExeScrWork[6460] == 0) {
        // Twipo tabs
        for (int i = 0; i < 4; i++) {
          if (mouseSelectHitTest(mouseX, mouseY, 75, 275 + (i * 273), 90, 273) && (InputObject->mouseButtons & 0x1))
            gameExeScrWork[6461] = i;
        }

        // Reply button
        int data = *(int*)(TwipoDataPointers[gameExeScrWork[6461]] + 8 * gameExeScrWork[6459]);
        if ((TwipoData1[5 * data] != 0xFFFF) && TwipoData2[data]) {
          if (mouseSelectHitTest(mouseX, mouseY, 1533, 1052, 357, 118) && (InputObject->mouseButtons & 0x1)) {
            *InputMask |= 0x1000;
          }
        }

        // Scroll tweeps up/down by clicking on them
        if ((TwipoTweepCountInTabs[gameExeScrWork[6461]] - gameExeScrWork[6459]) > 1) {
          if (mouseSelectHitTest(mouseX, mouseY, 166, 365, 677, 366) && (InputObject->mouseButtons & 0x1)) {
            *InputMask4 |= 0x10000;
          }
        }
        if (gameExeScrWork[6459] > 0) {
          if (mouseSelectHitTest(mouseX, mouseY, 166, 1079, 677, 355) && (InputObject->mouseButtons & 0x1)) {
            *InputMask4 |= 0x20000;
          }
        }

        // Scroll tweeps up/down with scroll wheel
        if (mouseHitTest(mouseX, mouseY, 166, 1079, 675, 1080)) {
          if (InputObject->mouseButtonsHeld & 0x8) {
            *InputMask4 |= 0x10000;
          }
          else if (InputObject->mouseButtonsHeld & 0x10) {
            *InputMask4 |= 0x20000;
          }
        }
      } else if (gameExeScrWork[6460] == 1) {  // Reply mode
        // Reply button
        if(mouseSelectHitTest(mouseX, mouseY, 1533, 1052, 357, 118) && (InputObject->mouseButtons & 0x1)) {
          *InputMask |= 0x1000;
        }

        // Scroll choices by clicking on up/down arrows
        if (mouseSelectHitTest(mouseX, mouseY, 1332, 497, 94, 70) && (InputObject->mouseButtons & 0x1)) {
          *InputMask |= 0x10000;
        }
        if (mouseSelectHitTest(mouseX, mouseY, 1332, 950, 94, 70) && (InputObject->mouseButtons & 0x1)) {
          *InputMask |= 0x20000;
        }

        // Scroll choices with scroll wheel
        if (mouseHitTest(mouseX, mouseY, 883, 915, 986, 452)) {
          if (InputObject->mouseButtonsHeld & 0x8) {
            *InputMask |= 0x10000;
          }
          else if (InputObject->mouseButtonsHeld & 0x10) {
            *InputMask |= 0x20000;
          }
        }
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeTwipoMainReal();
  }

  int __cdecl backlogMenuMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      int startLineIndex = *BacklogFirstFullDispLine > 0 ? *BacklogFirstFullDispLine - 1 : 0;
      for (int i = startLineIndex; i < *BacklogLastFullDispLine + 2; i++) {
        int lineY = (BacklogDispLinePosY[i] - *BacklogDispPos + 54 + BacklogDispLineSize[i] + 8) * 2;
        if (menuButtonHitTest(i, mouseX, mouseY, 0, lineY, 1725, (BacklogDispLineSize[i] + 8) * 2, BacklogSelectedIndex) && (InputObject->mouseButtons & 0x1))
          *InputMask |= 0x1000;
      }

      if (InputObject->mouseButtonsHeld & 0x8) {
        *BacklogDispPos = *BacklogDispPos <= 0 ? 0 : *BacklogDispPos - 48;
        gameExeBacklogRecalcMovement();
      } else if (InputObject->mouseButtonsHeld & 0x10) {
        *BacklogDispPos = *BacklogDispPos + 48 >= *BacklogDispPosMax - 388 ? *BacklogDispPosMax - 380 : *BacklogDispPos + 48;
        gameExeBacklogRecalcMovement();
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeBacklogMenuMainReal();
  }

  int __cdecl musicMenuMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      for (int i = 0; i < 15; i++) {
        if (menuButtonHitTest(i, mouseX, mouseY, 0, 346 + (i * 44), 1870, 43, MusicModeCurrentIndex) && (InputObject->mouseButtons & 0x1))
          *InputMask |= 0x1000;
      }

      if (mouseSelectHitTest(mouseX, mouseY, 1532, 176, 258, 36) && (InputObject->mouseButtons & 0x1))
          *InputMask |= 0x8000;

      if (InputObject->mouseButtonsHeld & 0x8) {
        *MusicModeStartIndex = *MusicModeStartIndex == 0 ? 0 : *MusicModeStartIndex - 1;
      } else if (InputObject->mouseButtonsHeld & 0x10) {
        *MusicModeStartIndex = *MusicModeStartIndex == 31 ? 31 : *MusicModeStartIndex + 1;
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeMusicMenuMainReal();
  }

  void __cdecl pokecomMainMenuMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;
      
      if (menuButtonHitTest(0, mouseX, mouseY, 1, 269, 478, 268, &gameExeScrWork[6383]) ||
          menuButtonHitTest(1, mouseX, mouseY, 481, 269, 478, 268, &gameExeScrWork[6383]) ||
          menuButtonHitTest(2, mouseX, mouseY, 1441, 269, 478, 268, &gameExeScrWork[6383]) ||
          menuButtonHitTest(3, mouseX, mouseY, 1441, 541, 478, 268, &gameExeScrWork[6383]) ||
          menuButtonHitTest(4, mouseX, mouseY, 481, 812, 478, 268, &gameExeScrWork[6383]) ||
          menuButtonHitTest(5, mouseX, mouseY, 962, 812, 478, 268, &gameExeScrWork[6383]) ||
          menuButtonHitTest(6, mouseX, mouseY, 1, 1080, 478, 268, &gameExeScrWork[6383]) ||
          menuButtonHitTest(7, mouseX, mouseY, 962, 1080, 478, 268, &gameExeScrWork[6383]) ||
          menuButtonHitTest(8, mouseX, mouseY, 1441, 1080, 478, 268, &gameExeScrWork[6383])) {
        if (InputObject->mouseButtons & 0x1) {
          *InputMask |= 0x1000;
        }
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    gameExePokecomMainMenuMainReal();
  }

  int __cdecl cgViewModeMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      if (InputObject->mouseButtonsHeld & 0x1) {
        *AlbumCGX -= InputObject->mouseXAxis * 2000;
        *AlbumCGY -= InputObject->mouseYAxis * 2000;
      }

      if (InputObject->mouseButtons1 & 0x1)
        *InputMask |= 0x1000;
      
      if (InputObject->mouseButtonsHeld & 0x8) {
        *AlbumCGScale = *AlbumCGScale >= *AlbumCGScaleMax ? *AlbumCGScaleMax : *AlbumCGScale + 100;
      } else if (InputObject->mouseButtonsHeld & 0x10) {
        *AlbumCGScale = *AlbumCGScale <= *AlbumCGScaleMin ? *AlbumCGScaleMin : *AlbumCGScale - 100;
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeCGViewModeMainReal();
  }

  int __cdecl albumMenuMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;
      
      int id = 0;
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
          if (menuButtonHitTest(id++, mouseX, mouseY, 88 + (j * 446), 396 + (i * 276), 410, 240, CGLibraryItemIndex) && (InputObject->mouseButtons & 0x1))
            *InputMask2 |= 0x1000;
        }
      }
      
      if (InputObject->mouseButtonsHeld & 0x8) {
        *CGLibraryPageIndex = *CGLibraryPageIndex == 0 ? 12 : *CGLibraryPageIndex - 1;
      } else if (InputObject->mouseButtonsHeld & 0x10) {
        *CGLibraryPageIndex = *CGLibraryPageIndex == 12 ? 0 : *CGLibraryPageIndex + 1;
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeAlbumMenuMainReal();
  }

  int __cdecl instTipsHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      int count = TipsMenuTabTipCount[*TipsMenuCurrentTab] > 19 ? 19 : TipsMenuTabTipCount[*TipsMenuCurrentTab];
      for (int i = 0; i < count; i++) {
        if (menuButtonHitTest(i, mouseX, mouseY, 97, 215 + (i * 44), 534, 43, TipsMenuSelectedIndex) && (InputObject->mouseButtons & 0x1))
          *InputMask |= 0x1000;
      }

      for (int i = 0; i < 4; i++) {
        if (TipsMenuTabTipCount[i]) {
          if (mouseSelectHitTest(mouseX, mouseY, 94 + (i * 144), 116, 140, 35) && (InputObject->mouseButtons & 0x1))
            *TipsMenuCurrentTab = i;
        }
      }

      int maxStartIndex = TipsMenuTabTipCount[*TipsMenuCurrentTab] - 19;
      if ((TipsMenuTabTipCount[*TipsMenuCurrentTab] > 19) && mouseHitTest(mouseX, mouseY, 97, 1013, 560, 890)) {
        if (InputObject->mouseButtonsHeld & 0x8) {
          *TipsMenuStartIndex = *TipsMenuStartIndex == 0 ? 0 : *TipsMenuStartIndex - 1;
        } else if (InputObject->mouseButtonsHeld & 0x10) {
          *TipsMenuStartIndex = *TipsMenuStartIndex == maxStartIndex ? maxStartIndex : *TipsMenuStartIndex + 1;
        }
      }

      if ((*TipMenuTipTextHeight > 370) && mouseHitTest(mouseX, mouseY, 664, 1013, 1100, 788)) {
        if (InputObject->mouseButtonsHeld & 0x8) {
          *TipsMenuTipTextStartOffset = *TipsMenuTipTextStartOffset == 0 ? 0 : *TipsMenuTipTextStartOffset - 20;
        } else if (InputObject->mouseButtonsHeld & 0x10) {
          *TipsMenuTipTextStartOffset = (*TipsMenuTipTextStartOffset - 370) > (*TipMenuTipTextHeight - 740) ? *TipsMenuTipTextStartOffset : *TipsMenuTipTextStartOffset + 20;
        }
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeInstTipsReal(thread);
  }

  int __cdecl instSystemMenuHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;
      
      for (int i = 0; i < 11; i++) {
        if (menuButtonHitTest(i, mouseX, mouseY, 0, 289 + (i * 58), 626, 40, &gameExeScrWork[3338]) && (InputObject->mouseButtons & 0x1))
          *InputMask |= 0x1000;
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }
    return gameExeInstSystemMenuReal(thread);
  }

  int __cdecl instSystemMesHook(uint8_t* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;
      
      int* selIndexVar = SysMesBoxChoiceIndex;
      int* selNumVar = SysMesBoxChoiceNum;
      int* mesNumVar = SysMesBoxMesNum;
      
      // (Revo) May whoever reads this forgive me for these sins
      uint8_t type = *((uint8_t*)(*(uint32_t*)(thread + 332)) + 2);
      uint8_t val = type - 32;
      if (type < 0x80)
        val = type;
      // Here we determine whether to use the second instance of the sysmesbox
      // It's used for the "Exit Game" message that can overlap anything
      if (val >= 0xA) {
        selIndexVar = SysMesBoxChoiceIndexAlt;
        selNumVar = SysMesBoxChoiceNumAlt;
        mesNumVar = SysMesBoxMesNumAlt;
      }
      
      // Calculating the Y position of the buttons
      // based on number of lines in the message...
      float val1 = 28 * (*mesNumVar + 4);
      float val2 = 360 - val1 / 2;
      int buttonY = (val1 + val2 - 3) * 1.5;
      
      switch (*selNumVar) {
        case 1: {
          if (menuButtonHitTest(1, mouseX, mouseY, 1225, buttonY, 158, 39, selIndexVar) && (InputObject->mouseButtons & 0x1))
            *InputMask |= 0x1000;
        } break;
        case 2: {
          if (menuButtonHitTest(0, mouseX, mouseY, 1057, buttonY, 158, 39, selIndexVar) ||
              menuButtonHitTest(1, mouseX, mouseY, 1225, buttonY, 158, 39, selIndexVar))
            if (InputObject->mouseButtons & 0x1) {
              *InputMask |= 0x1000;
            }
        } break;
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeInstSystemMesReal(thread);
  }

  int __cdecl instSaveMenuHook(uint8_t* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;
      
      uint8_t type = *((uint8_t*)(*(uint32_t*)(thread + 332)) + 2);
      
      if (type == 11) { // Daily Records/Data Collection
        int* selIndexVar = DailyRecordsItemIndex;
        if (*DataCollectionMode) {
          selIndexVar = DataCollectionItemIndex;
        }
        int id = 0;
        for (int i = 0; i < 6; i++) {
          for (int j = 0; j < 7; j++) {
            if (menuButtonHitTest(id++, mouseX, mouseY, 330 + (j * 211), 322 + (i * 130), 206, 127, selIndexVar) && (InputObject->mouseButtons & 0x1))
              *InputMask |= 0x1000;
          }
        }
      
        if (!*DataCollectionMode) {
          // Page switch Left/Right buttons
          if ((*DailyRecordsPageIndex != 0) && mouseSelectHitTest(mouseX, mouseY, 261, 564, 60, 89) && (InputObject->mouseButtons & 0x1)) {
            *InputMask2 |= 0x100;
          }
          if ((*DailyRecordsPageIndex < (*DailyRecordsUnlockedPages - 1)) && mouseSelectHitTest(mouseX, mouseY, 1804, 564, 60, 89) && (InputObject->mouseButtons & 0x1)) {
            *InputMask2 |= 0x200;
          }

          // Page switch with scroll wheel
          if (InputObject->mouseButtonsHeld & 0x8) {
            *InputMask2 |= 0x100;
          }
          else if (InputObject->mouseButtonsHeld & 0x10) {
            *InputMask2 |= 0x200;
          }
        }
      } else { // Save/Load
        int id = 0;
        for (int i = 0; i < 2; i++) {
          for (int j = 0; j < 4; j++) {
            if (menuButtonHitTest(id++, mouseX, mouseY, 98 + (i * 890), 327 + (j * 220), 840, 188, SelectedSaveEntryIndex) && (InputObject->mouseButtons & 0x1))
              *InputMask |= 0x1000;
          }
        }
      
        // Page switch with scroll wheel
        if (InputObject->mouseButtonsHeld & 0x8) {
          *CurrentSaveMenuPage = *CurrentSaveMenuPage == 0 ? 5 : *CurrentSaveMenuPage - 1;
        } else if (InputObject->mouseButtonsHeld & 0x10) {
          *CurrentSaveMenuPage = *CurrentSaveMenuPage == 5 ? 0 : *CurrentSaveMenuPage + 1;
        }
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeInstSaveMenuReal(thread);
  }

  int __cdecl movieModeDispHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;
      
      int id = 0;
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
          if (menuButtonHitTest(id++, mouseX, mouseY, 88 + (j * 446), 396 + (i * 276), 410, 240, MovieModeSelectionIndex) && (InputObject->mouseButtons & 0x1))
            *InputMask2 |= 0x1000;
        }
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeMovieModeDispReal(thread);
  }

  int __cdecl instTitleMenuHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;
      int* selIndexVar = TitleMenuSelectionIndex;
      // Number of main menu items
      // Flag 821 means Phase 09 menu item has been unlocked
      int numItems = 6 + gameExeGetFlag(821);
      if (*SubMenuSelIndex) {
        switch (*SubMenuSelIndex) {
          case 1: {
            selIndexVar = LoadMenuSelIndex;
            numItems = 2;
          } break;
          case 2: {
            selIndexVar = ExtrasMenuSelIndex;
            // Get number of Extras menu items based on unlocked menus
            // Flags: 860 - Album, 863 - Movie, 864 - Sound
            numItems = 2 + (gameExeGetFlag(860) + gameExeGetFlag(863) + gameExeGetFlag(864));
          } break;
        }
      }
      
      if (!*TitleSubMenuAnimCounter) {
        for (int i = 0; i < numItems; i++) {
          if (menuButtonHitTest(i, mouseX, mouseY, 0, 266 + (i * 76), 835, 52, selIndexVar) && (InputObject->mouseButtons & 0x1))
            *InputMask |= 0x1000;
        }
      }

      if (InputObject->mouseButtons & 0x2) {
        *InputMask |= 0x2000;
      }
    }

    return gameExeInstTitleMenuReal(thread);
  }

  int __fastcall mgsInputExecuteServerHook(MgsInputObj_t* pThis) {
    int ret = gameExeMgsInputExecuteServerReal(pThis);

    if (MouseExclusive) {

    }
    else {
      (*MouseDevice)->Unacquire();
      (*MouseDevice)->SetCooperativeLevel(*WindowHandle, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
      (*MouseDevice)->Acquire();
    }

    if (!*MouseEnabled && pThis->mouseFocused && (InputObject->mouseButtons & 0x1)) {
      InputObject->mouseButtons = 0;
      *MouseEnabled = 1;
    }

    if (GetActiveWindow() != *WindowHandle) 
      *MouseEnabled = 0;

    *MousePointing = PointedThisFrame;
    PointedThisFrame = false;

    return ret;
  }

  int __cdecl padMainHook() {
    int ret = gameExePADmainReal();

    if (*MouseEnabled && !LockMouseControls) {
      if (InputObject->mouseButtons & 0x1) {
        *InputMask |= 0x1000;
      }
      
      if (InputObject->mouseButtons & 0x2) {
        if (gameExeScrWork[2113] & 1)
          *InputMask |= 0x10;
        else
          *InputMask |= 0x2000;
      }

      if (InputObject->mouseButtons & 0x4 && gameExeScrWork[2113] & 1) {
        *InputMask |= 0x800;
      }
      
      if (InputObject->mouseButtons & 0x8) {
        *InputMask |= 0x8000;
      } else if (InputObject->mouseButtons & 0x10) {
        *InputMask |= 0x1000;
      }
    }

    LockMouseControls = false;

    if (CarryInputToTheNextFrame) {
      *InputMask |= CarryInputToTheNextFrame;
      CarryInputToTheNextFrame = 0;
    }

    return ret;
  }
}  // namespace lb