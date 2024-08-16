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
#include <d3d9.h>

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

// partial
typedef struct {
  char gap0[8];
  char byte8;
  char gap9[2];
  char byteB;
  char gapC[12];
  uint32_t dword18;
  char gap1C[3118];
  uint16_t unsigned___int16C4A;
  char gapC4C[124];
  uint32_t dwordCC8;
  uint32_t dwordCCC;
  char gapCD0[8];
  CriManaFrameInfo criManaFrameInfo;
} MgsMoviePlayerRNDObj_t;

typedef struct {
  uint32_t lastFrameNum = 0;
  bool keepLastFrame = false;
  double time = 0;
  ID3D11Texture2D* stagingTexture = NULL;
  IDirect3DTexture9* stagingTexture9;
  csri_inst* csri = NULL;
} CriManaModState_t;

static std::unordered_map<void*, CriManaModState_t*> stateMap;

uint32_t RENDER_TARGET_SURF_ID = 199;

typedef int(__thiscall* MgsMovieCPlayerPlayProc)(void* pThis, int a2, int a3,
                                                 char* movieFileName);
static MgsMovieCPlayerPlayProc gameExeMgsMovieCPlayerPlay = NULL;
static MgsMovieCPlayerPlayProc gameExeMgsMovieCPlayerPlayReal = NULL;

typedef int(__thiscall* MgsMovieCPlayerPlayByIdProc)(void* pThis, int a2,
                                                     int id);
static MgsMovieCPlayerPlayByIdProc gameExeMgsMovieCPlayerPlayById = NULL;
static MgsMovieCPlayerPlayByIdProc gameExeMgsMovieCPlayerPlayByIdReal = NULL;

typedef int(__thiscall* MgsMovieCPlayerStopProc)(void* pThis);
static MgsMovieCPlayerStopProc gameExeMgsMovieCPlayerStop = NULL;
static MgsMovieCPlayerStopProc gameExeMgsMovieCPlayerStopReal = NULL;

typedef int(__thiscall* MgsMovieCPlayerRenderProc)(void* pThis);
static MgsMovieCPlayerRenderProc gameExeMgsMovieCPlayerRender = NULL;
static MgsMovieCPlayerRenderProc gameExeMgsMovieCPlayerRenderReal = NULL;

typedef int(__cdecl* DrawMovieFrameProc)(int tint, int opacity);
static DrawMovieFrameProc gameExeDrawMovieFrame = NULL;
static DrawMovieFrameProc gameExeDrawMovieFrameReal = NULL;

typedef int(__cdecl* DrawMovieFrameRNDProc)(int unk1, int tint, int opacity,
                                            int unk2);
static DrawMovieFrameRNDProc gameExeDrawMovieFrameRND = NULL;
static DrawMovieFrameRNDProc gameExeDrawMovieFrameRNDReal = NULL;

int VideoPlayerObjVariant = 0;

