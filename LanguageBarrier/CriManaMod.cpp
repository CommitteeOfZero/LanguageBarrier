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
  uint32_t lastFrameNum = 0;
  bool keepLastFrame = false;
  double time = 0;
  ID3D11Texture2D* stagingTexture = NULL;
  csri_inst* csri = NULL;
} CriManaModState_t;

static std::unordered_map<MgsMoviePlayerObj_t*, CriManaModState_t*> stateMap;

uint32_t RENDER_TARGET_SURF_ID = 199;

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

typedef int(__cdecl* DrawMovieFrameProc)(int tint, int opacity);
static DrawMovieFrameProc gameExeDrawMovieFrame = NULL;
static DrawMovieFrameProc gameExeDrawMovieFrameReal = NULL;

namespace lb {
int __fastcall mgsMovieCPlayerPlayHook(MgsMoviePlayerObj_t* pThis, void* dummy, int a2,
                                       int a3, char* movieFileName);
int __fastcall mgsMovieCPlayerStopHook(MgsMoviePlayerObj_t* pThis);
int __fastcall mgsMovieCPlayerRenderHook(MgsMoviePlayerObj_t* pThis);
int __cdecl drawMovieFrameHook(int tint, int opacity);

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
      !scanCreateEnableHook("game", "drawMovieFrame", (uintptr_t*)&gameExeDrawMovieFrame,
                            (LPVOID)&drawMovieFrameHook,
                            (LPVOID*)&gameExeDrawMovieFrameReal))
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
    auto state = kv.second;
    if (!state->keepLastFrame) {      
      gameExePMgsD3D11State->pid3d11devicecontext18->CopyResource(state->stagingTexture, surfaceArray[RENDER_TARGET_SURF_ID].texPtr[0]);
      
      D3D11_MAPPED_SUBRESOURCE rsc;
      memset(&rsc, 0, sizeof(D3D11_MAPPED_SUBRESOURCE));
      gameExePMgsD3D11State->pid3d11devicecontext18->Map(state->stagingTexture, 0, D3D11_MAP_READ_WRITE, 0, &rsc);
      
      uint8_t* imagePtr = (uint8_t*)rsc.pData;
      csri_frame frame;
      frame.planes[0] = imagePtr;
      frame.strides[0] = rsc.RowPitch;
      frame.pixfmt = CSRI_F_BGR_;
      csri_fmt format = { frame.pixfmt, surfaceArray[RENDER_TARGET_SURF_ID].width, surfaceArray[RENDER_TARGET_SURF_ID].height };
      if (csri_request_fmt(state->csri, &format) == 0) {
          csri_render(state->csri, &frame, state->time);
      }
      
      gameExePMgsD3D11State->pid3d11devicecontext18->Unmap(state->stagingTexture, 0);
      gameExePMgsD3D11State->pid3d11devicecontext18->CopyResource(surfaceArray[RENDER_TARGET_SURF_ID].texPtr[0], state->stagingTexture);
    }
  }

  return ret;
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

      D3D11_TEXTURE2D_DESC desc;
      memset(&desc, 0, sizeof(D3D11_TEXTURE2D_DESC));
      surfaceArray[RENDER_TARGET_SURF_ID].texPtr[0]->GetDesc(&desc);
      desc.Usage = D3D11_USAGE_STAGING;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
      desc.BindFlags = 0;

      gameExePMgsD3D11State->pid3d11deviceC->CreateTexture2D(&desc, 0, &state->stagingTexture);

      state->csri =
          csri_open_mem(csri_renderer_default(), &sub[0], sub.size(), NULL);
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
  if (state->stagingTexture) {
      state->stagingTexture->Release();
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