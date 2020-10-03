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

static CSurface* gameExeCSurfaceObjects = NULL;

typedef unsigned char CriUint8;
typedef signed char CriSint8;
typedef unsigned short CriUint16;
typedef signed short CriSint16;
typedef unsigned long CriUint32;
typedef signed long CriSint32;
typedef unsigned __int64 CriUint64;
typedef signed __int64 CriSint64;
typedef struct {
    CriUint64	h;						
    CriUint64	l;						
} CriUint128;
typedef struct {
    CriSint64	h;						
    CriUint64	l;						
} CriSint128;
typedef signed short CriFloat16;
typedef float CriFloat32;
typedef double CriFloat64;
typedef signed long	CriFixed32;
typedef CriSint32 CriBool;
typedef char CriChar8;

typedef enum {
  CRIMANA_AUDIO_CODEC_UNKNOWN = 0,
  CRIMANA_AUDIO_CODEC_ADX = 2,
  CRIMANA_AUDIO_CODEC_HCA = 4,
  CRIMANA_AUDIO_CODEC_ENUM_SIZE_IS_4BYTES = 0x7FFFFFFF
} CriManaAudioCodecType;

typedef enum {
  CRIMANA_COMPO_OPAQ = 0,
  CRIMANA_COMPO_ALPHFULL = 1,
  CRIMANA_COMPO_ALPH3STEP = 2,
  CRIMANA_COMPO_ALPH32BIT = 3,
  CRIMANA_COMPO_ALPH1BIT = 4,
  CRIMANA_COMPO_ALPH2BIT = 5,
  CRIMANA_COMPO_ALPH3BIT = 6,
  CRIMANA_COMPO_ALPH4BIT = 7,
  CRIMANA_COMPO_ENUM_SIZE_IS_4BYTES = 0x7FFFFFFF
} CriManaAlphaType;

typedef enum {
  CRIMANA_REFER_RESULT_OK = 0,
  CRIMANA_REFER_RESULT_SHORT_INPUT = 1,
  CRIMANA_REFER_RESULT_SHORT_CPUTIME = 2,
  CRIMANA_REFER_RESULT_NO_MORE_KEEP = 3,
  CRIMANA_REFER_RESULT_ENUM_SIZE_IS_4BYTES = 0x7FFFFFFF
} CriManaReferFrameResult;

typedef enum {
  CRIMANA_COLORSPACE_CONVERSION_TYPE_ITU_R_BT601_LIMITED = 0,
  CRIMANA_COLORSPACE_CONVERSION_TYPE_ITU_R_BT601_FULLRANGE = 1,
  CRIMANA_COLORSPACE_CONVERSION_TYPE_ENUM_SIZE_IS_4BYTES = 0x7FFFFFFF
} CriManaColorSpaceConversionType;

typedef enum {
  CRIMANA_VIDEO_CODEC_UNKNOWN = 0,
  CRIMANA_VIDEO_CODEC_SOFDEC_PRIME = 1,
  CRIMANA_VIDEO_CODEC_H264 = 5,
  CRIMANA_VIDEO_CODEC_VP9 = 9,
  CRIMANA_VIDEO_CODEC_ENUM_SIZE_IS_4BYTES = 0x7FFFFFFF
} CriManaVideoCodecType;

typedef struct {
  CriUint8* imageptr;
  CriUint32 bufsize;
  CriUint32 line_pitch;
  CriUint32 line_size;
  CriUint32 num_lines;
} CriManaImageBufferInfo;

int piece_of_crap = 1280;

