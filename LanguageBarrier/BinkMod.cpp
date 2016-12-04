#include "LanguageBarrier.h"
#include <Simd/SimdLib.h>
#include <fstream>
#include <unordered_map>
#include "BinkMod.h"
#include "Config.h"
#include "Game.h"
#include <csri/csri.h>
#include <xmmintrin.h>

// partial
typedef struct BINK {
  uint32_t Width;         // Width (1 based, 640 for example)
  uint32_t Height;        // Height (1 based, 480 for example)
  uint32_t Frames;        // Number of frames (1 based, 100 = 100 frames)
  uint32_t FrameNum;      // Frame to *be* displayed (1 based)
  uint32_t LastFrameNum;  // Last frame decompressed or skipped (1 based)

  uint32_t FrameRate;     // Frame Rate Numerator
  uint32_t FrameRateDiv;  // Frame Rate Divisor (frame rate=numerator/divisor)
} BINK;

typedef BINK*(__stdcall* BINKOPEN)(const char* name, uint32_t flags);
typedef void(__stdcall* BINKCLOSE)(BINK* bnk);
typedef int32_t(__stdcall* BINKCOPYTOBUFFER)(BINK* bnk, void* dest,
                                             int32_t destpitch,
                                             uint32_t destheight,
                                             uint32_t destx, uint32_t desty,
                                             uint32_t flags);
typedef int32_t(__stdcall* BINKSETVOLUME)(BINK* bnk, int32_t unk1,
                                          int32_t volume);
static BINKOPEN BinkOpen = NULL;
static BINKCLOSE BinkClose = NULL;
static BINKCOPYTOBUFFER BinkCopyToBuffer = NULL;
static BINKSETVOLUME BinkSetVolume = NULL;

typedef struct {
  char isOpen;
  char shouldPlay;
  char isPaused;
  __declspec(align(2)) char byte4;
  char gap5[11];
  int dword10;
  char gap14[12];
  BINK* pBink;
  char gap24[264];
  void* surface;
  int field_130;
  float volume;
} MgsBink_t;

typedef int(__thiscall* MgsBinkSetPausedProc)(MgsBink_t* pThis, char paused);
static MgsBinkSetPausedProc gameExeMgsBinkSetPaused = NULL;
static MgsBinkSetPausedProc gameExeMgsBinkSetPausedReal = NULL;

typedef struct {
  uint32_t bgmId;
  void* framebuffer;
  uint32_t destwidth;
  uint32_t destheight;
  uint32_t lastFrameNum;
  csri_inst* csri;
} BinkModState_t;

static std::unordered_map<BINK*, BinkModState_t*> stateMap;

static __m128i MaskFF000000 = _mm_set1_epi32(0xFF000000);

