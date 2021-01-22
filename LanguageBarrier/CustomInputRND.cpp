#include "CustomInputRND.h"
#include "Config.h"
#include "Game.h"
#include "GameText.h"
#include "LanguageBarrier.h"
#include "SigScan.h"

// A warning before this madness starts:
// There are a lot of magic coordinate calculations in this code
// simply because that's how it's done in the original game
// and frankly, I have no desire or energy to try and make it better.

namespace lb {
namespace rnd {
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
    int mouseX;
    int mouseY;
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

  enum MouseKeys {
    MouseLeftClick = 0x1,
    MouseRightClick = 0x2,
    MouseMiddleClick = 0x4,
    MouseScrollWheelUp = 0x8,
    MouseScrollWheelDown = 0x10
  };

  enum PadKeys {
    PAD1A = 0x1000,
    PAD1B = 0x2000,
    PAD1X = 0x4000,
    PAD1Y = 0x8000,
    PAD1L1 = 0x100,
    PAD1R1 = 0x200,
    PAD1R2 = 0x800,
    PAD1START = 0x10,
    PAD1RIGHTSTICKUP = 0x10000,
    PAD1RIGHTSTICKDOWN = 0x20000,
  };

  enum ScrWorkEnum {
    SW_GAMEMODE = 2113,
    SW_SYSMENUSELNO = 3338,
    SW_POKECOMSELNO = 6383,
    SW_AR_ELV = 6404,
    SW_AR_ROT = 6405,
    SW_AR_ELVMIN = 6410,
    SW_AR_ELVMAX = 6411,
    SW_AR_ROTMIN = 6412,
    SW_AR_ROTMAX = 6413,
    SW_AR_ANGLE_M = 6427,
    SW_AR_ANGLE_C = 6428,
    SW_GEOTAGSEL_INDEX = 6432,
    SW_GEOTAGSEL_ID = 6433,
    SW_TWIPOCURTW = 6459,
    SW_TWIPOMODE = 6460,
    SW_TWIPOCURTAB = 6461,
    SW_DOC_MODE = 6471,
    SW_DOC_CURMAX = 6473,
    SW_DOC_CURTARGET = 6476,
    SW_DOC_SELECTED = 6479,
    SW_DOC_UNLOCKEDNUM = 6481,
    SW_MAP_POSX = 6510,
    SW_MAP_POSY = 6511,
    SW_MAP_MODE = 6516,
    SW_MAP_SIZE = 6517,
  };

  enum FlagWorkEnum {
    OPEN_START2 = 810,
    OPEN_START3 = 811,
    ALBUM_ENA = 860,
    MOVIE_ENA = 863,
    MUSIC_ENA = 864,
    SF_MESALLSKIP = 1234,
    CALENDAR_DISP = 1615,
    AR_SUPERMODE = 2817,
    Pokecom_Open = 2900,
    Pokecom_ManualMode = 2903,
    Pokecom_Disable = 2904,
    KRep_SearchMode = 3030
  };

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
  typedef uint8_t* (__cdecl* GetSC3StringByIDProc)(int, int);

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

  typedef int(__cdecl* InstOptionProc)(void*);
  static InstOptionProc gameExeInstOption = NULL;
  static InstOptionProc gameExeInstOptionReal = NULL;
  typedef int(__thiscall* SetConfigOptionProc)(int*, int);

  typedef int(__cdecl* MesDispWindowProc)(int, int, int, int, int, int, int, int);
  static MesDispWindowProc gameExeMesDispWindow = NULL;
  static MesDispWindowProc gameExeMesDispWindowReal = NULL;

  typedef int(__cdecl* SetScreenResProc)();

  typedef int(__cdecl* GeoTagHasLinkProc)(int, int, int, int);

  typedef int(__cdecl* PokecomViewGeotagProc)();
  static PokecomViewGeotagProc gameExePokecomViewGeotag = NULL;
  static PokecomViewGeotagProc gameExePokecomViewGeotagReal = NULL;

  typedef int(__cdecl* InstHelpMenuProc)(void*);
  static InstHelpMenuProc gameExeInstHelpMenu = NULL;
  static InstHelpMenuProc gameExeInstHelpMenuReal = NULL;

  typedef void(__cdecl* PlaySEProc)(int);

  bool ScrollDownToAdvanceText = false;
  bool ScrollDownToCloseBacklog = true;
  int IruoSensitivity = 500;
  bool PointedThisFrame = false;
  bool LockMouseControls = false;
  uint32_t CarryInputToTheNextFrame = 0;

  MgsInputObj_t* InputObject = NULL;
  uint8_t* MouseEnabled = NULL;
  HWND* WindowHandle = NULL;
  uint8_t* MousePointing = NULL;

  int* TitleMenuSelectionIndex = NULL;
  int* SubMenuSelIndex = NULL;
  int* LoadMenuSelIndex = NULL;
  int* ExtrasMenuSelIndex = NULL;
  int* TitleSubMenuAnimCounter = NULL;

  int* MovieModeSelectionIndex = NULL;
  int* MovieModeIsPlaying = NULL;

  int* SelectedSaveEntryIndex = NULL;
  int* CurrentSaveMenuPage = NULL;
  int* DailyRecordsItemIndex = NULL;

  int* SysMesBoxMesNum = NULL;
  int* SysMesBoxChoiceNum = NULL;
  int* SysMesBoxChoiceIndex = NULL;
  int* SysMesBoxMesNumAlt = NULL;
  int* SysMesBoxChoiceNumAlt = NULL;
  int* SysMesBoxChoiceIndexAlt = NULL;

  int* CGLibraryPageIndex = NULL;
  int* CGLibraryItemIndex = NULL;

  int* AlbumCGScale = NULL;
  int* AlbumCGScaleMin = NULL;
  int* AlbumCGScaleMax = NULL;
  int* AlbumCGX = NULL;
  int* AlbumCGY = NULL;

  int* MusicModeCurrentIndex = NULL;
  int* MusicModeStartIndex = NULL;

  int* TipsMenuTabTipCount = NULL;
  int* TipsMenuCurrentTab = NULL;
  int* TipsMenuSelectedIndex = NULL;
  int* TipsMenuStartIndex = NULL;
  int* TipsMenuTipTextStartOffset = NULL;
  int* TipMenuTipTextHeight = NULL;
  int* TipsMenuTipData = NULL;
  int* TipsMenuSelectedTip = NULL;

  int* TwipoTweepCountInTabs = NULL;

  int* PokecomSelectionIndex = NULL;
  int* PokecomWindowCoords = NULL;

  int* ARNumberOfGeoTags = NULL;
  float* ARGeoTagsXYZCoords = NULL;
  int* ARSelectedGeoTag = NULL;
  int* ARPointingAtReport = NULL;
  int* ARDisplayedGeoTags = NULL;
  int* ARNumDisplayedGeoTags = NULL;
  int* ARSomeGeoTagArr = NULL;
  int* ARSomeGeoTagArr2 = NULL;
  int* ARSomeGeoTagArr3 = NULL;
  int* ARSomeGeoTagData = NULL;
  GetScreenCoordsProc gameExeGetScreenCoords;
  CountSC3CharactersProc gameExeCountSC3Characters;
  GetSC3StringByIDProc gameExeGetSC3StringByID;
  GeoTagHasLinkProc gameExeGeoTagHasLink;

  BacklogRecalcMovementProc gameExeBacklogRecalcMovement;
  int* BacklogDispPosMax = NULL;
  int* BacklogSelectedIndex = NULL;
  int* BacklogFirstFullDispLine = NULL;
  int* BacklogLastFullDispLine = NULL;

