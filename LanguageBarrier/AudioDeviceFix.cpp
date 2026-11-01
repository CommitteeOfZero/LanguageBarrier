#include "LanguageBarrier.h"
#include "AudioDeviceFix.h"

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <vector>
#include "XAudio2Abi.h"

// Workaround for "audio keeps playing on the old device after changing the
// default output device": XAudio2 2.7 binds the mastering voice to the
// default device at creation and never follows it afterwards. So we hand the
// game a 2.7 facade over the XAudio2 2.9 that comes with Windows 10, whose
// mastering voice follows the default device if created without a device ID.

namespace {
using namespace lb::xa2;

XAudio2CreateProc realXAudio2Create = nullptr;
HRESULT(__stdcall* realCoCreateInstance)
(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*) = nullptr;

#define FORWARD(target, ret, name, params, args) \
  ret __stdcall name params override { return target->name args; }

IVoice29* unwrap(IVoice27* voice);

// sends point to our wrappers, 2.9 wants its own voices
struct SendList {
  VOICE_SENDS list = {};
  std::vector<SEND_DESCRIPTOR> sends;
  const VOICE_SENDS* ptr = nullptr;

  explicit SendList(const VOICE_SENDS* in) {
    if (!in) return;
    for (UINT32 i = 0; i < in->SendCount; i++)
      sends.push_back(
          {in->pSends[i].Flags, unwrap((IVoice27*)in->pSends[i].pOutputVoice)});
    list = {in->SendCount, sends.data()};
    ptr = &list;
  }
};

template <typename T>
class VoiceImpl : public T {
 public:
  IVoice29* real_;

  explicit VoiceImpl(IVoice29* real) : real_(real) {}
  virtual ~VoiceImpl() = default;

  void __stdcall GetVoiceDetails(VOICE_DETAILS_27* out) override {
    VOICE_DETAILS_29 d = {};
    real_->GetVoiceDetails(&d);
    *out = {d.CreationFlags, d.InputChannels, d.InputSampleRate};
  }
  HRESULT __stdcall SetOutputVoices(const VOICE_SENDS* sends) override {
    return real_->SetOutputVoices(SendList(sends).ptr);
  }
  void __stdcall DestroyVoice() override {
    real_->DestroyVoice();
    delete this;
  }

  FORWARD(real_, HRESULT, SetEffectChain, (const EFFECT_CHAIN* c), (c))
  FORWARD(real_, HRESULT, EnableEffect, (UINT32 i, UINT32 op), (i, op))
  FORWARD(real_, HRESULT, DisableEffect, (UINT32 i, UINT32 op), (i, op))
  FORWARD(real_, void, GetEffectState, (UINT32 i, BOOL* e), (i, e))
  FORWARD(real_, HRESULT, SetEffectParameters,
          (UINT32 i, const void* p, UINT32 s, UINT32 op), (i, p, s, op))
  FORWARD(real_, HRESULT, GetEffectParameters, (UINT32 i, void* p, UINT32 s),
          (i, p, s))
  FORWARD(real_, HRESULT, SetFilterParameters,
          (const FILTER_PARAMETERS* p, UINT32 op), (p, op))
  FORWARD(real_, void, GetFilterParameters, (FILTER_PARAMETERS * p), (p))
  FORWARD(real_, HRESULT, SetOutputFilterParameters,
          (IVoice27 * v, const FILTER_PARAMETERS* p, UINT32 op),
          (unwrap(v), p, op))
  FORWARD(real_, void, GetOutputFilterParameters,
          (IVoice27 * v, FILTER_PARAMETERS* p), (unwrap(v), p))
  FORWARD(real_, HRESULT, SetVolume, (float v, UINT32 op), (v, op))
  FORWARD(real_, void, GetVolume, (float* v), (v))
  FORWARD(real_, HRESULT, SetChannelVolumes,
          (UINT32 n, const float* v, UINT32 op), (n, v, op))
  FORWARD(real_, void, GetChannelVolumes, (UINT32 n, float* v), (n, v))
  FORWARD(real_, HRESULT, SetOutputMatrix,
          (IVoice27 * v, UINT32 src, UINT32 dst, const float* m, UINT32 op),
          (unwrap(v), src, dst, m, op))
  FORWARD(real_, void, GetOutputMatrix,
          (IVoice27 * v, UINT32 src, UINT32 dst, float* m),
          (unwrap(v), src, dst, m))
};
typedef VoiceImpl<IVoice27> PlainVoice;

// every wrapper is a VoiceImpl<> with real_ right after the vtable pointer, so
// this works for source voices too
IVoice29* unwrap(IVoice27* voice) {
  return voice ? reinterpret_cast<PlainVoice*>(voice)->real_ : nullptr;
}

class SourceVoice : public VoiceImpl<ISourceVoice27> {
 public:
  explicit SourceVoice(ISourceVoice29* real) : VoiceImpl(real) {}
  ISourceVoice29* src() { return static_cast<ISourceVoice29*>(real_); }