typedef struct {
  CriSint32 frame_no;
  CriSint32 frame_no_per_file;
  CriUint32 width;
  CriUint32 height;
  CriUint32 disp_width;
  CriUint32 disp_height;
  CriUint32 framerate;
  CriUint32 framerate_n;
  CriUint32 framerate_d;
  CriUint32 total_frames_per_file;
  CriUint64 time;
  CriUint64 time_per_file;
  CriUint64 tunit;
  CriUint32 cnt_concatenated_movie;
  CriSint32 num_images;
  CriManaImageBufferInfo image_info[4];
  CriBool csc_flag;
  CriManaAlphaType alpha_type;
  CriManaReferFrameResult ref_result;
  void* details_ptr[2];
  CriManaColorSpaceConversionType color_conv;
  CriUint32 cnt_skipped_frames;
  CriUint32 reserved[1];
} CriManaFrameInfo;

typedef struct {
  CriUint32 width;
  CriUint32 height;
  CriUint32 disp_width;
  CriUint32 disp_height;
  CriUint32 framerate;
  CriUint32 framerate_n;
  CriUint32 framerate_d;
  CriUint32 total_frames;
  CriUint32 material_width;
  CriUint32 material_height;
  CriManaVideoCodecType codec_type;
  CriManaColorSpaceConversionType color_conv;
  CriSint32 max_picture_size;
  CriSint32 average_bitrate;
} CriManaVideoInfo;

typedef struct {
  CriUint32 width;
  CriUint32 height;
  CriUint32 disp_width;
  CriUint32 disp_height;
  CriUint32 framerate;
  CriUint32 framerate_n;
  CriUint32 framerate_d;
  CriUint32 total_frames;
  CriManaAlphaType alpha_type;
  CriManaVideoCodecType codec_type;
  CriManaColorSpaceConversionType color_conv;
  CriSint32 max_picture_size;
  CriSint32 average_bitrate;
} CriManaAlphaInfo;

typedef struct {
  CriManaAudioCodecType codec_type;
  CriUint32 sampling_rate;
  CriUint32 num_channels;
  CriUint32 total_samples;
  CriUint32 output_buffer_samples;
  CriUint8 ambisonics;
} CriManaAudioInfo;

typedef struct {
  CriUint32 is_playable;
  CriUint32 average_bitrate;
  CriUint32 max_chunk_size;
  CriUint32 min_buffer_size;
  CriUint32 num_video_streams;
  CriManaVideoInfo video_prm[1];
  CriUint32 num_audio_streams;
  CriManaAudioInfo audio_prm[32];
  CriUint32 num_subtitle_channels;
  CriUint32 max_subtitle_size;
  CriUint32 num_alpha_streams;
  CriManaAlphaInfo alpha_prm[1];
  CriBool seekinfo_flag;
  CriUint32 format_ver;
} CriManaMovieInfo;

typedef struct {
  char gap_0[16];
  int field_10;
  char gap_14[16];
  int field_24;
  int field_28;
  char gap_2C[4];
  int* CriMvEasyPlayer;
  char gap_34[4];
  int field_38;
  char gap_3C[12];
  int field_48;
  int field_4C;
  char gap_50[4];
  int field_54;
  char gap_58[20];
  int field_6C;
  char gap_70[48];
  int field_A0;
  int field_A4;
  int field_A8;
  char gap_AC[24];
  int paused;
  int field_C8;
  char gap_CC[152];
  int field_164;
  int field_168;
  char gap_16C[4];
  int field_170;
  char gap_174[4];
  int num_frames_keep;
  char gap_17C[184];
} CriManaPlayerObj_t;

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
  CriManaPlayerObj_t* criManaPlayer;
  int criManaPlayerStatus;
  char gap_CDC[4];
  CriManaFrameInfo criManaFrameInfo;
  CriManaMovieInfo criManaMovieInfo;
  char gap_D9C[2100];
} MgsMoviePlayerObj_t;

static const size_t alignment =
    32;  // for fast memcpy, sizeof __m256, dunno if this *actually* matters but
         // since we only use it for giant framebuffers, might as well

typedef struct {
  bool playing = false;
  uint32_t destwidth = 0;
  uint32_t destheight = 0;
  uint32_t lastFrameNum = 0;
  double time = 0;
  csri_inst* csri = NULL;
} CriManaModState_t;

