#include "BinkMod.h"
#include <csri/csri.h>
#include <stb_vorbis.h>
#include <emmintrin.h>
#include <fstream>
#include <unordered_map>
#include "Config.h"
#include "Game.h"
#include "LanguageBarrier.h"
#include "SigScan.h"

#define FLOATING_POINT
#define OUTSIDE_SPEEX
#define RANDOM_PREFIX lb_speexdsp_
#include "contrib/speexdsp/speex_resampler.h"

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

typedef struct BINKTRACK {
  uint32_t Frequency;
  uint32_t Bits;
  uint32_t Channels;
  uint32_t MaxSize;

  BINK* bink;
  uint32_t sndcomp;
  int32_t trackindex;
} BINKTRACK;

typedef BINK*(__stdcall* BINKOPEN)(const char* name, uint32_t flags);
typedef void(__stdcall* BINKCLOSE)(BINK* bnk);
typedef int32_t(__stdcall* BINKCOPYTOBUFFER)(BINK* bnk, void* dest,
                                             int32_t destpitch,
                                             uint32_t destheight,
                                             uint32_t destx, uint32_t desty,
                                             uint32_t flags);
typedef BINKTRACK*(__stdcall* BINKOPENTRACK)(BINK* bnk, uint32_t trackindex);
typedef void(__stdcall* BINKCLOSETRACK)(BINKTRACK* bnkt);
typedef void(__stdcall* BINKNEXTFRAME)(BINK* bnk);
typedef int(__stdcall* BINKDOFRAMEASYNCMULTI)(BINK* bnk,
                                              const int32_t* threadIds,
                                              int32_t numThreads);
typedef int(__stdcall* BINKDOFRAMEASYNCWAIT)(BINK* bnk, int32_t timeout);
typedef int(__stdcall* BINKSETVIDEOONOFF)(BINK* bnk, int32_t enable);
typedef void(__stdcall* BINKSETSOUNDTRACK)(uint32_t numTracks,
                                           const uint32_t* trackIds);
typedef uint32_t(__stdcall* BINKGETKEYFRAME)(BINK* bnk, uint32_t frame,
                                             uint32_t flags);
typedef int32_t(__stdcall* RADTIMERREAD)(void);

static BINKOPEN BinkOpen = NULL;
static BINKCLOSE BinkClose = NULL;
static BINKCOPYTOBUFFER BinkCopyToBuffer = NULL;
static BINKOPENTRACK BinkOpenTrack = NULL;
static BINKCLOSETRACK BinkCloseTrack = NULL;
static BINKNEXTFRAME BinkNextFrame = NULL;
static BINKDOFRAMEASYNCMULTI BinkDoFrameAsyncMulti = NULL;
static BINKDOFRAMEASYNCWAIT BinkDoFrameAsyncWait = NULL;
static BINKSETVIDEOONOFF BinkSetVideoOnOff = NULL;
static BINKSETSOUNDTRACK BinkSetSoundTrack = NULL;
static BINKGETKEYFRAME BinkGetKeyFrame = NULL;
static RADTIMERREAD RADTimerRead = NULL;

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

static const size_t alignment =
    32;  // for fast memcpy, sizeof __m256, dunno if this *actually* matters but
         // since we only use it for giant framebuffers, might as well

// HQ audio rewrite: Replacing decoded audio _in Bink_ with ours
// In our decompAudioInner replacement, we need to output previously decoded
// audio (discard less than we're decoding anew), hence the ring buffer

static const int CHANNEL_COUNT = 2;
static const int RINGBUFFER_CAPACITY =
    (4 * CHANNEL_COUNT * 0x800);  // 2 channels, 4 chunks (only ever seen 2 in
                                  // tests), 0x800 samples per channel per chunk
                                  // - double what we might ever need
static short ringbuffer[RINGBUFFER_CAPACITY];
static int rb_read_index;
static int rb_write_index;
static size_t rb_available_shorts;