namespace lb {
BINK* __stdcall BinkOpenHook(const char* name, uint32_t flags);
void __stdcall BinkCloseHook(BINK* bnk);
int32_t __stdcall BinkCopyToBufferHook(BINK* bnk, void* dest, int32_t destpitch,
                                       uint32_t destheight, uint32_t destx,
                                       uint32_t desty, uint32_t flags);
int32_t __stdcall BinkSetVolumeHook(BINK* bnk, int32_t unk1, int32_t volume);
int __fastcall mgsBinkSetPausedHook(MgsBink_t* pThis, void* EDX, char paused);

bool binkModInit() {
  if (config["patch"].count("fmv") != 1) {
    LanguageBarrierLog(
        "No FMV mods in patch configuration, not hooking Bink...");
    return true;
  }

  if (!createEnableApiHook(L"bink2w32", "_BinkSetVolume@12", BinkSetVolumeHook,
                           (LPVOID*)&BinkSetVolume) ||
      !createEnableApiHook(L"bink2w32", "_BinkOpen@8", BinkOpenHook,
                           (LPVOID*)&BinkOpen) ||
      !createEnableApiHook(L"bink2w32", "_BinkClose@4", BinkCloseHook,
                           (LPVOID*)&BinkClose) ||
      !createEnableApiHook(L"bink2w32", "_BinkCopyToBuffer@28",
                           BinkCopyToBufferHook, (LPVOID*)&BinkCopyToBuffer) ||
      !scanCreateEnableHook(
          "game", "mgsBinkSetPaused", (uintptr_t*)&gameExeMgsBinkSetPaused,
          (LPVOID)&mgsBinkSetPausedHook, (LPVOID*)&gameExeMgsBinkSetPausedReal))
    return false;

  if (config["patch"]["fmv"].count("fonts") == 1) {
    for (auto font : config["patch"]["fmv"]["fonts"]) {
      std::stringstream ss;
      ss << "languagebarrier\\subs\\fonts\\" << font.get<std::string>();
      std::string path = ss.str();
      AddFontResourceExA(path.c_str(), FR_PRIVATE, NULL);
    }
  }

  return true;
}

BINK* __stdcall BinkOpenHook(const char* name, uint32_t flags) {
  BINK* bnk = BinkOpen(name, flags);

  BinkModState_t* state = (BinkModState_t*)calloc(1, sizeof(BinkModState_t));
  stateMap.emplace(bnk, state);

  const char* tmp = name;
  if (strrchr(tmp, '\\')) tmp = strrchr(tmp, '\\') + 1;
  if (strrchr(tmp, '/')) tmp = strrchr(tmp, '/') + 1;

  if (config["patch"]["fmv"].count("audioRedirection") == 1 &&
      config["patch"]["fmv"]["audioRedirection"].count(tmp) == 1) {
    // TODO: temporarily set BGM volume to match movie volume, then revert in
    // BinkCloseHook
    // TODO: support using audio from 720p Bink videos in 1080p
    // ...meh, if the music's fine who cares
    uint32_t bgmId =
        config["patch"]["fmv"]["audioRedirection"][tmp].get<uint32_t>();
    gameSetBgm(bgmId, false);
    // we'll disable Bink audio in BinkSetVolumeHook. If we tried to do it here,
    // the game would just override it. If we tried to use BinkSetSoundOnOff,
    // the video wouldn't show (maybe the game thinks there's been an error).
    state->bgmId = bgmId;
  } else
    state->bgmId = 0;

  std::string subFileName;
  // TODO: support more than one track?
  // note: case sensitive
  if (config["patch"]["fmv"].count("subs") == 1 &&
      config["patch"]["fmv"]["subs"].count(tmp) == 1)
    subFileName = config["patch"]["fmv"]["subs"][tmp].get<std::string>();

  if (!subFileName.empty()) {
    std::stringstream ssSubPath;
    ssSubPath << "languagebarrier\\subs\\" << subFileName;
    std::string subPath = ssSubPath.str();
    std::stringstream logstr;
    logstr << "Using sub track " << subPath << " if available.";
    LanguageBarrierLog(logstr.str());

    // tried csri_open_file(), didn't work, not sure why. Not like it's a big
    // deal, anyway.
    std::ifstream in(subPath, std::ios::in | std::ios::binary);
    if (in.good()) {
      in.seekg(0, std::ios::end);
      std::string sub(in.tellg(), 0);
      in.seekg(0, std::ios::beg);
      in.read(&sub[0], sub.size());

      state->csri =
          csri_open_mem(csri_renderer_default(), &sub[0], sub.size(), NULL);
    }
    in.close();
  }

  return bnk;
}

void __stdcall BinkCloseHook(BINK* bnk) {
  BinkClose(bnk);
  if (stateMap.count(bnk) == 0) return;

  BinkModState_t* state = stateMap[bnk];
  if (state->framebuffer) {
    SimdFree(state->framebuffer);
  }
  if (state->csri) {
    csri_close(state->csri);
  }
  if (state->bgmId) {
    // Not cargoculting: the game doesn't always set a new BGM after playing a
    // video
    gameSetBgm(BGM_CLEAR, true);
    gameSetBgmShouldPlay(false);
  }
  free(state);
  stateMap.erase(bnk);
}

int32_t __stdcall BinkCopyToBufferHook(BINK* bnk, void* dest, int32_t destpitch,
                                       uint32_t destheight, uint32_t destx,
                                       uint32_t desty, uint32_t flags) {
  if (stateMap.count(bnk) == 0)
    return BinkCopyToBuffer(bnk, dest, destpitch, destheight, destx, desty,
                            flags);
  BinkModState_t* state = stateMap[bnk];

  if (state->csri == NULL)
    return BinkCopyToBuffer(bnk, dest, destpitch, destheight, destx, desty,
                            flags);

  uint32_t destwidth = destpitch / 4;
  double time = ((double)bnk->FrameRateDiv * (double)bnk->FrameNum) /
                (double)bnk->FrameRate;
  size_t align = SimdAlignment();

  if (bnk->FrameNum == state->lastFrameNum && destheight == state->destheight &&
      destwidth == state->destwidth) {
    SimdCopy((uint8_t*)state->framebuffer, destpitch, destwidth, destheight, 4,
             (uint8_t*)dest, destpitch);
    return 0;
  }
  if (state->destheight != destheight || state->destwidth != destwidth) {
    state->destheight = destheight;
    state->destwidth = destwidth;
    if (state->framebuffer != NULL) {
      SimdFree(state->framebuffer);
    }
    state->framebuffer =
        SimdAllocate(SimdAlign(destpitch * destheight, align), align);
  }

  state->lastFrameNum = bnk->FrameNum;

  // TODO: actually use destx and desty (figure out if Bink clips output or
  // expects destpitch/destheight to match)
  int32_t retval = BinkCopyToBuffer(bnk, state->framebuffer, destpitch,
                                    destheight, destx, desty, flags);

  csri_frame frame;
  frame.planes[0] = (uint8_t*)state->framebuffer;
  frame.strides[0] = destpitch;
  frame.pixfmt = CSRI_F_BGR_;
  csri_fmt format = {frame.pixfmt, destwidth, destheight};
  if (csri_request_fmt(state->csri, &format) == 0)
    csri_render(state->csri, &frame, time);

  // xy-VSFilter apparently doesn't support drawing onto BGRA directly and will
  // set the alpha of everything it touches to 0. So let's just pretend
  // everything's opaque and set it to FF. (Note it does alpha-blend onto the
  // BGR32 background though). We could save video alpha and reapply it, but we
  // don't need that for now since all our videos are 100% opaque.
  size_t i, imax;
  for (i = 0, imax = destwidth * destheight; i < imax; i += 4) {
    __m128i* vec = (__m128i*)((uint32_t*)state->framebuffer + i);
    *vec = _mm_or_si128(*vec, MaskFF000000);
  }
  for (; i < imax; i++) {
    ((uint32_t*)state->framebuffer)[i] |= 0xFF000000;
  }

  SimdCopy((uint8_t*)state->framebuffer, destpitch, destpitch / 4, destheight,
           4, (uint8_t*)dest, destpitch);

  return retval;
}

int32_t __stdcall BinkSetVolumeHook(BINK* bnk, int32_t unk1, int32_t volume) {
  if (stateMap.count(bnk) == 0) return BinkSetVolume(bnk, unk1, volume);
  BinkModState_t* state = stateMap[bnk];
  if (state->bgmId) return BinkSetVolume(bnk, unk1, 0);
  return BinkSetVolume(bnk, unk1, volume);
}

int __fastcall mgsBinkSetPausedHook(MgsBink_t* pThis, void* EDX, char paused) {
  if (stateMap.count(pThis->pBink) != 0) {
    BinkModState_t* state = stateMap[pThis->pBink];
    if (state->bgmId != 0) {
      // gameSetBgmPaused(paused);
      // unfortunately we can't pause BGM without desyncing right now
      // so let's just allow the video to run in the background
      return 0;
    }
  }
  return gameExeMgsBinkSetPausedReal(pThis, paused);
}
}