static std::unordered_map<MgsMoviePlayerObj_t*, CriManaModState_t*> stateMap;

static __m128i MaskFF000000 = _mm_set1_epi32(0xFF000000);

// video insertion

static char** videoNameTable;

typedef int(__thiscall* MgsMovieCPlayerPlayProc)(void* pThis, int a2, int a3, 
                                                 char* movieFileName);
static MgsMovieCPlayerPlayProc gameExeMgsMovieCPlayerPlay = NULL;
static MgsMovieCPlayerPlayProc gameExeMgsMovieCPlayerPlayReal = NULL;

typedef int(__thiscall* MgsMovieCPlayerRenderProc)(void* pThis);
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

typedef void(__cdecl* DrawAllThreadsProc)();
static DrawAllThreadsProc gameExeDrawAllThreads = NULL;
static DrawAllThreadsProc gameExeDrawAllThreadsReal = NULL;

typedef int(__cdecl* gslFillHookProc)(int id, int a1, int a2, int a3, int a4, int r, int g, int b, int a);
static gslFillHookProc gameExegslFill = NULL;
static gslFillHookProc gameExegslFillReal = NULL;

namespace lb {
int __cdecl gslCreateSurfaceHook(int id, int width, int height, int format);
int __cdecl gslDrawSpriteHook(int textureId, float spriteX, float spriteY,
                      float spriteWidth, float spriteHeight, float displayX,
                      float displayY, int color, int opacity, int shaderId);
int __fastcall mgsMovieCPlayerPlayHook(void* pThis, void* dummy, int a2,
                                       int a3, char* movieFileName);
int __fastcall mgsMovieCPlayerRenderHook(void* pThis);
void __cdecl drawAllThreadsHook();
int __cdecl gslFillHook(int id, int a1, int a2, int a3, int a4, int r, int g, int b, int a);

bool criManaModInit() {
  if (config["patch"].count("fmv") != 1) {
    LanguageBarrierLog(
        "No FMV mods in patch configuration, not hooking CriMana...");
    return true;
  }

  if (config["gamedef"]["signatures"]["game"].count("useOfCSurfaceObjects") == 1)
      gameExeCSurfaceObjects = (CSurface*)sigScan("game", "useOfCSurfaceObjects");

  if (!scanCreateEnableHook("game", "mgsMovieCPlayerPlay", (uintptr_t*)&gameExeMgsMovieCPlayerPlay,
                            (LPVOID)&mgsMovieCPlayerPlayHook,
                            (LPVOID*)&gameExeMgsMovieCPlayerPlayReal) ||
      !scanCreateEnableHook("game", "mgsMovieCPlayerRender", (uintptr_t*)&gameExeMgsMovieCPlayerRender,
                            (LPVOID)&mgsMovieCPlayerRenderHook,
                            (LPVOID*)&gameExeMgsMovieCPlayerRenderReal) ||
      !scanCreateEnableHook("game", "gslCreateSurface", (uintptr_t*)&gameExeGslCreateSurface,
                            (LPVOID)&gslCreateSurfaceHook,
                            (LPVOID*)&gameExeGslCreateSurfaceReal) ||
      !scanCreateEnableHook("game", "gslDrawSprite", (uintptr_t*)&gameExeGslDrawSprite,
                            (LPVOID)&gslDrawSpriteHook,
                            (LPVOID*)&gameExeGslDrawSpriteReal) ||
      !scanCreateEnableHook("game", "drawAllThreads", (uintptr_t*)&gameExeDrawAllThreads,
                            (LPVOID)&drawAllThreadsHook,
                            (LPVOID*)&gameExeDrawAllThreadsReal) ||
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

void drawAllThreadsHook() {
  gameExeDrawAllThreadsReal();

  if (stateMap.size() > 0 && stateMap.begin()->second->playing)
  {
      auto state = stateMap.begin()->second;
      gameExeCSurfaceObjects[191].vftable->mapTex(&gameExeCSurfaceObjects[191], 0, 0, 0);
      ID3D11Texture2D* help = gameExeCSurfaceObjects[191].dx11Texture[0];
      ID3D11ShaderResourceView* help1 = gameExeCSurfaceObjects[191].shaderRscView;
      gameExeCSurfaceObjects[191].dx11Texture[0] = gameExeCSurfaceObjects[191].dx11ReplaceTexture[0];
      csri_frame frame;
      frame.planes[0] = (uint8_t*)gameExeCSurfaceObjects[191].textureData;
      frame.strides[0] = 720;
      frame.pixfmt = CSRI_F_BGR_;
      csri_fmt format = { frame.pixfmt, gameExeCSurfaceObjects[191].textureWidth, gameExeCSurfaceObjects[191].textureHeight };
      if (csri_request_fmt(state->csri, &format) == 0) {
          csri_render(state->csri, &frame, state->time);
      }
      
      size_t i, imax;
      for (i = 0, imax = gameExeCSurfaceObjects[191].textureWidth * gameExeCSurfaceObjects[191].textureHeight; i < imax; i += 4) {
          __m128i* vec = (__m128i*)((uint32_t*)gameExeCSurfaceObjects[191].textureData + i);
          *vec = _mm_or_si128(*vec, MaskFF000000);
      }
      for (; i < imax; i++) {
          ((uint32_t*)gameExeCSurfaceObjects[191].textureData)[i] |= 0xFF000000;
      }
      
      D3D11_SHADER_RESOURCE_VIEW_DESC rscViewDesc;
      memset(&rscViewDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
      rscViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      rscViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      rscViewDesc.Buffer.NumElements = 1;
      rscViewDesc.Texture2D.MipLevels = 1;
      gameExePMgsD3D11State->pid3d11deviceC->CreateShaderResourceView(gameExeCSurfaceObjects[191].dx11Texture[0], &rscViewDesc, &gameExeCSurfaceObjects[191].shaderRscView);

      gameExeCSurfaceObjects[191].vftable->unmapTex(&gameExeCSurfaceObjects[191], 0);

      gslDrawSpriteHook(191, 0.0f, 0.0f, 1920.0f, 1080.0f, 0.0f, 0.0f, 0xFFFF00, 255, 1);
      gameExeCSurfaceObjects[191].dx11Texture[0] = help;
      gameExeCSurfaceObjects[191].shaderRscView = help1;
  }
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

int __fastcall mgsMovieCPlayerPlayHook(void* pThis, void* dummy, int a2,
                                       int a3, char* movieFileName) {
  MgsMoviePlayerObj_t* obj = (MgsMoviePlayerObj_t*)pThis;

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
      stateMap.emplace(obj, state);

      gslCreateSurfaceHook(312, 1920, 1080, 32);
      gslFillHook(312, 0, 0, 0, 0, 255, 0, 0, 255);

      state->csri =
          csri_open_mem(csri_renderer_default(), &sub[0], sub.size(), NULL);
      state->playing = true;
    }
    in.close();
  }

  return gameExeMgsMovieCPlayerPlayReal(pThis, a2, a3, movieFileName);
}

int __fastcall mgsMovieCPlayerRenderHook(void* pThis) {
  MgsMoviePlayerObj_t* obj = (MgsMoviePlayerObj_t*)pThis;

  if (stateMap.count(obj) == 0)
    return gameExeMgsMovieCPlayerRenderReal(pThis);

  CriManaModState_t* state = stateMap[obj];
  CriManaFrameInfo* frameInfo = &obj->criManaFrameInfo;

  if (state->csri == NULL)
    return gameExeMgsMovieCPlayerRenderReal(pThis);

  double time = ((double)frameInfo->framerate_d * (double)frameInfo->frame_no) /
                 (double)frameInfo->framerate_n;

  state->lastFrameNum = frameInfo->frame_no;
  state->time = time;

  int res = gameExeMgsMovieCPlayerRenderReal(pThis);
  return res;
}
}  // namespace lb