// cba doing the sample rate calculations
static const int DECODED_BUFFER_CAPACITY = RINGBUFFER_CAPACITY / 2;
static const int RESAMPLED_BUFFER_CAPACITY = RINGBUFFER_CAPACITY;
static short decoded_buffer[DECODED_BUFFER_CAPACITY];
static short resampled_buffer[RESAMPLED_BUFFER_CAPACITY];
static size_t resampled_available_samples;
static size_t resampled_consumed_samples;

// can be negative,for skipping
static int delay_samples;

static uint32_t bink_sample_rate;
static uint32_t our_sample_rate;

// global because we don't have an easy way of getting to the bink handle in
// decompAudioInner, and so we don't add a map lookup to the hot path careful
// with multiple videos playing at once, if that's ever possible?
static stb_vorbis* pVorbis = NULL;
static std::string* pVorbisFile = NULL;

static SpeexResamplerState* resampler = NULL;

void clear_processing_buffers() {
  memset(resampled_buffer, 0, sizeof(short) * RESAMPLED_BUFFER_CAPACITY);
  resampled_consumed_samples = 0;
  resampled_available_samples = 0;
}
void fill_resampled_buffer() {
  clear_processing_buffers();
  size_t decoded_available_samples = stb_vorbis_get_samples_short_interleaved(
      pVorbis, CHANNEL_COUNT, decoded_buffer, DECODED_BUFFER_CAPACITY);
  size_t in_len = decoded_available_samples;
  size_t out_len = RESAMPLED_BUFFER_CAPACITY;
  speex_resampler_process_interleaved_int(resampler, decoded_buffer, &in_len,
                                          resampled_buffer, &out_len);
  resampled_available_samples = out_len;
}
size_t get_resampled(short* dest, size_t num_samples) {
  size_t total_fetched = 0;

  if (delay_samples > 0) {
    int delay_out = min(num_samples, delay_samples);
    memset(dest, 0, CHANNEL_COUNT * sizeof(short) * delay_out);
    delay_samples -= delay_out;
    num_samples -= delay_out;
    total_fetched += delay_out;
  }

  while (num_samples > 0) {
    if (resampled_available_samples == 0) {
      fill_resampled_buffer();
      if (resampled_available_samples == 0) {
        return total_fetched;
      }
    }

    size_t to_fetch = min(num_samples, resampled_available_samples);
    memcpy(dest + CHANNEL_COUNT * total_fetched,
           resampled_buffer + CHANNEL_COUNT * resampled_consumed_samples,
           CHANNEL_COUNT * sizeof(short) * to_fetch);
    total_fetched += to_fetch;
    resampled_consumed_samples += to_fetch;
    resampled_available_samples -= to_fetch;
    num_samples -= to_fetch;
  }
  return total_fetched;
}

// Just how drunk was I when I first wrote this?

size_t rb_read(short* dest, size_t shorts);
void rb_consume(size_t samples);

