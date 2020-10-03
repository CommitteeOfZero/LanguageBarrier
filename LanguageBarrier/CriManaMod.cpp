#include "CriManaMod.h"
#include <csri/csri.h>
#include <stb_vorbis.h>
#include <emmintrin.h>
#include <fstream>
#include <unordered_map>
#include "Config.h"
#include "Game.h"
#include "LanguageBarrier.h"
#include "SigScan.h"
#include "DirectXTex.h"

#define FLOATING_POINT
#define OUTSIDE_SPEEX
#define RANDOM_PREFIX lb_speexdsp_
#include "contrib/speexdsp/speex_resampler.h"

typedef struct {
  uint8_t* imageptr;
  uint32_t bufsize;
  uint32_t line_pitch;
  uint32_t line_size;
  uint32_t num_lines;
} CriManaImageBufferInfo;

typedef struct {
  int32_t frame_no;
  int32_t frame_no_per_file;
  uint32_t width;
  uint32_t height;
  uint32_t disp_width;
  uint32_t disp_height;
  uint32_t framerate;
  uint32_t framerate_n;
  uint32_t framerate_d;
  uint32_t total_frames_per_file;
  uint64_t time;
  uint64_t time_per_file;
  uint64_t tunit;
  uint32_t cnt_concatenated_movie;
  int32_t num_images;
  CriManaImageBufferInfo image_info[4];
  int32_t csc_flag;
  uint32_t alpha_type;
  uint32_t ref_result;
  void* details_ptr[2];
  uint32_t color_conv;
  uint32_t cnt_skipped_frames;
  uint32_t reserved[1];
} CriManaFrameInfo;

typedef struct {
  signed int field_0;
  char gap_4[4];
  char initialized;
  char gap_9[2];
  char field_B;
  unsigned __int8 pauseFlag;
  char gap_D[5];
  unsigned __int8 loopFlag;
  __declspec(align(2)) char gap_14[4];
  float volume;
  char gap_1C[16];
  int field_2C;
  int field_30;
  char gap_34[4];
  int field_38;
  int field_3C;
  int field_40;
  int field_44;
  char gap_48[40];
  int movieIdInCpk;
  char gap_74[4];
  char moviePath[256];
  void* movieDataPtr;
  char gap_17C[4];
  int movieDataSize;
  char gap_184[24];
  mgsVFSObject* mgsFilePtr;
  char gap_1A0[2728];
  int field_C48;
  char gap_C4C[4];
  int field_C50;
  char gap_C54[124];
  int field_CD0;
  void* criManaPlayer;
  int criManaPlayerStatus;
  char gap_CDC[4];
  CriManaFrameInfo criManaFrameInfo;
  char gap_D9C[3020];
} MgsMoviePlayerObj_t;

typedef struct {
  bool playing = false;
  uint32_t lastFrameNum = 0;
  bool keepLastFrame = false;
  double time = 0;
  csri_inst* csri = NULL;
} CriManaModState_t;

static std::unordered_map<MgsMoviePlayerObj_t*, CriManaModState_t*> stateMap;

static __m128i MaskFF000000 = _mm_set1_epi32(0xFF000000);
uint32_t RENDER_TARGET_SURF_ID = 199;
uint32_t SUBS_LAYER_SURF_ID = 312;

typedef int(__thiscall* MgsMovieCPlayerPlayProc)(MgsMoviePlayerObj_t* pThis, int a2, int a3,
                                                 char* movieFileName);
static MgsMovieCPlayerPlayProc gameExeMgsMovieCPlayerPlay = NULL;
static MgsMovieCPlayerPlayProc gameExeMgsMovieCPlayerPlayReal = NULL;

typedef int(__thiscall* MgsMovieCPlayerStopProc)(MgsMoviePlayerObj_t* pThis);
static MgsMovieCPlayerStopProc gameExeMgsMovieCPlayerStop = NULL;
static MgsMovieCPlayerStopProc gameExeMgsMovieCPlayerStopReal = NULL;

typedef int(__thiscall* MgsMovieCPlayerRenderProc)(MgsMoviePlayerObj_t* pThis);
static MgsMovieCPlayerRenderProc gameExeMgsMovieCPlayerRender = NULL;
static MgsMovieCPlayerRenderProc gameExeMgsMovieCPlayerRenderReal = NULL;

typedef int (__cdecl* GslCreateSurfProc)(int id, int width, int height, int format);
static GslCreateSurfProc gameExeGslCreateSurface = NULL;
static GslCreateSurfProc gameExeGslCreateSurfaceReal = NULL;

typedef int (__cdecl* GslDrawSpriteProc)(int textureId, float spriteX,
                                         float spriteY, float spriteWidth,
                                         float spriteHeight, float displayX,
                                         float displayY, int color, int opacity,
                                         int shaderId);
static GslDrawSpriteProc gameExeGslDrawSprite = NULL;
static GslDrawSpriteProc gameExeGslDrawSpriteReal = NULL;

typedef int(__cdecl* DrawMovieFrameProc)(int tint, int opacity);
static DrawMovieFrameProc gameExeDrawMovieFrame = NULL;
static DrawMovieFrameProc gameExeDrawMovieFrameReal = NULL;