namespace lb {
int __fastcall mgsMovieCPlayerPlayHook(void* pThis, void* dummy, int a2, int a3,
                                       char* movieFileName);
int __fastcall mgsMovieCPlayerPlayByIdHook(void* pThis, void* dummy, int a2,
                                           int id);
void NewFunction(int id, void*& pThis);
int __fastcall mgsMovieCPlayerStopHook(void* pThis);
int __fastcall mgsMovieCPlayerRenderHook(void* pThis);
int __cdecl drawMovieFrameHook(int tint, int opacity);
int __cdecl drawMovieFrameHookRND(int unk1, int tint, int opacity, int unk2);

bool criManaModInit() {
  if (config["patch"].count("fmv") != 1) {
    LanguageBarrierLog(
        "No FMV mods in patch configuration, not hooking CriMana...");
    return true;
  }

  if (config["gamedef"].count("videoPlayerVariant") == 1 &&
      config["gamedef"]["videoPlayerVariant"] == "rnd") {
    VideoPlayerObjVariant = 1;
    RENDER_TARGET_SURF_ID = 210;

    if (!scanCreateEnableHook("game", "mgsMovieCPlayerPlayById",
                              (uintptr_t*)&gameExeMgsMovieCPlayerPlayById,
                              (LPVOID)&mgsMovieCPlayerPlayByIdHook,
                              (LPVOID*)&gameExeMgsMovieCPlayerPlayByIdReal) ||
        !scanCreateEnableHook("game", "drawMovieFrame",
                              (uintptr_t*)&gameExeDrawMovieFrameRND,
                              (LPVOID)&drawMovieFrameHookRND,
                              (LPVOID*)&gameExeDrawMovieFrameRNDReal))
      return false;
  } 
   else if (config["gamedef"].count("videoPlayerVariant") == 1 &&
      config["gamedef"]["videoPlayerVariant"] == "sge") {
    VideoPlayerObjVariant = 2;
    RENDER_TARGET_SURF_ID = 207;

    if (!scanCreateEnableHook("game", "mgsMovieCPlayerPlayById2",
                              (uintptr_t*)&gameExeMgsMovieCPlayerPlayById,
                              (LPVOID)&mgsMovieCPlayerPlayByIdHook,
                              (LPVOID*)&gameExeMgsMovieCPlayerPlayByIdReal) ||
        !scanCreateEnableHook("game", "drawMovieFrame",
                              (uintptr_t*)&gameExeDrawMovieFrameRND,
                              (LPVOID)&drawMovieFrameHook, (LPVOID*)&gameExeDrawMovieFrameReal))
      return false;
  } else {
    if (!scanCreateEnableHook("game", "mgsMovieCPlayerPlay",
                              (uintptr_t*)&gameExeMgsMovieCPlayerPlay,
                              (LPVOID)&mgsMovieCPlayerPlayHook,
                              (LPVOID*)&gameExeMgsMovieCPlayerPlayReal) ||
        !scanCreateEnableHook(
            "game", "drawMovieFrame", (uintptr_t*)&gameExeDrawMovieFrame,
            (LPVOID)&drawMovieFrameHook, (LPVOID*)&gameExeDrawMovieFrameReal))
      return false;
  }

  if (!scanCreateEnableHook("game", "mgsMovieCPlayerStop",
                            (uintptr_t*)&gameExeMgsMovieCPlayerStop,
                            (LPVOID)&mgsMovieCPlayerStopHook,
                            (LPVOID*)&gameExeMgsMovieCPlayerStopReal) ||
      !scanCreateEnableHook("game", "mgsMovieCPlayerRender",
                            (uintptr_t*)&gameExeMgsMovieCPlayerRender,
                            (LPVOID)&mgsMovieCPlayerRenderHook,
                            (LPVOID*)&gameExeMgsMovieCPlayerRenderReal))
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

static __m128i MaskFF000000 = _mm_set1_epi32(0xFFFFFF00);

void CopyD3D11Tex(CriManaModState_t* state);
void CopyD3D9Tex(CriManaModState_t* state);
void drawSubs(bool deferred) {
  for (const auto& kv : stateMap) {
    auto state = kv.second;
    if (!state->keepLastFrame) {
      if (deferred) {
        ID3D11CommandList* pCommandList;
        ID3D11DeviceContext* test =
            (&gameExePMgsD3D11State->pid3d11deferredcontext1)
                [*(uint32_t*)&gameExePMgsD3D11State->gap0];
        HRESULT hr = test->FinishCommandList(1, &pCommandList);
        gameExePMgsD3D11State->pid3d11devicecontext18->ExecuteCommandList(
            pCommandList, 0);
        pCommandList->Release();
      }

      if (state->stagingTexture)
      CopyD3D11Tex(state);
	  else
	  CopyD3D9Tex(state);
    }
  }
}

void CopyD3D11Tex(CriManaModState_t* state) {
  gameExePMgsD3D11State->pid3d11devicecontext18->CopyResource(
      state->stagingTexture,
      lb::SurfaceWrapper::getTexPtr(surfaceArray, RENDER_TARGET_SURF_ID, 0));

  D3D11_MAPPED_SUBRESOURCE rsc;
  memset(&rsc, 0, sizeof(D3D11_MAPPED_SUBRESOURCE));
  HRESULT hr = gameExePMgsD3D11State->pid3d11devicecontext18->Map(
      state->stagingTexture, 0, D3D11_MAP_READ_WRITE, 0, &rsc);

  uint8_t* imagePtr = (uint8_t*)rsc.pData;
  csri_frame frame;
  frame.planes[0] = imagePtr;
  frame.strides[0] = rsc.RowPitch;
  frame.pixfmt = CSRI_F_BGR_;
  csri_fmt format = {
      frame.pixfmt,
      lb::SurfaceWrapper::width(surfaceArray, RENDER_TARGET_SURF_ID),
      lb::SurfaceWrapper::height(surfaceArray, RENDER_TARGET_SURF_ID)};
  if (csri_request_fmt(state->csri, &format) == 0) {
    csri_render(state->csri, &frame, state->time);
  }

  gameExePMgsD3D11State->pid3d11devicecontext18->Unmap(state->stagingTexture,
                                                       0);
  gameExePMgsD3D11State->pid3d11devicecontext18->CopyResource(
      lb::SurfaceWrapper::getTexPtr(surfaceArray, RENDER_TARGET_SURF_ID, 0),
      state->stagingTexture);
}

int __cdecl drawMovieFrameHookRND(int unk1, int tint, int opacity, int unk2) {
  int ret = gameExeDrawMovieFrameRNDReal(unk1, tint, opacity, unk2);
  drawSubs(true);
  return ret;
}

int __cdecl drawMovieFrameHook(int tint, int opacity) {
  int ret = gameExeDrawMovieFrameReal(tint, opacity);
  drawSubs(false);
  return ret;
}

void __cdecl CopyD3D9Tex(CriManaModState_t* state) {
  csri_fmt format;                  // [esp+13Ch] [ebp-CCh] BYREF
  csri_frame frame;                 // [esp+150h] [ebp-B8h] BYREF
  unsigned __int8* imagePtr;        // [esp+17Ch] [ebp-8Ch]
  _D3DLOCKED_RECT lockedRect;       // [esp+188h] [ebp-80h] BYREF
  _D3DSURFACE_DESC desc;            // [esp+198h] [ebp-70h] BYREF
  HRESULT hr;                       // [esp+1C0h] [ebp-48h]
  IDirect3DSurface9* pSrcSurface;   // [esp+1CCh] [ebp-3Ch] BYREF
  IDirect3DSurface9* pDestSurface;  // [esp+1D8h] [ebp-30h] BYREF
  IDirect3DTexture9* stageTexture;  // [esp+1E4h] [ebp-24h]
  IDirect3DTexture9* pTexture;      // [esp+1F0h] [ebp-18h]
  IDirect3DDevice9* pD3DDevice;     // [esp+1FCh] [ebp-Ch]

  if (!gameExePMgsD3D9State)
    gameExePMgsD3D9State =       *(MgsD3D9State**)lb::sigScan("game", "useOfMgsD3D9State", 0);
  pD3DDevice = gameExePMgsD3D9State->device;
  RENDER_TARGET_SURF_ID = 207;
  pTexture =
      (IDirect3DTexture9*)lb::SurfaceWrapper::getTexPtr(surfaceArray, 207, 0);
  stageTexture = state->stagingTexture9;
  pDestSurface = 0;
  pSrcSurface = 0;
  stageTexture->GetSurfaceLevel(0, &pDestSurface);
  pTexture->GetSurfaceLevel( 0, &pSrcSurface);
  hr = pD3DDevice->StretchRect( pSrcSurface, 0, pDestSurface, 0,
                               D3DTEXF_NONE);
  pDestSurface->Release();
  hr = pD3DDevice->GetRenderTargetData( pSrcSurface, pDestSurface);
  stageTexture->GetLevelDesc( 0, &desc);
  hr = stageTexture->LockRect( 0, &lockedRect, 0, 0x2000u);
  if (hr >= 0) {
    imagePtr = (unsigned __int8*)lockedRect.pBits;
    frame.planes[0] = (unsigned __int8*)lockedRect.pBits;
    frame.strides[0] = lockedRect.Pitch;
    frame.pixfmt = CSRI_F_BGR_;
    format.pixfmt = CSRI_F_BGR_;
    format.width = desc.Width;
    format.height = desc.Height;
    if (!csri_request_fmt(state->csri, &format))
      csri_render(state->csri, &frame, state->time);
    stageTexture->UnlockRect( 0);
  }
  pD3DDevice->UpdateTexture( stageTexture, pTexture);
}

void __cdecl CreateD3D9Texture(CriManaModState_t* state) {
  IDirect3DTexture9* TexPtr;     // [esp+10h] [ebp-108h]
  _D3DSURFACE_DESC desc;         // [esp+E4h] [ebp-34h] BYREF
  IDirect3DDevice9* pD3DDevice;  // [esp+10Ch] [ebp-Ch]

  lb::SurfaceWrapper::game = SGE;
  if (!gameExePMgsD3D9State)
    gameExePMgsD3D9State =
        **(MgsD3D9State***)lb::sigScan("game", "useOfMgsD3D9State", 0);
  pD3DDevice = 0;
  memset(&desc, 0, sizeof(desc));
  TexPtr = (IDirect3DTexture9*)lb::SurfaceWrapper::getTexPtr(
      surfaceArray, RENDER_TARGET_SURF_ID, 0);
  TexPtr->GetLevelDesc(0, &desc);
  gameExePMgsD3D9State->device->CreateTexture(desc.Width, desc.Height, 1u, 0,
                                              desc.Format, D3DPOOL_SYSTEMMEM,
                                              &state->stagingTexture9, 0);
}


void SGEMethod(int a2, void* pThis) {
  std::string movieFileName = std::to_string(a2);
  std::string subFileName;
  auto& config = lb::config["patch"]["fmv"];

  if (config.count("subs") == 1 && config["subs"].count(movieFileName) == 1) {
    subFileName = config["subs"][movieFileName].get<std::string>();
  }

  if (!subFileName.empty()) {
    std::stringstream ssSubPath;
    ssSubPath << "languagebarrier\\subs\\" << subFileName;
    std::string subPath = ssSubPath.str();

    std::stringstream logstr;
    logstr << "Using sub track " << subPath << " if available.";
    lb::LanguageBarrierLog(logstr.str());

    std::ifstream in(subPath, std::ios::binary);
    if (in.good()) {
      in.seekg(0, std::ios::end);
      std::string sub(in.tellg(), '\0');
      in.seekg(0);
      in.read(&sub[0], sub.size());

      CriManaModState_t* state = new CriManaModState_t();
      stateMap.emplace(pThis, state);
      lb::CreateD3D9Texture(state);

      state->csri =
          csri_open_mem(csri_renderer_default(), &sub[0], sub.size(), nullptr);
    }
  }
}
void RNEMethod(int id, void*& pThis) {
  std::string subFileName;
  std::string movieFileName = std::to_string(id);
  // note: case sensitive
  if (config["patch"]["fmv"].count("subs") == 1 &&
      config["patch"]["fmv"]["subs"].count(movieFileName) == 1)
    subFileName =
        config["patch"]["fmv"]["subs"][movieFileName].get<std::string>();

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
      lb::SurfaceWrapper::getTexPtr(surfaceArray, RENDER_TARGET_SURF_ID, 0)
          ->GetDesc(&desc);
      desc.Usage = D3D11_USAGE_STAGING;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
      desc.BindFlags = 0;

      gameExePMgsD3D11State->pid3d11deviceC->CreateTexture2D(
          &desc, 0, &state->stagingTexture);

      state->csri =
          csri_open_mem(csri_renderer_default(), &sub[0], sub.size(), NULL);
    }
    in.close();
  }
}