void rb_init() {
  resampler =
      speex_resampler_init(CHANNEL_COUNT, our_sample_rate, bink_sample_rate,
                           SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);

  clear_processing_buffers();
  rb_read_index = rb_write_index = rb_available_shorts = 0;
  memset(ringbuffer, 0, RINGBUFFER_CAPACITY * sizeof(short));

  while (delay_samples < 0) {
    size_t skipped =
        rb_read(NULL, min(RINGBUFFER_CAPACITY, -delay_samples * CHANNEL_COUNT));
    rb_consume(skipped);
    delay_samples += skipped / CHANNEL_COUNT;
  }
}
size_t rb_read(short* dest, size_t shorts) {
  if (shorts > rb_available_shorts) {
    // fetch more samples from vorbis decoder
    size_t fetch_total = shorts - rb_available_shorts;
    size_t fetch_first = min(fetch_total, RINGBUFFER_CAPACITY - rb_write_index);
    size_t actual_fetched_first = 0;
    if (fetch_first > 0) {
      actual_fetched_first = get_resampled(ringbuffer + rb_write_index,
                                           fetch_first / CHANNEL_COUNT) *
                             CHANNEL_COUNT;
      rb_write_index += actual_fetched_first;
    }
    rb_available_shorts += actual_fetched_first;

    // wrap around if necessary
    // TODO: handle fetching more than two iterations (rest of first + entire
    // RINGBUFFER_CAPACITY second)? that much should _never_ be requested though
    size_t fetch_second =
        min(fetch_total - actual_fetched_first, RINGBUFFER_CAPACITY);
    size_t actual_fetched_second = 0;
    if (fetch_second > 0) {
      // note: in this case rb_write_index is at capacity, start from the
      // beginning
      actual_fetched_second =
          get_resampled(ringbuffer, fetch_second / CHANNEL_COUNT) *
          CHANNEL_COUNT;
      rb_write_index = actual_fetched_second;
    }
    rb_available_shorts += actual_fetched_second;
  }

  size_t shorts_to_output_total = min(rb_available_shorts, shorts);
  size_t shorts_to_output_first =
      min(shorts_to_output_total, RINGBUFFER_CAPACITY - rb_read_index);
  size_t shorts_to_output_second =
      shorts_to_output_total - shorts_to_output_first;
  if (dest) {
    memcpy(dest, &ringbuffer[rb_read_index],
           sizeof(short) * shorts_to_output_first);
    if (shorts_to_output_second > 0) {
      memcpy(dest + shorts_to_output_first, ringbuffer,
             sizeof(short) * shorts_to_output_second);
    }
    if (shorts_to_output_first + shorts_to_output_second < shorts)
      memset(dest + shorts_to_output_first + shorts_to_output_second, 0,
             sizeof(short) *
                 (shorts - shorts_to_output_first - shorts_to_output_second));
  }
  return shorts_to_output_total;
}
void rb_consume(size_t samples) {
  samples = min(samples, rb_available_shorts);
  rb_available_shorts -= samples;
  rb_read_index += samples;
  rb_read_index %= RINGBUFFER_CAPACITY;
}

typedef int(__cdecl* DecompAudioInnerProc)(
    // it actually takes an extra flag argument in ecx&1,
    // but this cannot be represented in msvc
    // (with __thiscall the callee cleanups the stack, not the case here)
    int samplesPerChannelPerChunk, float a5, size_t chunkCount,
    char* overlapSrcBase, int* a8, unsigned int a9, int a10, int a11,
    char* dest, unsigned int overlapCtBytes, int a14, char* overlapDest);
static DecompAudioInnerProc binkDllDecompAudioInner =
    NULL;  // = (DecompAudioInnerProc)0x10025EF0; (C;C)
static DecompAudioInnerProc binkDllDecompAudioInnerReal = NULL;

typedef struct {
  BINK* otherFile;
  unsigned startFrame;
  unsigned frameCount;
  bool shouldCloseBink;
} BinkPartialRedirect_t;

bool operator<(const BinkPartialRedirect_t& left,
               const BinkPartialRedirect_t& right) {
  return left.startFrame < right.startFrame;
}

typedef struct {
  void* framebuffer = NULL;
  uint32_t destwidth = 0;
  uint32_t destheight = 0;
  uint32_t lastFrameNum = 0;
  csri_inst* csri = NULL;
  std::vector<BinkPartialRedirect_t> partialRedirects;
  size_t currentPartialRedirectPos = 0;
  BINK* nowRedirected = NULL;
  uint32_t redirectKeyFrame = 0;
} BinkModState_t;

static std::unordered_map<BINK*, BinkModState_t*> stateMap;

static __m128i MaskFF000000 = _mm_set1_epi32(0xFF000000);

// video insertion

static char** videoNameTable;