  void __stdcall GetState(VOICE_STATE* state) override {
    src()->GetState(state, 0);  // 0 = include SamplesPlayed, like 2.7 did
  }

  FORWARD(src(), HRESULT, Start, (UINT32 f, UINT32 op), (f, op))
  FORWARD(src(), HRESULT, Stop, (UINT32 f, UINT32 op), (f, op))
  FORWARD(src(), HRESULT, SubmitSourceBuffer,
          (const BUFFER* b, const BUFFER_WMA* w), (b, w))
  FORWARD(src(), HRESULT, FlushSourceBuffers, (), ())
  FORWARD(src(), HRESULT, Discontinuity, (), ())
  FORWARD(src(), HRESULT, ExitLoop, (UINT32 op), (op))
  FORWARD(src(), HRESULT, SetFrequencyRatio, (float r, UINT32 op), (r, op))
  FORWARD(src(), void, GetFrequencyRatio, (float* r), (r))
  FORWARD(src(), HRESULT, SetSourceSampleRate, (UINT32 r), (r))
};

HRESULT getDefaultDeviceDetails(DEVICE_DETAILS_27* out) {
  ZeroMemory(out, sizeof(*out));
  wcscpy_s(out->DeviceID, L"default");
  wcscpy_s(out->DisplayName, L"Default Audio Device");
  out->Role = GLOBAL_DEFAULT_DEVICE;

  IMMDeviceEnumerator* enumerator = nullptr;
  IMMDevice* device = nullptr;
  IAudioClient* client = nullptr;
  WAVEFORMATEX* format = nullptr;
  HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                                IID_PPV_ARGS(&enumerator));
  if (SUCCEEDED(hr))
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
  if (SUCCEEDED(hr))
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL,
                          (void**)&client);
  if (SUCCEEDED(hr)) hr = client->GetMixFormat(&format);
  if (SUCCEEDED(hr)) {
    size_t size = sizeof(WAVEFORMATEX) + format->cbSize;
    memcpy(&out->OutputFormat, format,
           size < sizeof(out->OutputFormat) ? size : sizeof(out->OutputFormat));
  }
  CoTaskMemFree(format);
  if (client) client->Release();
  if (device) device->Release();
  if (enumerator) enumerator->Release();
  return hr;
}