  int* MapNumPoints = NULL;
  int* MapPointsData = NULL;
  int* MapPointsIDs = NULL;
  int* MapSelectedPointIndex = NULL;
  int* MapPointIsDisplayed = NULL;

  SetConfigOptionProc gameExeSetConfigProc;
  int* ConfigUnkArray = NULL;
  int* ConfigSelectedItem = NULL;
  int* ConfigFullScreen = NULL;
  int* ConfigFullScreen1 = NULL;
  int* ConfigResolution = NULL;
  int* ConfigResolution1 = NULL;
  int* ConfigMessageSpeed = NULL;
  int* ConfigMessageSpeedCur = NULL;
  int* ConfigAutoModeDelay = NULL;
  int* ConfigAutoModeDelayCur = NULL;
  int* ConfigVoiceVolume = NULL;
  int* ConfigBGMVolume = NULL;
  int* ConfigSEVolume = NULL;
  int* ConfigMovieVolume = NULL;
  SetScreenResProc gameExeSetScreenRes;
  PlaySEProc gameExePlaySE;

  int* CGviewModeAlpha = NULL;
  int AutoSkipAlpha = 256;
  int AutoSkipAlphaStep = 0;

  bool SliderMoving = false;
  int MovingSliderId = -1;

  uint32_t* InputMask = NULL;
  uint32_t* InputMask3 = NULL;
  uint32_t* InputMask2 = NULL;
  uint32_t* InputMask4 = NULL;
  LPDIRECTINPUTDEVICEA* MouseDevice = NULL;
  int* GameScreenTopLeftX = NULL;
  int* GameScreenTopLeftY = NULL;
  int* GameScreenBottomRightX = NULL;
  int* GameScreenBottomRightY = NULL;

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
  int __cdecl instOptionHook(void* thread);
  int __cdecl mesDispWindowHook(int a1, int a2, int a3, int a4, 
                                int a5, int a6, int a7, int a8);
  int __cdecl pokecomViewGeotagHook();
  int __cdecl instHelpMenuHook(void* thread);

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

  void mouseSlider(int mouseX, int mouseY, int bX, int bY, int w, int h, int min, int max, int& target, int id) {
    if (MovingSliderId == -1 && mouseSelectHitTest(mouseX, mouseY, bX, bY, w, h) && (InputObject->mouseButtonsHeld & MouseLeftClick || InputObject->mouseButtons & MouseLeftClick)) {
      SliderMoving = true;
      PointedThisFrame = true;
      MovingSliderId = id;
    }
    if (MovingSliderId == id) {
      if ((InputObject->mouseButtonsHeld & MouseLeftClick || InputObject->mouseButtons & MouseLeftClick) && SliderMoving) {
        PointedThisFrame = true;
        if (mouseX > bX + w)
          target = max;
        else if (mouseX < bX)
          target = min;
        else {
          target = min + (((mouseX - bX) / (float)w) * (float)(max - min));
        }
      }
      else {
        SliderMoving = false;
        MovingSliderId = -1;
      }
    }
  }

  void mouseScrollBar(int mouseX, int mouseY, int bX, int bY, int w, int h, int minVal, int maxVal, int minPos, int maxPos, int& target, int id) {
    if (MovingSliderId == -1 && mouseSelectHitTest(mouseX, mouseY, bX, bY, w, h) && InputObject->mouseButtonsHeld & MouseLeftClick) {
      SliderMoving = true;
      PointedThisFrame = true;
      MovingSliderId = id;
    }
    if (MovingSliderId == id) {
      if (InputObject->mouseButtonsHeld & MouseLeftClick && SliderMoving) {
        PointedThisFrame = true;
        if (mouseY > maxPos + (h / 2))
          target = maxVal;
        else if (mouseY < (minPos + (h / 2)))
          target = minVal;
        else {
          mouseY -= h / 2;
          target = minVal + (((mouseY - minPos) / (float)(maxPos - minPos)) * (float)(maxVal - minVal));
        }
      }
      else {
        SliderMoving = false;
        MovingSliderId = -1;
      }
    }
  }