namespace lb {
BINK* __stdcall BinkOpenHook(const char* name, uint32_t flags);
void __stdcall BinkCloseHook(BINK* bnk);
int32_t __stdcall BinkCopyToBufferHook(BINK* bnk, void* dest, int32_t destpitch,
                                       uint32_t destheight, uint32_t destx,
                                       uint32_t desty, uint32_t flags);
int __cdecl decompAudioInnerHook(int samplesPerChannelPerChunk, float a5,
                                 size_t chunkCount, char* overlapSrcBase,
                                 int* a8, unsigned int a9, int a10, int a11,
                                 char* dest, unsigned int overlapCtBytes,
                                 int a14, char* overlapDest);
int decompAudioInnerHookThunk();
void __stdcall BinkNextFrameHook(BINK* bnk);
int __stdcall BinkDoFrameAsyncMultiHook(BINK* bnk, const int32_t* threadIds,
                                        int32_t numThreads);
int __stdcall BinkDoFrameAsyncWaitHook(BINK* bnk, int32_t timeout);

bool binkModInit() {
  if (config["patch"].count("fmv") != 1) {
    LanguageBarrierLog(
        "No FMV mods in patch configuration, not hooking Bink...");
    return true;
  }

  if (!createEnableApiHook(L"bink2w32", "_BinkOpen@8", BinkOpenHook,
                           (LPVOID*)&BinkOpen) ||
      !createEnableApiHook(L"bink2w32", "_BinkClose@4", BinkCloseHook,
                           (LPVOID*)&BinkClose) ||
      !createEnableApiHook(L"bink2w32", "_BinkCopyToBuffer@28",
                           BinkCopyToBufferHook, (LPVOID*)&BinkCopyToBuffer) ||
      !scanCreateEnableHook(
          "bink2w32", "decompAudioInner", (uintptr_t*)&binkDllDecompAudioInner,
          decompAudioInnerHookThunk, (LPVOID*)&binkDllDecompAudioInnerReal))
    return false;

  HMODULE hmodBink = GetModuleHandleW(L"bink2w32");
  BinkOpenTrack = (BINKOPENTRACK)GetProcAddress(hmodBink, "_BinkOpenTrack@8");
  BinkCloseTrack =
      (BINKCLOSETRACK)GetProcAddress(hmodBink, "_BinkCloseTrack@4");
  BinkSetVideoOnOff =
      (BINKSETVIDEOONOFF)GetProcAddress(hmodBink, "_BinkSetVideoOnOff@8");
  BinkSetSoundTrack =
      (BINKSETSOUNDTRACK)GetProcAddress(hmodBink, "_BinkSetSoundTrack@8");
  BinkGetKeyFrame =
      (BINKGETKEYFRAME)GetProcAddress(hmodBink, "_BinkGetKeyFrame@12");
  RADTimerRead = (RADTIMERREAD)GetProcAddress(hmodBink, "_RADTimerRead@0");

  if (config["patch"]["fmv"].count("fonts") == 1) {
    for (auto font : config["patch"]["fmv"]["fonts"]) {
      std::stringstream ss;
      ss << "languagebarrier\\subs\\fonts\\" << font.get<std::string>();
      std::string path = ss.str();
      AddFontResourceExA(path.c_str(), FR_PRIVATE, NULL);
    }
  }

  if (config["gamedef"].count("videoTableOrigCount") == 1 &&
      config["patch"]["fmv"].count("additionalVideos") == 1) {
    char*** videoTableUse = (char***)sigScan("game", "useOfVideoTable");
    if (videoTableUse == NULL) {
      LanguageBarrierLog(
          "Additional videos configured but video table not found!");
      return false;
    }

    int videoTableOrigCount =
        config["gamedef"]["videoTableOrigCount"].get<int>();
    int additionalCount = config["patch"]["fmv"]["additionalVideos"].size();

    videoNameTable =
        (char**)calloc(videoTableOrigCount + additionalCount, sizeof(char*));
    memcpy(videoNameTable, *videoTableUse, videoTableOrigCount * sizeof(char*));

    // we rely on "video redirection" later to handle 720p/1080p versions

    if (config["patch"]["fmv"].count("videoRedirection") == 0) {
      config["patch"]["fmv"]["videoRedirection"] = json::object();
    }

    for (int i = 0; i < additionalCount; i++) {
      std::string videoName =
          config["patch"]["fmv"]["additionalVideos"][i].get<std::string>();
      std::string videoInName = videoName + ".bk2";
      videoNameTable[videoTableOrigCount + i] = strdup(videoInName.c_str());
      config["patch"]["fmv"]["videoRedirection"][videoInName] =
          videoName + ".bik";
    }

    write_perms(videoTableUse, videoNameTable);
  }

  if (config["patch"]["fmv"].count("videoPartialRedirection") == 1) {
    createEnableApiHook(L"bink2w32", "_BinkNextFrame@4", BinkNextFrameHook,
                        (LPVOID*)&BinkNextFrame);
    createEnableApiHook(L"bink2w32", "_BinkDoFrameAsyncMulti@12",
                        BinkDoFrameAsyncMultiHook,
                        (LPVOID*)&BinkDoFrameAsyncMulti);
    createEnableApiHook(L"bink2w32", "_BinkDoFrameAsyncWait@8",
                        BinkDoFrameAsyncWaitHook,
                        (LPVOID*)&BinkDoFrameAsyncWait);
  }

  return true;
}

void updatePartialRedirectState(BINK* bnk, BinkModState_t* state) {
  size_t pos = state->currentPartialRedirectPos;
  if (pos >= state->partialRedirects.size()) return;
  BinkPartialRedirect_t* redir = &state->partialRedirects[pos];
  // frames in BINK are 1-based, and I prefer 0-based frames in config, so +1
  // check end-of-redirection condition before start-of-redirection
  // to allow adjacent redirects (not sure how useful it is, but why not?)
  if (bnk->FrameNum == redir->startFrame + redir->frameCount + 1) {
    BinkSetVideoOnOff(bnk, 1);
    ++state->currentPartialRedirectPos;
    state->nowRedirected = NULL;
    ++pos;
    ++redir;
    if (pos >= state->partialRedirects.size()) return;
  }
  if (bnk->FrameNum == redir->startFrame + 1) {
    state->nowRedirected = redir->otherFile;
    // If the redirection covers everything till the end,
    // we don't need to decode the original video anymore.
    if (bnk->FrameNum + redir->frameCount >= bnk->Frames) {
      BinkSetVideoOnOff(bnk, 0);
    } else {
      // If we will need to switch back and there is a keyframe,
      // we can skip decoding before the last keyframe,
      // but we will need to start decoding again after that.
      // If there is no keyframe, we can't skip decoding at all.
      //
      // flags=0 == search the frame before or equal to the given one
      uint32_t lastKeyFrame =
          BinkGetKeyFrame(bnk, bnk->FrameNum + redir->frameCount, 0);
      if (lastKeyFrame > bnk->FrameNum) {
        BinkSetVideoOnOff(bnk, 0);
        state->redirectKeyFrame = lastKeyFrame;
      }
    }
  }
  if (bnk->FrameNum == state->redirectKeyFrame) BinkSetVideoOnOff(bnk, 1);
}

BINK* __stdcall BinkOpenHook(const char* name, uint32_t flags) {
  char* dup = _strdup(name);
  char* tmp = dup;
  if (strrchr(tmp, '\\')) tmp = strrchr(tmp, '\\') + 1;
  if (strrchr(tmp, '/')) tmp = strrchr(tmp, '/') + 1;
  _strlwr(tmp);  // case isn't always equivalent to filename on disk

  std::string videoFilesDir = "languagebarrier\\videos\\";
  if (strstr(name, "1280x720"))
    videoFilesDir += "720p\\";
  else
    videoFilesDir += "1080p\\";

  BINK* bnk;

  if (config["patch"]["fmv"].count("videoRedirection") == 1 &&
      config["patch"]["fmv"]["videoRedirection"].count(tmp) == 1) {
    std::string videoFileName =
        config["patch"]["fmv"]["videoRedirection"][tmp].get<std::string>();

    std::string videoPath = videoFilesDir + videoFileName;
    std::stringstream logstr;
    logstr << "Redirecting " << tmp << " to " << videoPath << " ... ";

    bnk = BinkOpen(videoPath.c_str(), flags);
    if (!bnk) {
      logstr << "\r\n\tCouldn't open video, trying absolute path... ";
      std::string videoFullPath =
          WideTo8BitPath(GetGameDirectoryPath()) + "\\" + videoPath;
      bnk = BinkOpen(videoFullPath.c_str(), flags);
      if (!bnk) {
        logstr << "\r\n\tFalling back to original... ";
        bnk = BinkOpen(name, flags);
      }
    }

    if (bnk) {
      logstr << "Successfully opened";
    } else {
      logstr << "Failed";
    }
    LanguageBarrierLog(logstr.str());
  } else {
    bnk = BinkOpen(name, flags);
  }

  BinkModState_t* state = new BinkModState_t;
  stateMap.emplace(bnk, state);

  if (config["patch"]["fmv"].count("audioRedirection") == 1 &&
      config["patch"]["fmv"]["audioRedirection"].count(tmp) == 1) {
    if (pVorbis != NULL) {
      std::stringstream ss;
      ss << "Started playing another video with audio redirection configured, "
            "while audio redirection decoder still up: "
         << name;
      LanguageBarrierLog(ss.str());
    } else {
      std::string audioPath =
          "languagebarrier\\audio\\" +
          config["patch"]["fmv"]["audioRedirection"][tmp].get<std::string>();

      std::ifstream ain(audioPath, std::ios::in | std::ios::binary);
      if (ain.good()) {
        ain.seekg(0, std::ios::end);
        pVorbisFile = new std::string(ain.tellg(), 0);
        ain.seekg(0, std::ios::beg);
        ain.read(&(*pVorbisFile)[0], pVorbisFile->size());
      }
      ain.close();

      int vorbisError;
      pVorbis =
          stb_vorbis_open_memory((const unsigned char*)pVorbisFile->c_str(),
                                 pVorbisFile->size(), &vorbisError, NULL);

      if (pVorbis == NULL) {
        std::stringstream ss;
        ss << "Error while trying to open audio redirection at '" << audioPath
           << "' for video '" << tmp << "', error code: " << vorbisError;
        LanguageBarrierLog(ss.str());
        delete pVorbisFile;
        pVorbisFile = NULL;
      } else {
        BINKTRACK* track = BinkOpenTrack(bnk, 0);
        bink_sample_rate = track->Frequency;
        BinkCloseTrack(track);

        stb_vorbis_info vi = stb_vorbis_get_info(pVorbis);
        our_sample_rate = vi.sample_rate;

        // seems to be the magic number of audio delay
        delay_samples = -0.433f * (float)bink_sample_rate;

        rb_init();
      }
    }
  }

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

  if (config["patch"]["fmv"].count("videoPartialRedirection") == 1 &&
      config["patch"]["fmv"]["videoPartialRedirection"].count(tmp) == 1) {
    std::unordered_map<std::string, BINK*> otherFiles;
    for (const json& part :
         config["patch"]["fmv"]["videoPartialRedirection"][tmp]) {
      auto namePtr = part.find("name");
      if (namePtr == part.end() || !namePtr->is_string()) continue;
      std::string name = namePtr->get<std::string>();
      unsigned startFrame = part.value<unsigned>("startFrame", 0);
      unsigned frameCount = part.value<unsigned>("frameCount", 0);
      bool preload = part.value<bool>("preload", false);
      auto otherFile = otherFiles.find(name);
      bool shouldCloseBink = false;
      if (otherFile == otherFiles.end()) {
        std::string fullName = videoFilesDir + name;
        std::string log =
            "Trying to open " + fullName + " for videoPartialRedirection ... ";
        // 0x100000 means decoding alpha if it is present in the file
        // 0x4000 means use tracks from BinkSetSoundTrack
        BinkSetSoundTrack(0, NULL);  // no audio
        BINK* otherBink =
            BinkOpen(fullName.c_str(), preload ? 0x106000 : 0x104000);
        const static uint32_t zero = 0;
        BinkSetSoundTrack(1, &zero);  // reset to defaults
        if (!otherBink) {
          log += "failed";
          LanguageBarrierLog(log);
          continue;
        }
        log += "success";
        LanguageBarrierLog(log);
        shouldCloseBink = true;
        otherFile = otherFiles.insert(std::make_pair(name, otherBink)).first;
      }
      state->partialRedirects.push_back(BinkPartialRedirect_t());
      state->partialRedirects.back().otherFile = otherFile->second;
      state->partialRedirects.back().startFrame = startFrame;
      state->partialRedirects.back().frameCount = frameCount;
      state->partialRedirects.back().shouldCloseBink = shouldCloseBink;
    }
    std::sort(state->partialRedirects.begin(), state->partialRedirects.end());
    updatePartialRedirectState(bnk, state);
  }

  free(dup);
  return bnk;
}

void __stdcall BinkCloseHook(BINK* bnk) {
  BinkClose(bnk);
  if (stateMap.count(bnk) == 0) return;

  BinkModState_t* state = stateMap[bnk];
  if (state->framebuffer) {
    _aligned_free(state->framebuffer);
  }
  if (state->csri) {
    csri_close(state->csri);
  }
  if (resampler != NULL) {
    speex_resampler_destroy(resampler);
    resampler = NULL;
  }
  if (pVorbis != NULL) {
    stb_vorbis_close(pVorbis);
    pVorbis = NULL;
  }
  if (pVorbisFile != NULL) {
    delete pVorbisFile;
    pVorbisFile = NULL;
  }
  for (auto it = state->partialRedirects.begin();
       it != state->partialRedirects.end(); ++it)
    if (it->shouldCloseBink) BinkClose(it->otherFile);
  delete state;
  stateMap.erase(bnk);
}

int32_t __stdcall BinkCopyToBufferHook(BINK* bnk, void* dest, int32_t destpitch,
                                       uint32_t destheight, uint32_t destx,
                                       uint32_t desty, uint32_t flags) {
  if (stateMap.count(bnk) == 0)
    return BinkCopyToBuffer(bnk, dest, destpitch, destheight, destx, desty,
                            flags);
  BinkModState_t* state = stateMap[bnk];
  BINK* videoSourceBnk = state->nowRedirected ? state->nowRedirected : bnk;

  uint32_t destwidth = destpitch / 4;

  if (state->csri == NULL)
    return BinkCopyToBuffer(videoSourceBnk, dest, destpitch, destheight, destx,
                            desty, flags);

  double time = ((double)bnk->FrameRateDiv * (double)bnk->FrameNum) /
                (double)bnk->FrameRate;

  if (bnk->FrameNum == state->lastFrameNum && destheight == state->destheight &&
      destwidth == state->destwidth) {
    memcpy(dest, state->framebuffer, destpitch * destheight);
    return 0;
  }
  if (state->destheight != destheight || state->destwidth != destwidth ||
      state->framebuffer == NULL) {
    state->destheight = destheight;
    state->destwidth = destwidth;
    if (state->framebuffer != NULL) {
      _aligned_free(state->framebuffer);
    }
    state->framebuffer = _aligned_malloc(
        alignCeil(destpitch * destheight, alignment), alignment);
  }

  state->lastFrameNum = bnk->FrameNum;

  // TODO: actually use destx and desty (figure out if Bink clips output or
  // expects destpitch/destheight to match)
  int32_t retval = BinkCopyToBuffer(videoSourceBnk, state->framebuffer,
                                    destpitch, destheight, destx, desty, flags);

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

  memcpy(dest, state->framebuffer, destpitch * destheight);

  return retval;
}

void __stdcall BinkNextFrameHook(BINK* bnk) {
  BinkNextFrame(bnk);
  auto stateIter = stateMap.find(bnk);
  if (stateIter == stateMap.end()) return;
  BinkModState_t* state = stateIter->second;
  if (state->nowRedirected) BinkNextFrame(state->nowRedirected);
  updatePartialRedirectState(bnk, state);
}

int __stdcall BinkDoFrameAsyncMultiHook(BINK* bnk, const int32_t* threadIds,
                                        int32_t numThreads) {
  int ret = BinkDoFrameAsyncMulti(bnk, threadIds, numThreads);
  auto stateIter = stateMap.find(bnk);
  if (stateIter != stateMap.end() && stateIter->second->nowRedirected) {
    int ret2 = BinkDoFrameAsyncMulti(stateIter->second->nowRedirected,
                                     threadIds, numThreads);
    ret = ret && ret2;
  }
  return ret;
}

int __stdcall BinkDoFrameAsyncWaitHook(BINK* bnk, int32_t timeout) {
  auto stateIter = stateMap.find(bnk);
  if (stateIter == stateMap.end() || !stateIter->second->nowRedirected)
    return BinkDoFrameAsyncWait(bnk, timeout);
  BINK* other = stateIter->second->nowRedirected;
  if (timeout <= 0) {
    // negative timeout = infinite, zero timeout = check status, don't wait
    // in both cases, just do both calls
    int ret1 = BinkDoFrameAsyncWait(bnk, timeout);
    int ret2 = BinkDoFrameAsyncWait(other, timeout);
    return ret1 && ret2;
  }
  // GetTickCount() instead of (RADTimerRead() / 1000) would probably suffice as
  // well, but let's be closer to how the bink engine measures time
  uint32_t start = RADTimerRead();
  int ret1 = BinkDoFrameAsyncWait(bnk, timeout);
  if (!ret1) return 0;
  int32_t passed = (RADTimerRead() - start) / 1000;
  if (timeout > passed)
    timeout -= passed;
  else
    timeout = 0;
  return BinkDoFrameAsyncWait(other, timeout);
}

#pragma warning(push)
#pragma warning(disable : 4414)  // short jump to function converted to near
int __declspec(naked) decompAudioInnerHookThunk() {
  __asm {
    // remember to save/restore ecx if [binkDllDecompAudioInnerReal] is going to
    // be used
    cmp [pVorbis], 0
    jnz decompAudioInnerHook
    jmp [binkDllDecompAudioInnerReal]
  }
}
#pragma warning(pop)

int __cdecl decompAudioInnerHook(int samplesPerChannelPerChunk, float a5,
                                 size_t chunkCount, char* overlapSrcBase,
                                 int* a8, unsigned int a9, int a10, int a11,
                                 char* dest, unsigned int overlapCtBytes,
                                 int a14, char* overlapDest) {
  // note: rb_read and rb_consume take shorts, memmove takes bytes
  rb_read((short*)dest, CHANNEL_COUNT * samplesPerChannelPerChunk * chunkCount);
  rb_consume(CHANNEL_COUNT * samplesPerChannelPerChunk -
             (overlapCtBytes / sizeof(short)));
  memmove(overlapSrcBase,
          dest + sizeof(short) * CHANNEL_COUNT * samplesPerChannelPerChunk,
          sizeof(short) * CHANNEL_COUNT * samplesPerChannelPerChunk);
  return 0;  // number of bytes to advance a8; we don't use it, so we don't care
}
}  // namespace lb