class Engine : public IXAudio2_27 {
 public:
  HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override {
    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, __uuidof(IXAudio2_27))) {
      *ppv = nullptr;
      return E_NOINTERFACE;
    }
    *ppv = static_cast<IXAudio2_27*>(this);
    AddRef();
    return S_OK;
  }
  ULONG __stdcall AddRef() override { return InterlockedIncrement(&refs_); }
  ULONG __stdcall Release() override {
    ULONG refs = InterlockedDecrement(&refs_);
    if (refs == 0) {
      if (real_) real_->Release();
      delete this;
    }
    return refs;
  }

  // we only ever expose the default device
  HRESULT __stdcall GetDeviceCount(UINT32* count) override {
    *count = 1;
    return S_OK;
  }
  HRESULT __stdcall GetDeviceDetails(UINT32 index,
                                     DEVICE_DETAILS_27* out) override {
    return index ? E_INVALIDARG : getDefaultDeviceDetails(out);
  }
  HRESULT __stdcall Initialize(UINT32 flags, UINT32) override {
    if (real_) return E_INVALID_CALL;
    // ignore the 2.7 processor hint, just use 2.9's default
    return realXAudio2Create(&real_, flags & ~DEBUG_ENGINE, PROCESSOR1);
  }
  HRESULT __stdcall CreateSourceVoice(ISourceVoice27** out,
                                      const WAVEFORMATEX* format, UINT32 flags,
                                      float maxRatio, IVoiceCallback* callback,
                                      const VOICE_SENDS* sends,
                                      const EFFECT_CHAIN* chain) override {
    ISourceVoice29* voice = nullptr;
    HRESULT hr = real_->CreateSourceVoice(&voice, format,
                                          flags & ~VOICE_MUSIC_27, maxRatio,
                                          callback, SendList(sends).ptr, chain);
    *out = SUCCEEDED(hr) ? new SourceVoice(voice) : nullptr;
    return hr;
  }
  HRESULT __stdcall CreateSubmixVoice(IVoice27** out, UINT32 channels,
                                      UINT32 rate, UINT32 flags, UINT32 stage,
                                      const VOICE_SENDS* sends,
                                      const EFFECT_CHAIN* chain) override {
    IVoice29* voice = nullptr;
    HRESULT hr = real_->CreateSubmixVoice(&voice, channels, rate,
                                          flags & ~VOICE_MUSIC_27, stage,
                                          SendList(sends).ptr, chain);
    *out = SUCCEEDED(hr) ? new PlainVoice(voice) : nullptr;
    return hr;
  }
  HRESULT __stdcall CreateMasteringVoice(IVoice27** out, UINT32 channels,
                                         UINT32 rate, UINT32 flags, UINT32,
                                         const EFFECT_CHAIN* chain) override {
    IVoice29* voice = nullptr;
    // NULL device ID = virtualized default device, which is the whole point
    HRESULT hr =
        real_->CreateMasteringVoice(&voice, channels, rate, flags, NULL, chain,
                                    AUDIO_CATEGORY_GAME_EFFECTS);
    *out = SUCCEEDED(hr) ? new PlainVoice(voice) : nullptr;
    return hr;
  }
  void __stdcall GetPerformanceData(PERFORMANCE_DATA_27* out) override {
    PERFORMANCE_DATA_29 d = {};
    real_->GetPerformanceData(&d);
    memcpy(out, &d, offsetof(PERFORMANCE_DATA_27, TotalSubmixVoiceCount));
    out->TotalSubmixVoiceCount = d.ActiveSubmixVoiceCount;  // gone in 2.8+
    out->ActiveXmaSourceVoices = d.ActiveXmaSourceVoices;
    out->ActiveXmaStreams = d.ActiveXmaStreams;
  }

  FORWARD(real_, HRESULT, RegisterForCallbacks, (IEngineCallback * c), (c))
  FORWARD(real_, void, UnregisterForCallbacks, (IEngineCallback * c), (c))
  FORWARD(real_, HRESULT, StartEngine, (), ())
  FORWARD(real_, void, StopEngine, (), ())
  FORWARD(real_, HRESULT, CommitChanges, (UINT32 op), (op))
  FORWARD(real_, void, SetDebugConfiguration,
          (const DEBUG_CONFIGURATION* c, void* r), (c, r))

 private:
  virtual ~Engine() = default;
  volatile LONG refs_ = 1;
  IXAudio2_29* real_ = nullptr;
};

#undef FORWARD

HRESULT __stdcall CoCreateInstanceHook(REFCLSID clsid, LPUNKNOWN outer,
                                       DWORD context, REFIID riid,
                                       LPVOID* ppv) {
  if (outer || (!IsEqualCLSID(clsid, __uuidof(XAudio2_27)) &&
                !IsEqualCLSID(clsid, __uuidof(XAudio2_27_Debug))))
    return realCoCreateInstance(clsid, outer, context, riid, ppv);
  Engine* engine = new Engine();
  HRESULT hr = engine->QueryInterface(riid, ppv);
  engine->Release();
  lb::LanguageBarrierLog("Redirecting XAudio2 2.7 to XAudio2 2.9");
  return hr;
}
}  // namespace

namespace lb {
bool audioDeviceFixInit() {
  // Wine/Proton has its own XAudio2 (FAudio), leave it alone
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (ntdll && GetProcAddress(ntdll, "wine_get_version")) {
    LanguageBarrierLog("Running under Wine, not hooking XAudio2...");
    return false;
  }

  HMODULE xaudio29 =
      LoadLibraryExW(L"xaudio2_9.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (xaudio29)
    realXAudio2Create =
        (XAudio2CreateProc)GetProcAddress(xaudio29, "XAudio2Create");
  if (!realXAudio2Create) {
    LanguageBarrierLog("XAudio2 2.9 not available, not hooking XAudio2...");
    return false;
  }
  return createEnableApiHook(L"ole32", "CoCreateInstance",
                             (LPVOID)CoCreateInstanceHook,
                             (LPVOID*)&realCoCreateInstance);
}
}  // namespace lb