  bool customInputInit() {
    if (config["patch"].count("RNDMouseInput") != 1 ||
        config["patch"]["RNDMouseInput"].get<bool>() == false) {
        return true;
    }

    if (config["patch"].count("ScrollDownToAdvanceText") == 1 &&
        config["patch"]["ScrollDownToAdvanceText"].get<bool>() == true)
      ScrollDownToAdvanceText = true;

    if (config["patch"].count("ScrollDownToCloseBacklog") == 1 &&
        config["patch"]["ScrollDownToCloseBacklog"].get<bool>() == false)
      ScrollDownToCloseBacklog = false;

    if (config["patch"].count("iruoSensitivity") == 1)
      IruoSensitivity = config["patch"]["iruoSensitivity"].get<int>();

    ShowCursor(1);

    InputObject = (MgsInputObj_t*)sigScan("game", "useOfInputObject");
    MouseEnabled = (uint8_t*)sigScan("game", "useOfMouseEnabled");
    WindowHandle = (HWND*)sigScan("game", "useOfWindowHandle");
    MousePointing = (uint8_t*)sigScan("game", "useOfMousePointing");

    InputMask = (uint32_t*)sigScan("game", "useOfInputMask");
    InputMask2 = (uint32_t*)sigScan("game", "useOfInputMask2");
    InputMask3 = (uint32_t*)sigScan("game", "useOfInputMask3");
    InputMask4 = (uint32_t*)sigScan("game", "useOfInputMask4");
    MouseDevice = (LPDIRECTINPUTDEVICEA*)sigScan("game", "useOfMouseDevice");

    TitleMenuSelectionIndex = (int*)sigScan("game", "useOfTitleMenuSelectionIndex");
    SubMenuSelIndex = (int*)sigScan("game", "useOfSubMenuSelIndex");
    LoadMenuSelIndex = (int*)sigScan("game", "useOfLoadMenuSelIndex");
    ExtrasMenuSelIndex = (int*)sigScan("game", "useOfExtrasMenuSelIndex");
    TitleSubMenuAnimCounter = (int*)sigScan("game", "useOfTitleSubMenuAnimCounter");

    MovieModeSelectionIndex = (int*)sigScan("game", "useOfMovieModeSelectionIndex");
    MovieModeIsPlaying = (int*)sigScan("game", "useOfMovieModeIsPlaying");

    SelectedSaveEntryIndex = (int*)sigScan("game", "useOfSelectedSaveEntryIndex");
    CurrentSaveMenuPage = (int*)sigScan("game", "useOfCurrentSaveMenuPage");
    DailyRecordsItemIndex = (int*)sigScan("game", "useOfDailyRecordsItemIndex");

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
    TipsMenuTipData = (int*)sigScan("game", "useOfTipsMenuTipData");
    TipsMenuSelectedTip = (int*)sigScan("game", "useOfTipsMenuSelectedTip");

    TwipoTweepCountInTabs = (int*)sigScan("game", "useOfTwipoTweepCountInTabs");

    PokecomSelectionIndex = (int*)sigScan("game", "useOfPokecomSelectionIndex");
    PokecomWindowCoords = (int*)sigScan("game", "useOfPokecomWindowCoords");

    ARNumberOfGeoTags = (int*)sigScan("game", "useOfARNumberOfGeoTags");
    ARGeoTagsXYZCoords = (float*)sigScan("game", "useOfARGeoTagsXYZCoords");
    ARSelectedGeoTag = (int*)sigScan("game", "useOfARSelectedGeoTag");
    ARPointingAtReport = (int*)sigScan("game", "useOfARPointingAtReport");
    ARDisplayedGeoTags = (int*)sigScan("game", "useOfARDisplayedGeoTags");
    ARNumDisplayedGeoTags = (int*)sigScan("game", "useOfARNumDisplayedGeoTags");
    ARSomeGeoTagArr = (int*)sigScan("game", "useOfARSomeGeoTagArr");
    ARSomeGeoTagArr2 = (int*)sigScan("game", "useOfARSomeGeoTagArr2");
    ARSomeGeoTagArr3 = (int*)sigScan("game", "useOfARSomeGeoTagArr3");
    ARSomeGeoTagData = (int*)sigScan("game", "useOfARSomeGeoTagData");

    BacklogDispPosMax = (int*)sigScan("game", "useOfBacklogDispPosMax");
    BacklogSelectedIndex = (int*)sigScan("game", "useOfBacklogSelectedIndex");
    BacklogFirstFullDispLine = (int*)sigScan("game", "useOfBacklogFirstFullDispLine");
    BacklogLastFullDispLine = (int*)sigScan("game", "useOfBacklogLastFullDispLine");

    MapNumPoints = (int*)sigScan("game", "useOfMapNumPoints");
    MapPointsData = (int*)sigScan("game", "useOfMapPointsData");
    MapPointsIDs = (int*)sigScan("game", "useOfMapPointsIDs");
    MapSelectedPointIndex = (int*)sigScan("game", "useOfMapSelectedPointIndex");
    MapPointIsDisplayed = (int*)sigScan("game", "useOfMapPointIsDisplayed");

    ConfigUnkArray = (int*)sigScan("game", "useOfConfigUnkArray");
    ConfigSelectedItem = (int*)sigScan("game", "useOfConfigSelectedItem");
    ConfigFullScreen = (int*)sigScan("game", "useOfConfigFullScreen");
    ConfigFullScreen1 = (int*)sigScan("game", "useOfConfigFullScreen1");
    ConfigResolution = (int*)sigScan("game", "useOfConfigResolution");
    ConfigResolution1 = (int*)sigScan("game", "useOfConfigResolution1");
    ConfigMessageSpeed = (int*)sigScan("game", "useOfConfigMessageSpeed");
    ConfigMessageSpeedCur = (int*)sigScan("game", "useOfConfigMessageSpeedCur");
    ConfigAutoModeDelay = (int*)sigScan("game", "useOfConfigAutoModeDelay");
    ConfigAutoModeDelayCur = (int*)sigScan("game", "useOfConfigAutoModeDelayCur");
    ConfigVoiceVolume = (int*)sigScan("game", "useOfConfigVoiceVolume");
    ConfigBGMVolume = (int*)sigScan("game", "useOfConfigBGMVolume");
    ConfigSEVolume = (int*)sigScan("game", "useOfConfigSEVolume");
    ConfigMovieVolume = (int*)sigScan("game", "useOfConfigMovieVolume");

    CGviewModeAlpha = (int*)sigScan("game", "useOfCGviewModeAlpha");

    GameScreenTopLeftX = (int*)sigScan("game", "useOfGameScreenTopLeftX");
    GameScreenTopLeftY = (int*)sigScan("game", "useOfGameScreenTopLeftY");
    GameScreenBottomRightX = (int*)sigScan("game", "useOfGameScreenBottomRightX");
    GameScreenBottomRightY = (int*)sigScan("game", "useOfGameScreenBottomRightY");
   
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
    gameExeSetConfigProc = (SetConfigOptionProc)sigScan("game", "SetConfigProc");
    scanCreateEnableHook("game", "InstOption", (uintptr_t*)&gameExeInstOption,
                         (LPVOID)&instOptionHook,
                         (LPVOID*)&gameExeInstOptionReal);
    scanCreateEnableHook("game", "MesDispWindow", (uintptr_t*)&gameExeMesDispWindow,
                         (LPVOID)&mesDispWindowHook,
                         (LPVOID*)&gameExeMesDispWindowReal);
    scanCreateEnableHook("game", "PokecomViewGeotag", (uintptr_t*)&gameExePokecomViewGeotag,
                         (LPVOID)&pokecomViewGeotagHook,
                         (LPVOID*)&gameExePokecomViewGeotagReal);
    scanCreateEnableHook("game", "InstHelpMenu", (uintptr_t*)&gameExeInstHelpMenu,
                         (LPVOID)&instHelpMenuHook,
                         (LPVOID*)&gameExeInstHelpMenuReal);
    gameExeGetScreenCoords = (GetScreenCoordsProc)sigScan("game", "GetScreenCoords");
    gameExeCountSC3Characters = (CountSC3CharactersProc)sigScan("game", "CountSC3Characters");
    gameExeGetSC3StringByID = (GetSC3StringByIDProc)sigScan("game", "GetSC3StringByID");
    gameExeSetScreenRes = (SetScreenResProc)sigScan("game", "SetScreenRes");
    gameExeGeoTagHasLink = (GeoTagHasLinkProc)sigScan("game", "GeoTagHasLink");
    gameExePlaySE = (PlaySEProc)sigScan("game", "PlaySE");

    return true;
  }