typedef int(__cdecl* gslFillHookProc)(int id, int a1, int a2, int a3, int a4, int r, int g, int b, int a);
static gslFillHookProc gameExegslFill = NULL;
static gslFillHookProc gameExegslFillReal = NULL;

namespace lb {
int __cdecl gslCreateSurfaceHook(int id, int width, int height, int format);
int __cdecl gslDrawSpriteHook(int textureId, float spriteX, float spriteY,
                      float spriteWidth, float spriteHeight, float displayX,
                      float displayY, int color, int opacity, int shaderId);
int __fastcall mgsMovieCPlayerPlayHook(MgsMoviePlayerObj_t* pThis, void* dummy, int a2,
                                       int a3, char* movieFileName);
int __fastcall mgsMovieCPlayerStopHook(MgsMoviePlayerObj_t* pThis);
int __fastcall mgsMovieCPlayerRenderHook(MgsMoviePlayerObj_t* pThis);
int __cdecl drawMovieFrameHook(int tint, int opacity);
int __cdecl gslFillHook(int id, int a1, int a2, int a3, int a4, int r, int g, int b, int a);

bool criManaModInit() {
  if (config["patch"].count("fmv") != 1) {
    LanguageBarrierLog(
        "No FMV mods in patch configuration, not hooking CriMana...");
    return true;
  }

  if (!scanCreateEnableHook("game", "mgsMovieCPlayerPlay", (uintptr_t*)&gameExeMgsMovieCPlayerPlay,
                            (LPVOID)&mgsMovieCPlayerPlayHook,
                            (LPVOID*)&gameExeMgsMovieCPlayerPlayReal) ||
      !scanCreateEnableHook("game", "mgsMovieCPlayerStop", (uintptr_t*)&gameExeMgsMovieCPlayerStop,
                            (LPVOID)&mgsMovieCPlayerStopHook,
                            (LPVOID*)&gameExeMgsMovieCPlayerStopReal) ||
      !scanCreateEnableHook("game", "mgsMovieCPlayerRender", (uintptr_t*)&gameExeMgsMovieCPlayerRender,
                            (LPVOID)&mgsMovieCPlayerRenderHook,
                            (LPVOID*)&gameExeMgsMovieCPlayerRenderReal) ||
      !scanCreateEnableHook("game", "gslCreateSurface", (uintptr_t*)&gameExeGslCreateSurface,
                            (LPVOID)&gslCreateSurfaceHook,
                            (LPVOID*)&gameExeGslCreateSurfaceReal) ||
      !scanCreateEnableHook("game", "gslDrawSprite", (uintptr_t*)&gameExeGslDrawSprite,
                            (LPVOID)&gslDrawSpriteHook,
                            (LPVOID*)&gameExeGslDrawSpriteReal) ||
      !scanCreateEnableHook("game", "drawMovieFrame", (uintptr_t*)&gameExeDrawMovieFrame,
                            (LPVOID)&drawMovieFrameHook,
                            (LPVOID*)&gameExeDrawMovieFrameReal) ||
      !scanCreateEnableHook("game", "gslFill", (uintptr_t*)&gameExegslFill,
                            (LPVOID)&gslFillHook,
                            (LPVOID*)&gameExegslFillReal))
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

int __cdecl drawMovieFrameHook(int tint, int opacity) {
  int ret = gameExeDrawMovieFrameReal(tint, opacity);

  for (const auto& kv : stateMap) {
    if (kv.second->playing) {
      if (!kv.second->keepLastFrame) {      
        DirectX::ScratchImage image;
        HRESULT hr = CaptureTexture(gameExePMgsD3D11State->pid3d11deviceC, gameExePMgsD3D11State->pid3d11devicecontext18, surfaceArray[RENDER_TARGET_SURF_ID].texPtr[0], image);
        
        if (surfaceArray[SUBS_LAYER_SURF_ID].texPtr[0]) {
            surfaceArray[SUBS_LAYER_SURF_ID].shaderRscView->Release();
            surfaceArray[SUBS_LAYER_SURF_ID].texPtr[0]->Release();
        }
        
        auto state = stateMap.begin()->second;
        uint8_t* imagePtr = image.GetPixels();
        csri_frame frame;
        frame.planes[0] = imagePtr;
        frame.strides[0] = image.GetImages()[0].rowPitch;
        frame.pixfmt = CSRI_F_BGR_;
        csri_fmt format = { frame.pixfmt, image.GetMetadata().width, image.GetMetadata().height };
        if (csri_request_fmt(state->csri, &format) == 0) {
            csri_render(state->csri, &frame, state->time);
        }
        
        // xy-VSFilter apparently doesn't support drawing onto BGRA directly and will
        // set the alpha of everything it touches to 0. So let's just pretend
        // everything's opaque and set it to FF. (Note it does alpha-blend onto the
        // BGR32 background though). We could save video alpha and reapply it, but we
        // don't need that for now since all our videos are 100% opaque.
        size_t i, imax;
        for (i = 0, imax = image.GetMetadata().width * image.GetMetadata().height; i < imax; i += 4) {
            __m128i* vec = (__m128i*)((uint32_t*)image.GetPixels() + i);
            *vec = _mm_or_si128(*vec, MaskFF000000);
        }
        for (; i < imax; i++) {
            ((uint32_t*)imagePtr)[i] |= 0xFF000000;
        }
        
        D3D11_SHADER_RESOURCE_VIEW_DESC rscViewDesc;
        memset(&rscViewDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
        auto result = DirectX::CreateTexture(gameExePMgsD3D11State->pid3d11deviceC, image.GetImages(),
            image.GetImageCount(), image.GetMetadata(),
            (ID3D11Resource**)&surfaceArray[SUBS_LAYER_SURF_ID].texPtr[0]);
        rscViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        rscViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        rscViewDesc.Buffer.NumElements = 1;
        rscViewDesc.Texture2D.MipLevels = image.GetMetadata().mipLevels;
        result = gameExePMgsD3D11State->pid3d11deviceC->
            CreateShaderResourceView(surfaceArray[SUBS_LAYER_SURF_ID].texPtr[0],
                &rscViewDesc, &surfaceArray[SUBS_LAYER_SURF_ID].shaderRscView);
      }

      gslDrawSpriteHook(SUBS_LAYER_SURF_ID, 0.0f, 0.0f, 1920.0f, 1080.0f, 0.0f, 0.0f, 0xFFFFFF, 256, 1);
    }
  }

  return ret;
}

int gslCreateSurfaceHook(int id, int width, int height, int format) {
  return gameExeGslCreateSurfaceReal(id, width, height, format);
}

int gslDrawSpriteHook(int textureId, float spriteX, float spriteY,
                      float spriteWidth, float spriteHeight, float displayX,
                      float displayY, int color, int opacity, int shaderId) {
  return gameExeGslDrawSpriteReal(textureId, spriteX, spriteY, spriteWidth,
                                  spriteHeight, displayX, displayY, color, opacity,
                                  shaderId);
}

int __cdecl gslFillHook(int id, int a1, int a2, int a3, int a4, int r, int g, int b, int a) {
  return gameExegslFillReal(id, a1, a2, a3, a4, r, g, b, a);
}

int __fastcall mgsMovieCPlayerPlayHook(MgsMoviePlayerObj_t* pThis, void* dummy, int a2,
                                       int a3, char* movieFileName) {
  std::string subFileName;
  // note: case sensitive
  if (config["patch"]["fmv"].count("subs") == 1 &&
      config["patch"]["fmv"]["subs"].count(movieFileName) == 1)
    subFileName = config["patch"]["fmv"]["subs"][movieFileName].get<std::string>();

  if (!subFileName.empty()) {
    std::stringstream ssSubPath;
    ssSubPath << "languagebarrier\\subs\\" << subFileName;
    std::string subPath = ssSubPath.str();
    std::stringstream logstr;
    logstr << "Using sub track " << subPath << " if available.";
    LanguageBarrierLog(logstr.str());

    std::ifstream in(subPath, std::ios::in | std::ios::binary);
    if (in.good()) {
      in.seekg(0, std::ios::end);
      std::string sub(in.tellg(), 0);
      in.seekg(0, std::ios::beg);
      in.read(&sub[0], sub.size());

      CriManaModState_t* state = new CriManaModState_t;
      stateMap.emplace(pThis, state);

      gslCreateSurfaceHook(SUBS_LAYER_SURF_ID, 1920, 1080, 32);
      gslFillHook(SUBS_LAYER_SURF_ID, 0, 0, 0, 0, 0, 0, 0, 255);

      state->csri =
          csri_open_mem(csri_renderer_default(), &sub[0], sub.size(), NULL);
      state->playing = true;
    }
    in.close();
  }

  return gameExeMgsMovieCPlayerPlayReal(pThis, a2, a3, movieFileName);
}

int __fastcall mgsMovieCPlayerStopHook(MgsMoviePlayerObj_t* pThis) {
  if (stateMap.count(pThis) == 0) return gameExeMgsMovieCPlayerStopReal(pThis);
  
  CriManaModState_t* state = stateMap[pThis];

  if (state->csri) {
      csri_close(state->csri);
  }
  delete state;
  stateMap.erase(pThis);

  return gameExeMgsMovieCPlayerStopReal(pThis);
}

int __fastcall mgsMovieCPlayerRenderHook(MgsMoviePlayerObj_t* pThis) {
  if (stateMap.count(pThis) == 0)
    return gameExeMgsMovieCPlayerRenderReal(pThis);

  CriManaModState_t* state = stateMap[pThis];
  CriManaFrameInfo* frameInfo = &pThis->criManaFrameInfo;

  if (state->csri == NULL)
    return gameExeMgsMovieCPlayerRenderReal(pThis);

  double time = ((double)frameInfo->framerate_d * (double)frameInfo->frame_no) /
                 (double)frameInfo->framerate_n;

  state->keepLastFrame = frameInfo->frame_no == state->lastFrameNum;

  state->lastFrameNum = frameInfo->frame_no;
  state->time = time;

  return gameExeMgsMovieCPlayerRenderReal(pThis);
}
}  // namespace lb