#include "LanguageBarrier.h"
#include <Simd/SimdLib.h>
#include <ass/ass.h>
#include <unordered_map>
#include "BinkMod.h"
#include "Config.h"
#include "Game.h"

// warning: this creates messageboxes with loglevel verbose
// have fun watching videos with this on
//#define BINKMODDEBUG

static ASS_Library* AssHandler;

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
  ASS_Renderer* AssRenderer;
  ASS_Track* AssTrack;
  bool renderedSinceLastInit;
  uint32_t bgmId;
  void* framebuffer;
  uint32_t destwidth;
  uint32_t destheight;
  uint32_t lastFrameNum;
} BinkModState_t;

static std::unordered_map<BINK*, BinkModState_t*> stateMap;

namespace lb {
BINK* __stdcall BinkOpenHook(const char* name, uint32_t flags);
void __stdcall BinkCloseHook(BINK* bnk);
int32_t __stdcall BinkCopyToBufferHook(BINK* bnk, void* dest, int32_t destpitch,
                                       uint32_t destheight, uint32_t destx,
                                       uint32_t desty, uint32_t flags);
int32_t __stdcall BinkSetVolumeHook(BINK* bnk, int32_t unk1, int32_t volume);

#ifdef BINKMODDEBUG
void msg_callback(int level, const char* fmt, va_list va, void* data);
#endif
bool initLibass();
bool initLibassRenderer(BinkModState_t* state);
void closeLibassRenderer(BinkModState_t* state);

#ifdef BINKMODDEBUG
void msg_callback(int level, const char* fmt, va_list va, void* data) {
  if (level > 6) return;
  char buffer[500];
  vsprintf_s(buffer, 500, fmt, va);
  MessageBoxA(NULL, buffer, "LanguageBarrier", MB_OK);
}
#endif

bool initLibass() {
  if (AssHandler) return true;

  AssHandler = ass_library_init();
  if (!AssHandler) {
    LanguageBarrierLog("ass_library_init() failed!");
    return false;
  }

  ass_set_fonts_dir(AssHandler, "languagebarrier\\subs\\fonts");
#ifdef BINKMODDEBUG
  ass_set_message_cb(AssHandler, msg_callback, NULL);
#endif

  return true;
}

bool binkModInit() {
  if (!createEnableApiHook(L"bink2w32", "_BinkSetVolume@12", BinkSetVolumeHook,
                           (LPVOID*)&BinkSetVolume) ||
      !createEnableApiHook(L"bink2w32", "_BinkOpen@8", BinkOpenHook,
                           (LPVOID*)&BinkOpen) ||
      !createEnableApiHook(L"bink2w32", "_BinkClose@4", BinkCloseHook,
                           (LPVOID*)&BinkClose) ||
      !createEnableApiHook(L"bink2w32", "_BinkCopyToBuffer@28",
                           BinkCopyToBufferHook, (LPVOID*)&BinkCopyToBuffer) ||
      !initLibass())
    return false;

  return true;
}

bool initLibassRenderer(BinkModState_t* state) {
  if (state->AssRenderer) return true;

  state->AssRenderer = ass_renderer_init(AssHandler);
  if (!(state->AssRenderer)) {
    LanguageBarrierLog("ass_renderer_init failed!");
    return false;
  }

  ass_set_hinting(state->AssRenderer, ASS_HINTING_LIGHT);

  // this kills performance a little too hard
  // ass_set_cache_limits(state->AssRenderer, 2000, 60);

  ass_set_fonts(state->AssRenderer, NULL, "sans-serif",
                ASS_FONTPROVIDER_AUTODETECT, NULL, 1);

  state->renderedSinceLastInit = false;

  return true;
}

void closeLibassRenderer(BinkModState_t* state) {
  if (state->AssRenderer) {
    ass_renderer_done(state->AssRenderer);
    state->AssRenderer = NULL;
  }
}

BINK* __stdcall BinkOpenHook(const char* name, uint32_t flags) {
  BINK* bnk = BinkOpen(name, flags);

  if (!initLibass()) return bnk;
  BinkModState_t* state = (BinkModState_t*)calloc(1, sizeof(BinkModState_t));
  stateMap.emplace(bnk, state);

  const char* tmp = name;
  if (strrchr(tmp, '\\')) tmp = strrchr(tmp, '\\') + 1;
  if (strrchr(tmp, '/')) tmp = strrchr(tmp, '/') + 1;

  if (Config::config().j["fmv"]["useHqAudio"].get<bool>() == true &&
      Config::fmv().j["hqAudio"].count(tmp) == 1) {
    // TODO: temporarily set BGM volume to match movie volume, then revert in
    // BinkCloseHook
    // TODO: support using audio from 720p Bink videos in 1080p
    // ...meh, if the music's fine who cares
    uint32_t bgmId = Config::fmv().j["hqAudio"][tmp].get<uint32_t>();
    gameSetBgm(bgmId);
    // we'll disable Bink audio in BinkSetVolumeHook. If we tried to do it here,
    // the game would just override it. If we tried to use BinkSetSoundOnOff,
    // the video wouldn't show (maybe the game thinks there's been an error).
    state->bgmId = bgmId;
  } else
    state->bgmId = 0;

  if (!initLibassRenderer(state)) return bnk;

  std::string subFileName;
  // TODO: support more than one track?
  // note: case sensitive
  if (Config::config().j["fmv"]["enableJpVideoSubs"].get<bool>() == true &&
      Config::fmv().j["subs"]["jpVideo"].count(tmp) == 1)
    subFileName = Config::fmv().j["subs"]["jpVideo"][tmp].get<std::string>();
  if (Config::config().j["fmv"]["enableKaraokeSubs"].get<bool>() == true &&
      Config::fmv().j["subs"]["karaoke"].count(tmp) == 1)
    subFileName = Config::fmv().j["subs"]["karaoke"][tmp].get<std::string>();

  if (!subFileName.empty()) {
    std::stringstream ssSubPath;
    ssSubPath << "languagebarrier\\subs\\" << subFileName;
    std::string subPath = ssSubPath.str();
    std::stringstream logstr;
    logstr << "Using sub track " << subPath << " if available.";
    LanguageBarrierLog(logstr.str());
    char* cSubPath = &subPath[0];
    state->AssTrack = ass_read_file(AssHandler, cSubPath, "UTF-8");
  }

  return bnk;
}

void __stdcall BinkCloseHook(BINK* bnk) {
  BinkClose(bnk);
  if (stateMap.count(bnk) == 0) return;

  BinkModState_t* state = stateMap[bnk];
  if (state->AssRenderer) {
    closeLibassRenderer(state);
  }
  if (state->AssTrack) {
    ass_free_track(state->AssTrack);
  }
  if (state->framebuffer) {
    SimdFree(state->framebuffer);
  }
  if (state->bgmId) {
    // Not cargoculting: the game doesn't always set a new BGM after playing a
    // video
    gameSetBgm(BGM_CLEAR);
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

  if (!(state->AssRenderer) || !(state->AssTrack))
    return BinkCopyToBuffer(bnk, dest, destpitch, destheight, destx, desty,
                            flags);

  uint32_t destwidth = destpitch / 4;
  ass_set_frame_size(state->AssRenderer, destwidth, destheight);
  // Not entirely sure whether FrameNum is correct here or off by one, but it's
  // pedantic anyway
  uint32_t curTime = (uint32_t)(
      bnk->FrameNum *
      (1000.0 / ((double)bnk->FrameRate / (double)bnk->FrameRateDiv)));
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

  ASS_Image* subpict =
      ass_render_frame(state->AssRenderer, state->AssTrack, curTime, NULL);
  if (subpict) state->renderedSinceLastInit = true;
  // Libass likes caching a little too much, so for complex karaoke subs we
  // might end up with hundreds of megabytes of cached data. Quick hack is to
  // reinitialise the renderer during off times, keeping memory consumption down
  // without introducing serious hitching.
  // ...nevermind, this also makes shit too slow.
  /*if (!subpict && state->renderedSinceLastInit) {
    closeLibassRenderer(state);
    initLibassRenderer(state);
  }*/

  for (; subpict; subpict = subpict->next) {
    if (subpict->h == 0 || subpict->w == 0) continue;

    uint8_t* background = (uint8_t*)state->framebuffer + subpict->dst_x * 4 +
                          subpict->dst_y * destpitch;

    const uint8_t Rf = (subpict->color >> 24) & 0xff;
    const uint8_t Gf = (subpict->color >> 16) & 0xff;
    const uint8_t Bf = (subpict->color >> 8) & 0xff;
    const uint8_t Af = 255 - (subpict->color & 0xff);

    uint8_t* mask = (uint8_t*)SimdAllocate(
        SimdAlign(subpict->h * subpict->stride, align), align);
    for (int i = 0, imax = subpict->h * subpict->stride; i < imax; i++) {
      mask[i] = (subpict->bitmap[i] * Af) / 255;
    }

    // no, this shouldn't be subpict->stride, subpict->stride is for 1bpp
    size_t fgStride = subpict->w * 4;
    uint8_t* foreground =
        (uint8_t*)SimdAllocate(SimdAlign(subpict->h * fgStride, align), align);
    SimdFillBgra(foreground, fgStride, subpict->w, subpict->h, Bf, Gf, Rf, 255);
    SimdAlphaBlending(foreground, fgStride, subpict->w, subpict->h, 4, mask,
                      subpict->stride, background, destpitch);
    SimdFree(foreground);
    SimdFree(mask);
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
}