  int __cdecl instHelpMenuHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
        *InputMask3 |= PAD1L1;
      }
      else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
        *InputMask3 |= PAD1R1;
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }
    return gameExeInstHelpMenuReal(thread);
  }

  int __cdecl mesDispWindowHook(int a1, int a2, int a3, int a4,
    int a5, int a6, int a7, int a8) {
    int ret = gameExeMesDispWindowReal(a1, a2, a3, a4, a5, a6, a7, a8);

    AutoSkipAlpha += AutoSkipAlphaStep;
    if (*MouseEnabled || AutoSkipAlphaStep) {
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      if (AutoSkipAlpha > 256) {
        AutoSkipAlpha = 256;
      }
      else if (AutoSkipAlpha < 0) {
        AutoSkipAlpha = 0;
      }

      if ((gameExeScrWork[SW_GAMEMODE] & 1) && gameExeGetFlag(CALENDAR_DISP) && !gameExeGetFlag(Pokecom_Disable) && !gameExeGetFlag(Pokecom_Open)) {
        if (mouseSelectHitTest(mouseX, mouseY, 1246, 153, 674, 86) && (InputObject->mouseButtons & MouseLeftClick))
          CarryInputToTheNextFrame |= PAD1R2;
      }

      int alpha = (AutoSkipAlpha * *CGviewModeAlpha) >> 8;

      if (mouseSelectHitTest(mouseX, mouseY, 13, 1064, 128, 38)) {
        drawSpriteHook(80, 3805.0f, 1937.0f, 135.0f, 45.0f, 8.0f, 1022.0f, 0xFFFFFF, alpha, 1);
        if (InputObject->mouseButtons & MouseLeftClick) {
          CarryInputToTheNextFrame |= PAD1X;
        }
      }
      else {
        drawSpriteHook(80, 3805.0f, 1987.0f, 135.0f, 45.0f, 8.0f, 1022.0f, 0xFFFFFF, alpha, 1);
      }

      if (mouseSelectHitTest(mouseX, mouseY, 152, 1064, 128, 38)) {
        drawSpriteHook(80, 3944.0f, 1937.0f, 135.0f, 45.0f, 147.0f, 1022.0f, 0xFFFFFF, alpha, 1);
        if (InputObject->mouseButtons & MouseLeftClick) {
          if (gameExeGetFlag(SF_MESALLSKIP))
            CarryInputToTheNextFrame |= PAD1B;
          else
            CarryInputToTheNextFrame |= PAD1R1;
        }
      }
      else {
        drawSpriteHook(80, 3944.0f, 1987.0f, 135.0f, 45.0f, 147.0f, 1022.0f, 0xFFFFFF, alpha, 1);
      }
    }

    return ret;
  }

  int __cdecl instOptionHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      mouseSlider(mouseX, mouseY, 163, 469, 156, 30, 256, 4096, *ConfigMessageSpeed, 0);
      *ConfigMessageSpeedCur = *ConfigMessageSpeed;
      mouseSlider(mouseX, mouseY, 163, 525, 156, 30, 2048, 256, *ConfigAutoModeDelay, 1);
      *ConfigAutoModeDelayCur = *ConfigAutoModeDelay;
      mouseSlider(mouseX, mouseY, 163, 697, 156, 30, 0, 128, *ConfigVoiceVolume, 2);
      mouseSlider(mouseX, mouseY, 163, 753, 156, 30, 0, 128, *ConfigBGMVolume, 3);
      mouseSlider(mouseX, mouseY, 163, 809, 156, 30, 0, 128, *ConfigSEVolume, 4);
      mouseSlider(mouseX, mouseY, 163, 865, 156, 30, 0, 128, *ConfigMovieVolume, 5);

      for (int i = 13; i < 28; i++) {
        int* val = (int*)ConfigUnkArray[9 * i + 6];
        mouseSlider(mouseX, mouseY, 914, 259 + ((i - 13) * 50), 159, 25, 0, 128, *val, i);
      }

      if (!SliderMoving) {
        int index = 0;
        for (int i = 0; i < 3; i++) {
          if (index == 1) index++;
          if (menuButtonHitTest(index++, mouseX, mouseY, 149, 249 + (i * 56), 637, 48, ConfigSelectedItem) && InputObject->mouseButtons & MouseLeftClick) {
            gameExePlaySE(18);
            gameExeSetConfigProc(&ConfigUnkArray[9 * *ConfigSelectedItem], 2);
            if (index == 3 || index == 4) {
              *ConfigFullScreen = *ConfigFullScreen1;
              *ConfigResolution1 = *ConfigResolution;
              gameExeSetScreenRes();
            }
          }
        }
        for (int i = 0; i < 3; i++) {
          if (menuButtonHitTest(index++, mouseX, mouseY, 149, 477 + (i * 56), 637, 48, ConfigSelectedItem) && InputObject->mouseButtons & MouseLeftClick) {
            if (index == 7) {
              gameExePlaySE(18);
              gameExeSetConfigProc(&ConfigUnkArray[9 * *ConfigSelectedItem], 2);
            }
          }
        }
        for (int i = 0; i < 6; i++) {
          if (menuButtonHitTest(index++, mouseX, mouseY, 149, 706 + (i * 56), 637, 48, ConfigSelectedItem) && InputObject->mouseButtons & MouseLeftClick) {
            if (index == 12 || index == 13) {
              gameExePlaySE(18);
              gameExeSetConfigProc(&ConfigUnkArray[9 * *ConfigSelectedItem], 2);
            }
          }
        }
        for (int i = 0; i < 15; i++) {
          if (menuButtonHitTest(index++, mouseX, mouseY, 904, 265 + (i * 50), 433, 41, ConfigSelectedItem)) {
            if (mouseHitTest(mouseX, mouseY, 1089, 265 + (i * 50), 247, 41) && InputObject->mouseButtons & MouseLeftClick) {
              *InputMask |= PAD1Y;
              gameExePlaySE(18);
            }
          }
        }
      }


      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }
    return gameExeInstOptionReal(thread);
  }

  int __cdecl pokecomARMainHook() {
    if (*MouseEnabled && gameExeGetFlag(Pokecom_ManualMode)) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      bool dontMoveThisFrame = false;
      auto pokecomWidth = *(signed __int16*)((char*)PokecomWindowCoords + (16 * gameExeScrWork[6374]) + 12);
      auto pokecomHeight = *(signed __int16*)((char*)PokecomWindowCoords + (16 * gameExeScrWork[6374]) + 14);
      auto pokecomX = (gameExeScrWork[6402] + gameExeScrWork[6378] + 960) - (pokecomWidth / 2);
      auto pokecomY = (gameExeScrWork[6403] + gameExeScrWork[6379] + 540) - (pokecomHeight / 2);

      float pokecomScaleX = pokecomWidth / 1920.0f;
      float pokecomScaleY = pokecomHeight / 1080.0f;

      if (*ARNumDisplayedGeoTags) {
        for (int i = 0; i < *ARNumDisplayedGeoTags; i++) {
          float x, y;
          int id = ARDisplayedGeoTags[i];
          int xyzCnt = 4 * id;
          // Oh shit, here we go again
          gameExeGetScreenCoords(1, &x, &y, ARGeoTagsXYZCoords[xyzCnt], 1.25f + ARGeoTagsXYZCoords[xyzCnt + 1], ARGeoTagsXYZCoords[xyzCnt + 2]);

          // Your guess is as good as mine
          int screenX = x - 84;
          int screenY = y - 138;

          screenX = (screenX * pokecomScaleX) + pokecomX;
          screenY = (screenY * pokecomScaleY) + pokecomY;

          int textWidth;
          if (ARSomeGeoTagArr2[ARSomeGeoTagArr[id]]) {
            textWidth = gameExeCountSC3Characters(gameExeGetSC3StringByID(13, *ARSomeGeoTagData + 200), 100, 0);
          }
          else {
            textWidth = getSc3StringDisplayWidthHook((char*)gameExeGetSC3StringByID(13, ARSomeGeoTagArr3[ARSomeGeoTagArr[id]]), 0, 22);
          }
          textWidth *= 2;

          if (menuButtonHitTest(id, mouseX, mouseY, screenX, screenY + 62, (textWidth + 80) * pokecomScaleX, 62, ARSelectedGeoTag)) {
            if (InputObject->mouseButtons & MouseLeftClick) {
              dontMoveThisFrame = true;
              *InputMask |= PAD1A;
            }
            gameExeScrWork[SW_GEOTAGSEL_INDEX] = *ARSelectedGeoTag;
            gameExeScrWork[SW_GEOTAGSEL_ID] = ARSomeGeoTagArr[*ARSelectedGeoTag];
          }
        }
      }

      if (gameExeGetFlag(KRep_SearchMode) && *ARPointingAtReport) {
        int screenX = 923 * pokecomScaleX + pokecomX;
        int screenY = 581 * pokecomScaleY + pokecomY;

        if (mouseSelectHitTest(mouseX, mouseY, screenX, screenY, 72 * pokecomScaleX, 70 * pokecomScaleY) && (InputObject->mouseButtons & MouseLeftClick))
          *InputMask |= PAD1A;
      }

      int axisMultiplier = gameExeScrWork[SW_AR_ANGLE_C] / IruoSensitivity;
      if ((InputObject->mouseButtonsHeld & MouseLeftClick) && !dontMoveThisFrame) {
        gameExeScrWork[SW_AR_ELV] -= InputObject->mouseYAxis * axisMultiplier;
        gameExeScrWork[SW_AR_ROT] += InputObject->mouseXAxis * axisMultiplier;

        // Super mode == unrestrained look around
        if (gameExeGetFlag(AR_SUPERMODE)) {
          if (gameExeScrWork[SW_AR_ELV] < -80000)
            gameExeScrWork[SW_AR_ELV] = -80000;
          if (gameExeScrWork[SW_AR_ELV] >= 80000)
            gameExeScrWork[SW_AR_ELV] = 80000;
          if (gameExeScrWork[SW_AR_ROT] <= -180000)
            gameExeScrWork[SW_AR_ROT] += 360000;
          if (gameExeScrWork[SW_AR_ROT] > 180000)
            gameExeScrWork[SW_AR_ROT] -= 360000;
        }
        else {
          if (gameExeScrWork[SW_AR_ELV] < gameExeScrWork[SW_AR_ELVMIN])
            gameExeScrWork[SW_AR_ELV] = gameExeScrWork[SW_AR_ELVMIN];
          if (gameExeScrWork[SW_AR_ELV] > gameExeScrWork[SW_AR_ELVMAX])
            gameExeScrWork[SW_AR_ELV] = gameExeScrWork[SW_AR_ELVMAX];
          if (gameExeScrWork[SW_AR_ROT] < gameExeScrWork[SW_AR_ROTMIN])
            gameExeScrWork[SW_AR_ROT] = gameExeScrWork[SW_AR_ROTMIN];
          if (gameExeScrWork[SW_AR_ROT] > gameExeScrWork[SW_AR_ROTMAX])
            gameExeScrWork[SW_AR_ROT] = gameExeScrWork[SW_AR_ROTMAX];
        }
      }

      if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
        gameExeScrWork[SW_AR_ANGLE_C] = gameExeScrWork[SW_AR_ANGLE_C] - 1000 > 10000 ? gameExeScrWork[SW_AR_ANGLE_C] - 1000 : 10000;
      }
      else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
        gameExeScrWork[SW_AR_ANGLE_C] = gameExeScrWork[SW_AR_ANGLE_C] + 1000 < gameExeScrWork[SW_AR_ANGLE_M] - 500 ? gameExeScrWork[SW_AR_ANGLE_C] + 1000 : gameExeScrWork[SW_AR_ANGLE_M] - 500;
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        CarryInputToTheNextFrame |= PAD1B;
      }
    }

    return gameExePokecomARMainReal();
  }

  int __cdecl pokecomViewGeotagHook() {
    if (*MouseEnabled && gameExeGetFlag(Pokecom_ManualMode)) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      // Yes, very good hack, yes
      // There are only 3 geotags with links, 2 of them identical
      // So let's just hack up the width of the hit box

      int width = 678;
      if (gameExeScrWork[SW_GEOTAGSEL_ID] == 59)
        width = 795;
      if (gameExeGeoTagHasLink(5, gameExeScrWork[SW_GEOTAGSEL_ID], 0, 0) && mouseSelectHitTest(mouseX, mouseY, 87, 741, width, 55)
        && InputObject->mouseButtons & MouseLeftClick) {
        *InputMask |= PAD1Y;
      }
      else if (InputObject->mouseButtons & MouseLeftClick || InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }

    return gameExePokecomViewGeotagReal();
  }

  int __cdecl pokecomMapMainHook() {
    if (*MouseEnabled && gameExeGetFlag(Pokecom_ManualMode)) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      // 6510 - MapPosX
      // 6511 - MapPosY
      if (InputObject->mouseButtonsHeld & MouseLeftClick) {
        gameExeScrWork[SW_MAP_POSX] -= (InputObject->mouseXAxis) * 2;
        gameExeScrWork[SW_MAP_POSY] -= (InputObject->mouseYAxis) * 2;
      }

      // 6517 - MapSize
      if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
        gameExeScrWork[SW_MAP_SIZE] = gameExeScrWork[SW_MAP_SIZE] - 50 > 500 ? gameExeScrWork[SW_MAP_SIZE] - 50 : gameExeScrWork[SW_MAP_SIZE];
      }
      else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
        gameExeScrWork[SW_MAP_SIZE] = gameExeScrWork[SW_MAP_SIZE] + 50 < 2000 ? gameExeScrWork[SW_MAP_SIZE] + 50 : gameExeScrWork[SW_MAP_SIZE];
      }

      if ((int8_t)gameExeScrWork[SW_MAP_MODE] < 0) {
        for (int i = 0; i < *MapNumPoints; i++) {
          if (MapPointIsDisplayed[i]) {
            int id = MapPointsIDs[i];
            int screenX = 1920 * (MapPointsData[5 * id + 1] - (gameExeScrWork[SW_MAP_POSX] - (1920 * gameExeScrWork[SW_MAP_SIZE] / 1000) / 2)) / (1920 * gameExeScrWork[SW_MAP_SIZE] / 1000) - 84;
            int screenY = 1080 * (MapPointsData[5 * id + 2] - (gameExeScrWork[SW_MAP_POSY] - (1080 * gameExeScrWork[SW_MAP_SIZE] / 1000) / 2)) / (1080 * gameExeScrWork[SW_MAP_SIZE] / 1000) - 138;

            int height = 93;
            int width = 150;
            width += (2 * MapPointsData[5 * id + 3] + 54) > 96 ? 2 * MapPointsData[5 * id + 3] - 42 : 0;
            if (!gameExeScrWork[6377]) {
              auto pokecomWidth = *(signed __int16*)((char*)PokecomWindowCoords + (16 * gameExeScrWork[6374]) + 12);
              auto pokecomHeight = *(signed __int16*)((char*)PokecomWindowCoords + (16 * gameExeScrWork[6374]) + 14);
              auto pokecomX = 960 - (pokecomWidth / 2);
              auto pokecomY =  540 - (pokecomHeight / 2);

              float pokecomScaleX = pokecomWidth / 1920.0f;
              float pokecomScaleY = pokecomHeight / 1080.0f;
              screenX = (screenX * pokecomScaleX) + pokecomX + 12;
              screenY = (screenY * pokecomScaleY) + pokecomY + 54;
              height = 54;
              width *= pokecomScaleX;
            } else {
              screenX += 20;
              screenY += height;
            }

            if (menuButtonHitTest(i, mouseX, mouseY, screenX, screenY, width, height, MapSelectedPointIndex)) {
              if (InputObject->mouseButtons & MouseLeftClick) {
                *InputMask |= PAD1A;
              }
            }
          }
        }
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }

    return gameExePokecomMapMainReal();
  }

  int __cdecl pokecomDocMainHook() {
    if (*MouseEnabled && gameExeGetFlag(Pokecom_ManualMode)) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      // Doc mode - selection
      if (gameExeScrWork[SW_DOC_MODE] == 0) {
        // 6481 - Number of docs unlocked
        // 6479 - Currently selected doc
        for (int i = 0; i < gameExeScrWork[SW_DOC_UNLOCKEDNUM]; i++) {
          int j = 0;
          int r = i;
          if (i > 5) {
            r = i - 6;
            j = 1;
          }
          if (menuButtonHitTest(i, mouseX, mouseY, 72 + (r * 232), 346 + (j * 334), 232, 207, &gameExeScrWork[SW_DOC_SELECTED]) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
        }
      }
      else if (gameExeScrWork[SW_DOC_MODE] == 1) { // Doc mode - reading
     // 6476 - Doc current pos
     // 6473 - Doc max pos
        if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
          gameExeScrWork[SW_DOC_CURTARGET] = gameExeScrWork[SW_DOC_CURTARGET] <= 8 ? 8 : gameExeScrWork[SW_DOC_CURTARGET] - 24;
        }
        else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
          gameExeScrWork[SW_DOC_CURTARGET] = gameExeScrWork[SW_DOC_CURTARGET] + 24 >= 32 * gameExeScrWork[SW_DOC_CURMAX] ? 32 * gameExeScrWork[SW_DOC_CURMAX] : gameExeScrWork[SW_DOC_CURTARGET] + 24;
        }
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
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
      if (gameExeScrWork[SW_TWIPOMODE] == 0) {
        // Twipo tabs
        for (int i = 0; i < 4; i++) {
          if (mouseSelectHitTest(mouseX, mouseY, 449, 385 + (i * 157), 59, 157) && (InputObject->mouseButtons & MouseLeftClick))
            gameExeScrWork[SW_TWIPOCURTAB] = i;
        }

        // Scroll tweeps up/down by clicking on them
        if ((TwipoTweepCountInTabs[gameExeScrWork[SW_TWIPOCURTAB]] - gameExeScrWork[SW_TWIPOCURTW]) > 1) {
          if (mouseSelectHitTest(mouseX, mouseY, 508, 437, 383, 209) && (InputObject->mouseButtons & MouseLeftClick)) {
            *InputMask4 |= PAD1RIGHTSTICKUP;
          }
        }
        if (gameExeScrWork[SW_TWIPOCURTW] > 0) {
          if (mouseSelectHitTest(mouseX, mouseY, 508, 849, 383, 209) && (InputObject->mouseButtons & MouseLeftClick)) {
            *InputMask4 |= PAD1RIGHTSTICKDOWN;
          }
        }

        // Scroll tweeps up/down with scroll wheel
        if (mouseHitTest(mouseX, mouseY, 166, 1079, 675, 1080)) {
          if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
            *InputMask4 |= PAD1RIGHTSTICKUP;
          }
          else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
            *InputMask4 |= PAD1RIGHTSTICKDOWN;
          }
        }
      }
      else if (gameExeScrWork[SW_TWIPOMODE] == 1) {  // Reply mode
     // Reply button
        if (mouseSelectHitTest(mouseX, mouseY, 1533, 1052, 357, 118) && (InputObject->mouseButtons & MouseLeftClick)) {
          *InputMask |= PAD1A;
        }

        // Scroll choices by clicking on up/down arrows
        if (mouseSelectHitTest(mouseX, mouseY, 1332, 497, 94, 70) && (InputObject->mouseButtons & MouseLeftClick)) {
          *InputMask |= PAD1RIGHTSTICKUP;
        }
        if (mouseSelectHitTest(mouseX, mouseY, 1332, 950, 94, 70) && (InputObject->mouseButtons & MouseLeftClick)) {
          *InputMask |= PAD1RIGHTSTICKDOWN;
        }

        // Scroll choices with scroll wheel
        if (mouseHitTest(mouseX, mouseY, 883, 915, 986, 452)) {
          if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
            *InputMask |= PAD1RIGHTSTICKUP;
          }
          else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
            *InputMask |= PAD1RIGHTSTICKDOWN;
          }
        }
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }

    return gameExeTwipoMainReal();
  }

  int __cdecl backlogMenuMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      int scrollBarY = *BacklogDispPosMax <= 600 ? 138 : (793 * *BacklogDispPos / (*BacklogDispPosMax - 600)) + 138;
      mouseScrollBar(mouseX, mouseY, 1820, scrollBarY + 48, 11, 48, 0, *BacklogDispPosMax - 600, 138, 931, *BacklogDispPos, 0);

      if (!SliderMoving) {
        int startLineIndex = *BacklogFirstFullDispLine > 0 ? *BacklogFirstFullDispLine - 1 : 0;
        for (int i = startLineIndex; i < *BacklogLastFullDispLine + 2; i++) {
          int lineY = (float)(BacklogDispLinePosY[i] - *BacklogDispPos + 54 + BacklogDispLineSize[i] + 16) * 1.5f;
          if (menuButtonHitTest(i, mouseX, mouseY, 538, lineY, 1725, (BacklogDispLineSize[i] + 16) * 1.5f, BacklogSelectedIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
        }

        if (ScrollDownToCloseBacklog && (*BacklogDispPos >= *BacklogDispPosMax - 600)
          && InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
          *InputMask |= PAD1B;
        }

        if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
          *BacklogDispPos = *BacklogDispPos <= 0 ? 0 : *BacklogDispPos - 48;
          gameExeBacklogRecalcMovement();
        }
        else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
          *BacklogDispPos = *BacklogDispPos + 48 >= *BacklogDispPosMax - 608 ? *BacklogDispPosMax - 600 : *BacklogDispPos + 48;
          gameExeBacklogRecalcMovement();
        }

        if (InputObject->mouseButtons & MouseRightClick) {
          *InputMask |= PAD1B;
        }
      }
      else {
        gameExeBacklogRecalcMovement();
      }
    }

    return gameExeBacklogMenuMainReal();
  }

  int __cdecl musicMenuMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      int scrollBarY = 552 * *MusicModeStartIndex / 22 + 357;
      mouseScrollBar(mouseX, mouseY, 1815, scrollBarY + 60, 10, 60, 0, 22, 357, 909, *MusicModeStartIndex, 0);

      if (!SliderMoving) {
        for (int i = 0; i < 16; i++) {
          if (menuButtonHitTest(i, mouseX, mouseY, 80, 399 + (i * 38), 1690, 40, MusicModeCurrentIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
        }

        if (mouseSelectHitTest(mouseX, mouseY, 1600, 198, 222, 22) && (InputObject->mouseButtons & MouseLeftClick))
          *InputMask |= PAD1Y;

        if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
          *MusicModeStartIndex = *MusicModeStartIndex == 0 ? 0 : *MusicModeStartIndex - 1;
        }
        else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
          *MusicModeStartIndex = *MusicModeStartIndex == 22 ? 22 : *MusicModeStartIndex + 1;
        }

        if (InputObject->mouseButtons & MouseRightClick) {
          *InputMask |= PAD1B;
        }
      }
    }

    return gameExeMusicMenuMainReal();
  }

  void __cdecl pokecomMainMenuMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      if (menuButtonHitTest(0, mouseX, mouseY, 408, 382, 275, 154, PokecomSelectionIndex) ||
        menuButtonHitTest(1, mouseX, mouseY, 684, 382, 275, 154, PokecomSelectionIndex)   ||
        menuButtonHitTest(3, mouseX, mouseY, 1236, 382, 275, 154, PokecomSelectionIndex)  ||
        menuButtonHitTest(7, mouseX, mouseY, 1236, 538, 275, 154, PokecomSelectionIndex)  ||
        menuButtonHitTest(9, mouseX, mouseY, 684, 693, 275, 154, PokecomSelectionIndex)   ||
        menuButtonHitTest(10, mouseX, mouseY, 960, 693, 275, 154, PokecomSelectionIndex)  ||
        menuButtonHitTest(12, mouseX, mouseY, 408, 849, 275, 154, PokecomSelectionIndex)  ||
        menuButtonHitTest(14, mouseX, mouseY, 960, 849, 275, 154, PokecomSelectionIndex)  ||
        menuButtonHitTest(15, mouseX, mouseY, 1236, 849, 275, 154, PokecomSelectionIndex)) {
        if (InputObject->mouseButtons & MouseLeftClick) {
          *InputMask |= PAD1A;
        }
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }

    gameExePokecomMainMenuMainReal();
  }

  int __cdecl cgViewModeMainHook() {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      // 2000 is just an arbitrary speed multiplier
      if (InputObject->mouseButtonsHeld & MouseLeftClick) {
        *AlbumCGX -= InputObject->mouseXAxis * 2000;
        *AlbumCGY -= InputObject->mouseYAxis * 2000;
      }

      if (InputObject->mouseButtons1 & MouseLeftClick)
        *InputMask |= PAD1A;

      if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
        *AlbumCGScale = *AlbumCGScale >= *AlbumCGScaleMax ? *AlbumCGScaleMax : *AlbumCGScale + 100;
      }
      else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
        *AlbumCGScale = *AlbumCGScale <= *AlbumCGScaleMin ? *AlbumCGScaleMin : *AlbumCGScale - 100;
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
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
          if (menuButtonHitTest(id++, mouseX, mouseY, 128 + (j * 426), 427 + (i * 257), 386, 218, CGLibraryItemIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
        }
      }

      if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
        *CGLibraryPageIndex = *CGLibraryPageIndex == 0 ? 7 : *CGLibraryPageIndex - 1;
      }
      else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
        *CGLibraryPageIndex = *CGLibraryPageIndex == 7 ? 0 : *CGLibraryPageIndex + 1;
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }

    return gameExeAlbumMenuMainReal();
  }

  int __cdecl instTipsHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      if (TipsMenuTipData[6 * *TipsMenuSelectedTip] & 1 && *TipMenuTipTextHeight > 370) {
        int scrollBarY = 530 * *TipsMenuTipTextStartOffset / (*TipMenuTipTextHeight - 370) + 374;
        mouseScrollBar(mouseX, mouseY, 1822, scrollBarY + 60, 10, 60, 0, *TipMenuTipTextHeight - 370, 376, 906, *TipsMenuTipTextStartOffset, 0);
      }

      if (TipsMenuTabTipCount[*TipsMenuCurrentTab] > 22) {
        int scrollBarY = (671 * *TipsMenuStartIndex / (TipsMenuTabTipCount[*TipsMenuCurrentTab] - 22)) + 222;
        mouseScrollBar(mouseX, mouseY, 555, scrollBarY + 60, 10, 60, 0, TipsMenuTabTipCount[*TipsMenuCurrentTab] - 22, 224, 895, *TipsMenuStartIndex, 1);
        *((float*)(TipsMenuStartIndex + 3)) = (float)*TipsMenuStartIndex;
      }

      if (!SliderMoving) {
        int count = TipsMenuTabTipCount[*TipsMenuCurrentTab] > 22 ? 22 : TipsMenuTabTipCount[*TipsMenuCurrentTab];
        for (int i = 0; i < count; i++) {
          if (menuButtonHitTest(i, mouseX, mouseY, 82, 287 + (i * 32), 467, 32, TipsMenuSelectedIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
        }

        for (int i = 0; i < 4; i++) {
          if (TipsMenuTabTipCount[i]) {
            if (mouseSelectHitTest(mouseX, mouseY, 80 + (i * 126), 189, 126, 30) && (InputObject->mouseButtons & MouseLeftClick))
              *TipsMenuCurrentTab = i;
          }
        }

        int maxStartIndex = TipsMenuTabTipCount[*TipsMenuCurrentTab] - 22;
        if ((TipsMenuTabTipCount[*TipsMenuCurrentTab] > 22) && mouseHitTest(mouseX, mouseY, 97, 1013, 560, 890)) {
          if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
            *TipsMenuStartIndex = *TipsMenuStartIndex == 0 ? 0 : *TipsMenuStartIndex - 1;
            // Why???????
            *((float*)(TipsMenuStartIndex + 3)) = (float)*TipsMenuStartIndex;
          }
          else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
            *TipsMenuStartIndex = *TipsMenuStartIndex == maxStartIndex ? maxStartIndex : *TipsMenuStartIndex + 1;
            *((float*)(TipsMenuStartIndex + 3)) = (float)*TipsMenuStartIndex;
          }
        }

        if ((*TipMenuTipTextHeight > 370) && mouseHitTest(mouseX, mouseY, 664, 1013, 1100, 788)) {
          if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
            *TipsMenuTipTextStartOffset = (*TipsMenuTipTextStartOffset - 20) <= 0 ? 0 : *TipsMenuTipTextStartOffset - 20;
          }
          else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
            *TipsMenuTipTextStartOffset = (*TipsMenuTipTextStartOffset + 20) > (*TipMenuTipTextHeight - 370) ? *TipMenuTipTextHeight - 370 : *TipsMenuTipTextStartOffset + 20;
          }
        }

        if (InputObject->mouseButtons & MouseRightClick) {
          *InputMask |= PAD1B;
        }
      }
    }

    int ret = gameExeInstTipsReal(thread);
    return ret;
  }

  int __cdecl instSystemMenuHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      for (int i = 0; i < 11; i++) {
        if (menuButtonHitTest(i, mouseX, mouseY, 40 + (i * 48), 308 + (i * 77), 766, 65, &gameExeScrWork[SW_SYSMENUSELNO]) && (InputObject->mouseButtons & MouseLeftClick))
          *InputMask |= PAD1A;
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
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
        if (menuButtonHitTest(0, mouseX, mouseY, 1225, buttonY, 158, 39, selIndexVar) && (InputObject->mouseButtons & MouseLeftClick))
          *InputMask |= PAD1A;
      } break;
      case 2: {
        if (menuButtonHitTest(0, mouseX, mouseY, 1057, buttonY, 158, 39, selIndexVar) ||
          menuButtonHitTest(1, mouseX, mouseY, 1225, buttonY, 158, 39, selIndexVar))
          if (InputObject->mouseButtons & MouseLeftClick) {
            *InputMask |= PAD1A;
          }
      } break;
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
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
        int id = 0;
        for (int i = 0; i < 6; i++) {
          for (int j = 0; j < 7; j++) {
            if (menuButtonHitTest(id++, mouseX, mouseY, 378 + (j * 202), 349 + (i * 123), 198, 122, selIndexVar) && (InputObject->mouseButtons & MouseLeftClick))
              *InputMask |= PAD1A;
          }
        }

        // Page switch with scroll wheel
        if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
          *InputMask2 |= PAD1L1;
        }
        else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
          *InputMask2 |= PAD1R1;
        }
      }
      else { // Save/Load
        int id = 0;
        for (int i = 0; i < 2; i++) {
          for (int j = 0; j < 4; j++) {
            if (menuButtonHitTest(id++, mouseX, mouseY, 56 + (i * 910), 365 + (j * 201), 896, 178, SelectedSaveEntryIndex) && (InputObject->mouseButtons & MouseLeftClick))
              *InputMask |= PAD1A;
          }
        }

        // Page switch with scroll wheel
        if (InputObject->mouseButtonsHeld & MouseScrollWheelUp) {
          *CurrentSaveMenuPage = *CurrentSaveMenuPage == 0 ? 5 : *CurrentSaveMenuPage - 1;
        }
        else if (InputObject->mouseButtonsHeld & MouseScrollWheelDown) {
          *CurrentSaveMenuPage = *CurrentSaveMenuPage == 5 ? 0 : *CurrentSaveMenuPage + 1;
        }
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }

    return gameExeInstSaveMenuReal(thread);
  }

  int __cdecl movieModeDispHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;

      if (!*MovieModeIsPlaying) {
        int id = 0;
        for (int i = 0; i < 2; i++) {
          for (int j = 0; j < 3; j++) {
            if (menuButtonHitTest(id++, mouseX, mouseY, 143 + (j * 560), 560 + (i * 336), 514, 291, MovieModeSelectionIndex) && (InputObject->mouseButtons & MouseLeftClick))
              *InputMask |= PAD1A;
          }
        }
      } else if (InputObject->mouseButtons & MouseLeftClick) {
        *InputMask |= PAD1A;
      }

      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }

    return gameExeMovieModeDispReal(thread);
  }

  int __cdecl instTitleMenuHook(void* thread) {
    if (*MouseEnabled) {
      LockMouseControls = true;
      int mouseX = InputObject->scaledMouseX;
      int mouseY = InputObject->scaledMouseY;
      // Flag 810 means Phase Aki menu item has been unlocked, 811 - Last Phase
      // Flags: 860 - Album, 863 - Movie, 864 - Sound

      if (!*TitleSubMenuAnimCounter) {
        switch (*SubMenuSelIndex) {
        case 0: {
          int flagVal = gameExeGetFlag(OPEN_START2) + gameExeGetFlag(OPEN_START3);
          if (menuButtonHitTest(0, mouseX, mouseY, 72, 591 + (0 * 48), 333, 41, TitleMenuSelectionIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
          if (gameExeGetFlag(OPEN_START2)) {
            if (menuButtonHitTest(1, mouseX, mouseY, 72, 591 + (1 * 48), 321, 41, TitleMenuSelectionIndex) && (InputObject->mouseButtons & MouseLeftClick))
              *InputMask |= PAD1A;
          }
          if (gameExeGetFlag(OPEN_START3)) {
            if (menuButtonHitTest(2, mouseX, mouseY, 72, 591 + (2 * 48), 311, 41, TitleMenuSelectionIndex) && (InputObject->mouseButtons & MouseLeftClick))
              *InputMask |= PAD1A;
          }
          if (menuButtonHitTest(1 + flagVal, mouseX, mouseY, 72, 591 + ((1 + flagVal) * 48), 262, 41, TitleMenuSelectionIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
          if (menuButtonHitTest(2 + flagVal, mouseX, mouseY, 72, 591 + ((2 + flagVal) * 48), 186, 41, TitleMenuSelectionIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
          if (menuButtonHitTest(3 + flagVal, mouseX, mouseY, 72, 591 + ((3 + flagVal) * 48), 198, 41, TitleMenuSelectionIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
          if (menuButtonHitTest(4 + flagVal, mouseX, mouseY, 72, 591 + ((4 + flagVal) * 48), 151, 41, TitleMenuSelectionIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
          if (menuButtonHitTest(5 + flagVal, mouseX, mouseY, 72, 591 + ((5 + flagVal) * 48), 262, 41, TitleMenuSelectionIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
        } break;
        case 1: {
          if (menuButtonHitTest(0, mouseX, mouseY, 72, 591, 366, 41, LoadMenuSelIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
          if (menuButtonHitTest(1, mouseX, mouseY, 72, 591 + (1 * 48), 150, 41, LoadMenuSelIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
        } break;
        case 2: {
          int flagVal = (gameExeGetFlag(ALBUM_ENA) + gameExeGetFlag(MOVIE_ENA) + gameExeGetFlag(MUSIC_ENA));
          if (menuButtonHitTest(0, mouseX, mouseY, 72, 591, 287, 41, ExtrasMenuSelIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
          if (gameExeGetFlag(ALBUM_ENA) && gameExeGetFlag(MOVIE_ENA) && gameExeGetFlag(MUSIC_ENA)) {
            if (menuButtonHitTest(1, mouseX, mouseY, 72, 591 + (1 * 48), 299, 41, ExtrasMenuSelIndex) && (InputObject->mouseButtons & MouseLeftClick))
              *InputMask |= PAD1A;
            if (menuButtonHitTest(2, mouseX, mouseY, 72, 591 + (2 * 48), 384, 41, ExtrasMenuSelIndex) && (InputObject->mouseButtons & MouseLeftClick))
              *InputMask |= PAD1A;
            if (menuButtonHitTest(3, mouseX, mouseY, 72, 591 + (3 * 48), 365, 41, ExtrasMenuSelIndex) && (InputObject->mouseButtons & MouseLeftClick))
              *InputMask |= PAD1A;
          }
          if (menuButtonHitTest(1 + flagVal, mouseX, mouseY, 72, 591 + ((1 + flagVal) * 48), 240, 41, ExtrasMenuSelIndex) && (InputObject->mouseButtons & MouseLeftClick))
            *InputMask |= PAD1A;
        } break;
        }
      }



      if (InputObject->mouseButtons & MouseRightClick) {
        *InputMask |= PAD1B;
      }
    }

    return gameExeInstTitleMenuReal(thread);
  }

  int __fastcall mgsInputExecuteServerHook(MgsInputObj_t* pThis) {
    if (*ConfigFullScreen) {
      int x = GetSystemMetrics(SM_CXSCREEN);
      int y = GetSystemMetrics(SM_CYSCREEN);
      int width = min(x, ceilf((float)y * 1.7777f));
      int height = min(y, ceilf((float)x / 1.7777f));
      int boundX = (x - width) / 2;
      int boundY = (y - height) / 2;
      *GameScreenTopLeftX = boundX;
      *GameScreenTopLeftY = boundY;
      *GameScreenBottomRightX = width + boundX;
      *GameScreenBottomRightY = height + boundY;
    }

    int ret = gameExeMgsInputExecuteServerReal(pThis);

    if (!*MouseEnabled && pThis->mouseFocused && (InputObject->mouseButtons & MouseLeftClick)) {
      InputObject->mouseButtons = 0;
      *MouseEnabled = 1;
      AutoSkipAlphaStep = 32;
    }

    if (GetActiveWindow() != *WindowHandle) {
      *MouseEnabled = 0;
      SliderMoving = false;
      MovingSliderId = -1;      
    }

    if (!*MouseEnabled) {
      AutoSkipAlphaStep = -32;
    }

    *MousePointing = PointedThisFrame;
    PointedThisFrame = false;

    return ret;
  }

  int __cdecl padMainHook() {
    int ret = gameExePADmainReal();

    if (*MouseEnabled && !LockMouseControls) {
      // ScrWork[SW_GAMEMODE] & 1 is the main novel mode

      if (InputObject->mouseButtons & MouseLeftClick) {
        if (gameExeScrWork[SW_GAMEMODE] & 1) {
          if (!mouseHitTest(InputObject->scaledMouseX, InputObject->scaledMouseY, 13, 1064, 270, 40) &&
              !(mouseSelectHitTest(InputObject->scaledMouseX, InputObject->scaledMouseY, 1246, 153, 674, 86) 
                && gameExeGetFlag(CALENDAR_DISP) && !gameExeGetFlag(Pokecom_Disable) && !gameExeGetFlag(Pokecom_Open))) {
            *InputMask |= PAD1A;
          }
        } else {
          *InputMask |= PAD1A;
        }
      }

      if ((InputObject->mouseButtons & MouseLeftClick) && gameExeGetFlag(SF_MESALLSKIP) && gameExeScrWork[SW_GAMEMODE] & 1) {
        *InputMask |= PAD1R1;
      }
      
      if (InputObject->mouseButtons & MouseRightClick) {
        if (gameExeScrWork[SW_GAMEMODE] & 1)
          *InputMask |= PAD1START;
        else
          *InputMask |= PAD1B;
      }

      if (InputObject->mouseButtons & MouseMiddleClick && gameExeScrWork[SW_GAMEMODE] & 1) {
        *InputMask |= PAD1R2;
      }
      
      if (InputObject->mouseButtons & MouseScrollWheelUp) {
        *InputMask |= PAD1Y;
      }

      if (ScrollDownToAdvanceText && (InputObject->mouseButtons & MouseScrollWheelDown)) {
        *InputMask |= PAD1A;
      }
    }

    LockMouseControls = false;

    if (CarryInputToTheNextFrame) {
      *InputMask |= CarryInputToTheNextFrame;
      CarryInputToTheNextFrame = 0;
    }

    return ret;
  }
}
}  // namespace lb