int __fastcall mgsMovieCPlayerPlayByIdHook(void* pThis, void* dummy, int a2,
                                           int id) {
  if (VideoPlayerObjVariant == 2) {
    SGEMethod(id, pThis);
  } else if (VideoPlayerObjVariant == 1) {
    RNEMethod(id, pThis);
  }
  return gameExeMgsMovieCPlayerPlayByIdReal(pThis, a2, id);
}


int __fastcall mgsMovieCPlayerPlayHook(void* pThis, void* dummy, int a2, int a3,
                                       char* movieFileName) {
  std::string subFileName;
  // note: case sensitive
  if (config["patch"]["fmv"].count("subs") == 1 &&
      config["patch"]["fmv"]["subs"].count(movieFileName) == 1)
    subFileName =
        config["patch"]["fmv"]["subs"][movieFileName].get<std::string>();

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
      lb::SurfaceWrapper::getTexPtr(surfaceArray, RENDER_TARGET_SURF_ID, 0)
          ->GetDesc(&desc);
      desc.Usage = D3D11_USAGE_STAGING;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
      desc.BindFlags = 0;

      gameExePMgsD3D11State->pid3d11deviceC->CreateTexture2D(
          &desc, 0, &state->stagingTexture);

      state->csri =
          csri_open_mem(csri_renderer_default(), &sub[0], sub.size(), NULL);
    }
    in.close();
  }

  return gameExeMgsMovieCPlayerPlayReal(pThis, a2, a3, movieFileName);
}

int __fastcall mgsMovieCPlayerStopHook(void* pThis) {
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

int __fastcall mgsMovieCPlayerRenderHook(void* pThis) {
  if (stateMap.count(pThis) == 0)
    return gameExeMgsMovieCPlayerRenderReal(pThis);

  CriManaModState_t* state = stateMap[pThis];
  CriManaFrameInfo* frameInfo;
  if (VideoPlayerObjVariant) {
    MgsMoviePlayerRNDObj_t* obj = (MgsMoviePlayerRNDObj_t*)pThis;
    frameInfo = &obj->criManaFrameInfo;
  } else {
    MgsMoviePlayerObj_t* obj = (MgsMoviePlayerObj_t*)pThis;
    frameInfo = &obj->criManaFrameInfo;
  }

  if (state->csri == NULL) return gameExeMgsMovieCPlayerRenderReal(pThis);

  double time = ((double)frameInfo->framerate_d * (double)frameInfo->frame_no) /
                (double)frameInfo->framerate_n;

  state->keepLastFrame = frameInfo->frame_no == state->lastFrameNum;

  state->lastFrameNum = frameInfo->frame_no;
  state->time = time;

  return gameExeMgsMovieCPlayerRenderReal(pThis);
}
}  // namespace lb