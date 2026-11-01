#ifndef __XAUDIO2ABI_H__
#define __XAUDIO2ABI_H__

#include <Windows.h>
#include <Unknwn.h>
#include <mmsystem.h>
#include <mmreg.h>

namespace lb {
namespace xa2 {

// partial XAudio2 2.7 (DirectX SDK June 2010) and 2.9 (Windows 10) ABIs
// 2.7 headers only ship with the old DirectX SDK, and the Windows SDK's
// xaudio2.h wants a Win8+ target, so we declare what we need ourselves

#pragma pack(push, 1)  // same as xaudio2.h

struct SEND_DESCRIPTOR {
  UINT32 Flags;
  void* pOutputVoice;
};
struct VOICE_SENDS {
  UINT32 SendCount;
  SEND_DESCRIPTOR* pSends;
};
// same in both versions, we just pass these through
struct EFFECT_CHAIN;
struct FILTER_PARAMETERS;
struct BUFFER;
struct BUFFER_WMA;
struct DEBUG_CONFIGURATION;
struct VOICE_STATE;
class IVoiceCallback;
class IEngineCallback;

struct VOICE_DETAILS_27 {
  UINT32 CreationFlags, InputChannels, InputSampleRate;
};
struct VOICE_DETAILS_29 {  // ActiveFlags is new in 2.8
  UINT32 CreationFlags, ActiveFlags, InputChannels, InputSampleRate;
};

struct DEVICE_DETAILS_27 {
  WCHAR DeviceID[256];
  WCHAR DisplayName[256];
  UINT32 Role;
  WAVEFORMATEXTENSIBLE OutputFormat;
};

struct PERFORMANCE_DATA_27 {
  UINT64 AudioCyclesSinceLastQuery, TotalCyclesSinceLastQuery;
  UINT32 MinimumCyclesPerQuantum, MaximumCyclesPerQuantum, MemoryUsageInBytes,
      CurrentLatencyInSamples, GlitchesSinceEngineStarted,
      ActiveSourceVoiceCount, TotalSourceVoiceCount, ActiveSubmixVoiceCount,
      TotalSubmixVoiceCount, ActiveXmaSourceVoices, ActiveXmaStreams;
};
struct PERFORMANCE_DATA_29 {
  UINT64 AudioCyclesSinceLastQuery, TotalCyclesSinceLastQuery;
  UINT32 MinimumCyclesPerQuantum, MaximumCyclesPerQuantum, MemoryUsageInBytes,
      CurrentLatencyInSamples, GlitchesSinceEngineStarted,
      ActiveSourceVoiceCount, TotalSourceVoiceCount, ActiveSubmixVoiceCount,
      ActiveResamplerCount, ActiveMatrixMixCount, ActiveXmaSourceVoices,
      ActiveXmaStreams;
};

#pragma pack(pop)

constexpr HRESULT E_INVALID_CALL = (HRESULT)0x88960001;
constexpr UINT32 VOICE_MUSIC_27 = 0x10;  // removed in 2.8, 2.9 rejects it
constexpr UINT32 DEBUG_ENGINE = 0x1;
constexpr UINT32 PROCESSOR1 = 0x1;
constexpr UINT32 GLOBAL_DEFAULT_DEVICE = 0xf;
constexpr UINT32 AUDIO_CATEGORY_GAME_EFFECTS = 6;  // AudioCategory_GameEffects

class __declspec(uuid("5a508685-a254-4fba-9b82-9a24b00306af")) XAudio2_27;
class __declspec(uuid("db05ea35-0329-4d4b-a53a-6dead03d3852")) XAudio2_27_Debug;

// IXAudio2Voice only differs in the GetVoiceDetails() struct
// note: voices aren't COM objects, no IUnknown here
template <typename Details>
class IVoiceT {
 public:
  virtual void __stdcall GetVoiceDetails(Details* pVoiceDetails) = 0;
  virtual HRESULT __stdcall SetOutputVoices(const VOICE_SENDS* pSendList) = 0;
  virtual HRESULT __stdcall SetEffectChain(
      const EFFECT_CHAIN* pEffectChain) = 0;
  virtual HRESULT __stdcall EnableEffect(UINT32 EffectIndex,
                                         UINT32 OperationSet) = 0;
  virtual HRESULT __stdcall DisableEffect(UINT32 EffectIndex,
                                          UINT32 OperationSet) = 0;
  virtual void __stdcall GetEffectState(UINT32 EffectIndex, BOOL* pEnabled) = 0;
  virtual HRESULT __stdcall SetEffectParameters(UINT32 EffectIndex,
                                                const void* pParameters,
                                                UINT32 ParametersByteSize,
                                                UINT32 OperationSet) = 0;
  virtual HRESULT __stdcall GetEffectParameters(UINT32 EffectIndex,
                                                void* pParameters,
                                                UINT32 ParametersByteSize) = 0;
  virtual HRESULT __stdcall SetFilterParameters(
      const FILTER_PARAMETERS* pParameters, UINT32 OperationSet) = 0;
  virtual void __stdcall GetFilterParameters(
      FILTER_PARAMETERS* pParameters) = 0;
  virtual HRESULT __stdcall SetOutputFilterParameters(
      IVoiceT* pDestinationVoice, const FILTER_PARAMETERS* pParameters,
      UINT32 OperationSet) = 0;
  virtual void __stdcall GetOutputFilterParameters(
      IVoiceT* pDestinationVoice, FILTER_PARAMETERS* pParameters) = 0;
  virtual HRESULT __stdcall SetVolume(float Volume, UINT32 OperationSet) = 0;
  virtual void __stdcall GetVolume(float* pVolume) = 0;
  virtual HRESULT __stdcall SetChannelVolumes(UINT32 Channels,
                                              const float* pVolumes,
                                              UINT32 OperationSet) = 0;
  virtual void __stdcall GetChannelVolumes(UINT32 Channels,
                                           float* pVolumes) = 0;
  virtual HRESULT __stdcall SetOutputMatrix(IVoiceT* pDestinationVoice,
                                            UINT32 SourceChannels,
                                            UINT32 DestinationChannels,
                                            const float* pLevelMatrix,
                                            UINT32 OperationSet) = 0;
  virtual void __stdcall GetOutputMatrix(IVoiceT* pDestinationVoice,
                                         UINT32 SourceChannels,
                                         UINT32 DestinationChannels,
                                         float* pLevelMatrix) = 0;
  virtual void __stdcall DestroyVoice() = 0;
};
using IVoice27 = IVoiceT<VOICE_DETAILS_27>;
using IVoice29 = IVoiceT<VOICE_DETAILS_29>;

// GetState() got an extra Flags argument in 2.8, everything else is the same
class ISourceVoice27 : public IVoice27 {
 public:
  virtual HRESULT __stdcall Start(UINT32 Flags, UINT32 OperationSet) = 0;
  virtual HRESULT __stdcall Stop(UINT32 Flags, UINT32 OperationSet) = 0;
  virtual HRESULT __stdcall SubmitSourceBuffer(
      const BUFFER* pBuffer, const BUFFER_WMA* pBufferWMA) = 0;
  virtual HRESULT __stdcall FlushSourceBuffers() = 0;
  virtual HRESULT __stdcall Discontinuity() = 0;
  virtual HRESULT __stdcall ExitLoop(UINT32 OperationSet) = 0;
  virtual void __stdcall GetState(VOICE_STATE* pVoiceState) = 0;
  virtual HRESULT __stdcall SetFrequencyRatio(float Ratio,
                                              UINT32 OperationSet) = 0;
  virtual void __stdcall GetFrequencyRatio(float* pRatio) = 0;
  virtual HRESULT __stdcall SetSourceSampleRate(UINT32 NewSourceSampleRate) = 0;
};
class ISourceVoice29 : public IVoice29 {
 public:
  virtual HRESULT __stdcall Start(UINT32 Flags, UINT32 OperationSet) = 0;
  virtual HRESULT __stdcall Stop(UINT32 Flags, UINT32 OperationSet) = 0;
  virtual HRESULT __stdcall SubmitSourceBuffer(
      const BUFFER* pBuffer, const BUFFER_WMA* pBufferWMA) = 0;
  virtual HRESULT __stdcall FlushSourceBuffers() = 0;
  virtual HRESULT __stdcall Discontinuity() = 0;
  virtual HRESULT __stdcall ExitLoop(UINT32 OperationSet) = 0;
  virtual void __stdcall GetState(VOICE_STATE* pVoiceState, UINT32 Flags) = 0;
  virtual HRESULT __stdcall SetFrequencyRatio(float Ratio,
                                              UINT32 OperationSet) = 0;
  virtual void __stdcall GetFrequencyRatio(float* pRatio) = 0;
  virtual HRESULT __stdcall SetSourceSampleRate(UINT32 NewSourceSampleRate) = 0;
};

class __declspec(uuid("8bcf1f58-9fe7-4583-8ac6-e2adc465c8bb")) IXAudio2_27
    : public IUnknown {
 public:
  virtual HRESULT __stdcall GetDeviceCount(UINT32* pCount) = 0;
  virtual HRESULT __stdcall GetDeviceDetails(
      UINT32 Index, DEVICE_DETAILS_27* pDeviceDetails) = 0;
  virtual HRESULT __stdcall Initialize(UINT32 Flags,
                                       UINT32 XAudio2Processor) = 0;
  virtual HRESULT __stdcall RegisterForCallbacks(
      IEngineCallback* pCallback) = 0;
  virtual void __stdcall UnregisterForCallbacks(IEngineCallback* pCallback) = 0;
  virtual HRESULT __stdcall CreateSourceVoice(
      ISourceVoice27** ppSourceVoice, const WAVEFORMATEX* pSourceFormat,
      UINT32 Flags, float MaxFrequencyRatio, IVoiceCallback* pCallback,
      const VOICE_SENDS* pSendList, const EFFECT_CHAIN* pEffectChain) = 0;
  virtual HRESULT __stdcall CreateSubmixVoice(
      IVoice27** ppSubmixVoice, UINT32 InputChannels, UINT32 InputSampleRate,
      UINT32 Flags, UINT32 ProcessingStage, const VOICE_SENDS* pSendList,
      const EFFECT_CHAIN* pEffectChain) = 0;
  virtual HRESULT __stdcall CreateMasteringVoice(
      IVoice27** ppMasteringVoice, UINT32 InputChannels, UINT32 InputSampleRate,
      UINT32 Flags, UINT32 DeviceIndex, const EFFECT_CHAIN* pEffectChain) = 0;
  virtual HRESULT __stdcall StartEngine() = 0;
  virtual void __stdcall StopEngine() = 0;
  virtual HRESULT __stdcall CommitChanges(UINT32 OperationSet) = 0;
  virtual void __stdcall GetPerformanceData(PERFORMANCE_DATA_27* pPerfData) = 0;
  virtual void __stdcall SetDebugConfiguration(
      const DEBUG_CONFIGURATION* pDebugConfiguration, void* pReserved) = 0;
};

class IXAudio2_29 : public IUnknown {
 public:
  virtual HRESULT __stdcall RegisterForCallbacks(
      IEngineCallback* pCallback) = 0;
  virtual void __stdcall UnregisterForCallbacks(IEngineCallback* pCallback) = 0;
  virtual HRESULT __stdcall CreateSourceVoice(
      ISourceVoice29** ppSourceVoice, const WAVEFORMATEX* pSourceFormat,
      UINT32 Flags, float MaxFrequencyRatio, IVoiceCallback* pCallback,
      const VOICE_SENDS* pSendList, const EFFECT_CHAIN* pEffectChain) = 0;
  virtual HRESULT __stdcall CreateSubmixVoice(
      IVoice29** ppSubmixVoice, UINT32 InputChannels, UINT32 InputSampleRate,
      UINT32 Flags, UINT32 ProcessingStage, const VOICE_SENDS* pSendList,
      const EFFECT_CHAIN* pEffectChain) = 0;
  virtual HRESULT __stdcall CreateMasteringVoice(
      IVoice29** ppMasteringVoice, UINT32 InputChannels, UINT32 InputSampleRate,
      UINT32 Flags, LPCWSTR szDeviceId, const EFFECT_CHAIN* pEffectChain,
      UINT32 StreamCategory) = 0;
  virtual HRESULT __stdcall StartEngine() = 0;
  virtual void __stdcall StopEngine() = 0;
  virtual HRESULT __stdcall CommitChanges(UINT32 OperationSet) = 0;
  virtual void __stdcall GetPerformanceData(PERFORMANCE_DATA_29* pPerfData) = 0;
  virtual void __stdcall SetDebugConfiguration(
      const DEBUG_CONFIGURATION* pDebugConfiguration, void* pReserved) = 0;
};

typedef HRESULT(__stdcall* XAudio2CreateProc)(IXAudio2_29** ppXAudio2,
                                              UINT32 Flags,
                                              UINT32 XAudio2Processor);

}  // namespace xa2
}  // namespace lb

#endif  // !__XAUDIO2ABI_H__
