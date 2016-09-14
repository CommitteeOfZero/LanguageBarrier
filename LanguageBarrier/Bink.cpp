#include "LanguageBarrier.h"
#include <Simd/SimdLib.h>
#include <ass/ass.h>

static ASS_Library* AssHandler;
static ASS_Renderer* AssRenderer;
static ASS_Track* AssTrack;

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

typedef void*(__stdcall* BINKOPEN)(const char* name, uint32_t flags);
typedef void(__stdcall* BINKCLOSE)(BINK* bnk);
typedef int32_t(__stdcall* BINKCOPYTOBUFFER)(BINK* bnk, void* dest,
                                             int32_t destpitch,
                                             uint32_t destheight,
                                             uint32_t destx, uint32_t desty,
                                             uint32_t flags);
static BINKOPEN BinkOpen = NULL;
static BINKCLOSE BinkClose = NULL;
static BINKCOPYTOBUFFER BinkCopyToBuffer = NULL;

static bool renderedSinceLastInit = false;

#ifdef _DEBUG
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
    MessageBox(NULL, L"ass_library_init failed!", L"LanguageBarrier", MB_OK);
    return false;
  }

  ass_set_fonts_dir(AssHandler, "subs/fonts");
#ifdef _DEBUG
  ass_set_message_cb(AssHandler, msg_callback, NULL);
#endif

  return true;
}

bool initLibassRenderer() {
  if (AssRenderer) return true;

  AssRenderer = ass_renderer_init(AssHandler);
  if (!AssRenderer) {
    MessageBox(NULL, L"ass_renderer_init failed!", L"LanguageBarrier", MB_OK);
    return false;
  }

  ass_set_fonts(AssRenderer, NULL, "sans-serif", ASS_FONTPROVIDER_AUTODETECT,
                NULL, 1);

  renderedSinceLastInit = false;

  return true;
}

void closeLibassRenderer() {
  if (AssRenderer) {
    ass_renderer_done(AssRenderer);
    AssRenderer = NULL;
  }
}

void* __stdcall BinkOpenHook(const char* name, uint32_t flags) {
  if (!initLibass()) return BinkOpen(name, flags);
  if (AssTrack) {
    ass_free_track(AssTrack);
    AssTrack = NULL;
  }

  if (!initLibassRenderer()) return BinkOpen(name, flags);

  const char* tmp = name;
  if (strrchr(tmp, '\\')) tmp = strrchr(tmp, '\\') + 1;
  if (strrchr(tmp, '/')) tmp = strrchr(tmp, '/') + 1;
  size_t length = strlen(tmp) + strlen("subs/") + strlen(".ass") + 1;
  char* subName = (char*)calloc(length, 1);
  snprintf(subName, length, "subs/%s.ass", tmp);

  AssTrack = ass_read_file(AssHandler, subName, "UTF-8");
  free(subName);
  return BinkOpen(name, flags);
}

void __stdcall BinkCloseHook(BINK* bnk) {
  BinkClose(bnk);
  if (AssTrack) {
    ass_free_track(AssTrack);
    AssTrack = NULL;
  }
  closeLibassRenderer();
}

int32_t __stdcall BinkCopyToBufferHook(BINK* bnk, void* dest, int32_t destpitch,
                                       uint32_t destheight, uint32_t destx,
                                       uint32_t desty, uint32_t flags) {
  if (!AssTrack || !AssRenderer)
    return BinkCopyToBuffer(bnk, dest, destpitch, destheight, destx, desty,
                            flags);

  uint32_t destwidth = destpitch / 4;
  ass_set_frame_size(AssRenderer, destwidth, destheight);
  // Not entirely sure whether FrameNum is correct here or off by one, but it's
  // pedantic anyway
  uint32_t curTime = (uint32_t)(
      bnk->FrameNum *
      (1000.0 / ((double)bnk->FrameRate / (double)bnk->FrameRateDiv)));
  size_t align = SimdAlignment();

  // This way our writes are guaranteed to be aligned at least every 4 pixels
  uint8_t* frame =
      (uint8_t*)SimdAllocate(SimdAlign(destpitch * destheight, align), align);
  // TODO: actually use destx and desty (figure out if Bink clips output or
  // expects destpitch/destheight to match)
  int32_t retval =
      BinkCopyToBuffer(bnk, frame, destpitch, destheight, destx, desty, flags);

  ASS_Image* subpict = ass_render_frame(AssRenderer, AssTrack, curTime, NULL);
  if (subpict) renderedSinceLastInit = true;
  // Libass likes caching a little too much, so for complex karaoke subs we
  // might end up with hundreds of megabytes of cached data. Quick hack is to
  // reinitialise the renderer during off times, keeping memory consumption down
  // without introducing serious hitching.
  if (!subpict && renderedSinceLastInit) {
    closeLibassRenderer();
    initLibassRenderer();
  }

  for (; subpict; subpict = subpict->next) {
    if (subpict->h == 0 || subpict->w == 0) continue;

    uint8_t* background =
        frame + subpict->dst_x * 4 + subpict->dst_y * destpitch;

    const uint8_t Rf = (subpict->color >> 24) & 0xff;
    const uint8_t Gf = (subpict->color >> 16) & 0xff;
    const uint8_t Bf = (subpict->color >> 8) & 0xff;
    const uint8_t Af = 255 - (subpict->color & 0xff);

    // no, this shouldn't be subpict->stride, subpict->stride is for 1bpp
    size_t fgStride = subpict->w * 4;
    uint8_t* foreground =
        (uint8_t*)SimdAllocate(SimdAlign(subpict->h * fgStride, align), align);
    SimdFillBgra(foreground, fgStride, subpict->w, subpict->h, Bf, Gf, Rf, Af);
    SimdAlphaBlending(foreground, fgStride, subpict->w, subpict->h, 4,
                      subpict->bitmap, subpict->stride, background, destpitch);
    SimdFree(foreground);
  }

  SimdCopy(frame, destpitch, destpitch / 4, destheight, 4, (uint8_t*)dest,
           destpitch);
  SimdFree(frame);

  return retval;
}
