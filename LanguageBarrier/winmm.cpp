#include <Windows.h>
#include "LanguageBarrier.h"
#include "Game.h"

struct winmm_dll {
	HMODULE dll;
	FARPROC OrignalCloseDriver;
	FARPROC OrignalDefDriverProc;
	FARPROC OrignalDriverCallback;
	FARPROC OrignalDrvGetModuleHandle;
	FARPROC OrignalGetDriverModuleHandle;
	FARPROC OrignalNotifyCallbackData;
	FARPROC OrignalOpenDriver;
	FARPROC OrignalPlaySound;
	FARPROC OrignalPlaySoundA;
	FARPROC OrignalPlaySoundW;
	FARPROC OrignalSendDriverMessage;
	FARPROC OrignalWOW32DriverCallback;
	FARPROC OrignalWOW32ResolveMultiMediaHandle;
	FARPROC OrignalWOWAppExit;
	FARPROC Orignalaux32Message;
	FARPROC OrignalauxGetDevCapsA;
	FARPROC OrignalauxGetDevCapsW;
	FARPROC OrignalauxGetNumDevs;
	FARPROC OrignalauxGetVolume;
	FARPROC OrignalauxOutMessage;
	FARPROC OrignalauxSetVolume;
	FARPROC Orignaljoy32Message;
	FARPROC OrignaljoyConfigChanged;
	FARPROC OrignaljoyGetDevCapsA;
	FARPROC OrignaljoyGetDevCapsW;
	FARPROC OrignaljoyGetNumDevs;
	FARPROC OrignaljoyGetPos;
	FARPROC OrignaljoyGetPosEx;
	FARPROC OrignaljoyGetThreshold;
	FARPROC OrignaljoyReleaseCapture;
	FARPROC OrignaljoySetCapture;
	FARPROC OrignaljoySetThreshold;
	FARPROC Orignalmci32Message;
	FARPROC OrignalmciDriverNotify;
	FARPROC OrignalmciDriverYield;
	FARPROC OrignalmciExecute;
	FARPROC OrignalmciFreeCommandResource;
	FARPROC OrignalmciGetCreatorTask;
	FARPROC OrignalmciGetDeviceIDA;
	FARPROC OrignalmciGetDeviceIDFromElementIDA;
	FARPROC OrignalmciGetDeviceIDFromElementIDW;
	FARPROC OrignalmciGetDeviceIDW;
	FARPROC OrignalmciGetDriverData;
	FARPROC OrignalmciGetErrorStringA;
	FARPROC OrignalmciGetErrorStringW;
	FARPROC OrignalmciGetYieldProc;
	FARPROC OrignalmciLoadCommandResource;
	FARPROC OrignalmciSendCommandA;
	FARPROC OrignalmciSendCommandW;
	FARPROC OrignalmciSendStringA;
	FARPROC OrignalmciSendStringW;
	FARPROC OrignalmciSetDriverData;
	FARPROC OrignalmciSetYieldProc;
	FARPROC Orignalmid32Message;
	FARPROC OrignalmidiConnect;
	FARPROC OrignalmidiDisconnect;
	FARPROC OrignalmidiInAddBuffer;
	FARPROC OrignalmidiInClose;
	FARPROC OrignalmidiInGetDevCapsA;
	FARPROC OrignalmidiInGetDevCapsW;
	FARPROC OrignalmidiInGetErrorTextA;
	FARPROC OrignalmidiInGetErrorTextW;
	FARPROC OrignalmidiInGetID;
	FARPROC OrignalmidiInGetNumDevs;
	FARPROC OrignalmidiInMessage;
	FARPROC OrignalmidiInOpen;
	FARPROC OrignalmidiInPrepareHeader;
	FARPROC OrignalmidiInReset;
	FARPROC OrignalmidiInStart;
	FARPROC OrignalmidiInStop;
	FARPROC OrignalmidiInUnprepareHeader;
	FARPROC OrignalmidiOutCacheDrumPatches;
	FARPROC OrignalmidiOutCachePatches;
	FARPROC OrignalmidiOutClose;
	FARPROC OrignalmidiOutGetDevCapsA;
	FARPROC OrignalmidiOutGetDevCapsW;
	FARPROC OrignalmidiOutGetErrorTextA;
	FARPROC OrignalmidiOutGetErrorTextW;
	FARPROC OrignalmidiOutGetID;
	FARPROC OrignalmidiOutGetNumDevs;
	FARPROC OrignalmidiOutGetVolume;
	FARPROC OrignalmidiOutLongMsg;
	FARPROC OrignalmidiOutMessage;
	FARPROC OrignalmidiOutOpen;
	FARPROC OrignalmidiOutPrepareHeader;
	FARPROC OrignalmidiOutReset;
	FARPROC OrignalmidiOutSetVolume;
	FARPROC OrignalmidiOutShortMsg;
	FARPROC OrignalmidiOutUnprepareHeader;
	FARPROC OrignalmidiStreamClose;
	FARPROC OrignalmidiStreamOpen;
	FARPROC OrignalmidiStreamOut;
	FARPROC OrignalmidiStreamPause;
	FARPROC OrignalmidiStreamPosition;
	FARPROC OrignalmidiStreamProperty;
	FARPROC OrignalmidiStreamRestart;
	FARPROC OrignalmidiStreamStop;
	FARPROC OrignalmixerClose;
	FARPROC OrignalmixerGetControlDetailsA;
	FARPROC OrignalmixerGetControlDetailsW;
	FARPROC OrignalmixerGetDevCapsA;
	FARPROC OrignalmixerGetDevCapsW;
	FARPROC OrignalmixerGetID;
	FARPROC OrignalmixerGetLineControlsA;
	FARPROC OrignalmixerGetLineControlsW;
	FARPROC OrignalmixerGetLineInfoA;
	FARPROC OrignalmixerGetLineInfoW;
	FARPROC OrignalmixerGetNumDevs;
	FARPROC OrignalmixerMessage;
	FARPROC OrignalmixerOpen;
	FARPROC OrignalmixerSetControlDetails;
	FARPROC OrignalmmDrvInstall;
	FARPROC OrignalmmGetCurrentTask;
	FARPROC OrignalmmTaskBlock;
	FARPROC OrignalmmTaskCreate;
	FARPROC OrignalmmTaskSignal;
	FARPROC OrignalmmTaskYield;
	FARPROC OrignalmmioAdvance;
	FARPROC OrignalmmioAscend;
	FARPROC OrignalmmioClose;
	FARPROC OrignalmmioCreateChunk;
	FARPROC OrignalmmioDescend;
	FARPROC OrignalmmioFlush;
	FARPROC OrignalmmioGetInfo;
	FARPROC OrignalmmioInstallIOProcA;
	FARPROC OrignalmmioInstallIOProcW;
	FARPROC OrignalmmioOpenA;
	FARPROC OrignalmmioOpenW;
	FARPROC OrignalmmioRead;
	FARPROC OrignalmmioRenameA;
	FARPROC OrignalmmioRenameW;
	FARPROC OrignalmmioSeek;
	FARPROC OrignalmmioSendMessage;
	FARPROC OrignalmmioSetBuffer;
	FARPROC OrignalmmioSetInfo;
	FARPROC OrignalmmioStringToFOURCCA;
	FARPROC OrignalmmioStringToFOURCCW;
	FARPROC OrignalmmioWrite;
	FARPROC OrignalmmsystemGetVersion;
	FARPROC Orignalmod32Message;
	FARPROC Orignalmxd32Message;
	FARPROC OrignalsndPlaySoundA;
	FARPROC OrignalsndPlaySoundW;
	FARPROC Orignaltid32Message;
	FARPROC OrignaltimeBeginPeriod;
	FARPROC OrignaltimeEndPeriod;
	FARPROC OrignaltimeGetDevCaps;
	FARPROC OrignaltimeGetSystemTime;
	FARPROC OrignaltimeGetTime;
	FARPROC OrignaltimeKillEvent;
	FARPROC OrignaltimeSetEvent;
	FARPROC OrignalwaveInAddBuffer;
	FARPROC OrignalwaveInClose;
	FARPROC OrignalwaveInGetDevCapsA;
	FARPROC OrignalwaveInGetDevCapsW;
	FARPROC OrignalwaveInGetErrorTextA;
	FARPROC OrignalwaveInGetErrorTextW;
	FARPROC OrignalwaveInGetID;
	FARPROC OrignalwaveInGetNumDevs;
	FARPROC OrignalwaveInGetPosition;
	FARPROC OrignalwaveInMessage;
	FARPROC OrignalwaveInOpen;
	FARPROC OrignalwaveInPrepareHeader;
	FARPROC OrignalwaveInReset;
	FARPROC OrignalwaveInStart;
	FARPROC OrignalwaveInStop;
	FARPROC OrignalwaveInUnprepareHeader;
	FARPROC OrignalwaveOutBreakLoop;
	FARPROC OrignalwaveOutClose;
	FARPROC OrignalwaveOutGetDevCapsA;
	FARPROC OrignalwaveOutGetDevCapsW;
	FARPROC OrignalwaveOutGetErrorTextA;
	FARPROC OrignalwaveOutGetErrorTextW;
	FARPROC OrignalwaveOutGetID;
	FARPROC OrignalwaveOutGetNumDevs;
	FARPROC OrignalwaveOutGetPitch;
	FARPROC OrignalwaveOutGetPlaybackRate;
	FARPROC OrignalwaveOutGetPosition;
	FARPROC OrignalwaveOutGetVolume;
	FARPROC OrignalwaveOutMessage;
	FARPROC OrignalwaveOutOpen;
	FARPROC OrignalwaveOutPause;
	FARPROC OrignalwaveOutPrepareHeader;
	FARPROC OrignalwaveOutReset;
	FARPROC OrignalwaveOutRestart;
	FARPROC OrignalwaveOutSetPitch;
	FARPROC OrignalwaveOutSetPlaybackRate;
	FARPROC OrignalwaveOutSetVolume;
	FARPROC OrignalwaveOutUnprepareHeader;
	FARPROC OrignalwaveOutWrite;
	FARPROC Orignalwid32Message;
	FARPROC Orignalwod32Message;
} winmm;

__declspec(naked) void FakeCloseDriver() { _asm { jmp[winmm.OrignalCloseDriver] } }
__declspec(naked) void FakeDefDriverProc() { _asm { jmp[winmm.OrignalDefDriverProc] } }
__declspec(naked) void FakeDriverCallback() { _asm { jmp[winmm.OrignalDriverCallback] } }
__declspec(naked) void FakeDrvGetModuleHandle() { _asm { jmp[winmm.OrignalDrvGetModuleHandle] } }
__declspec(naked) void FakeGetDriverModuleHandle() { _asm { jmp[winmm.OrignalGetDriverModuleHandle] } }
__declspec(naked) void FakeNotifyCallbackData() { _asm { jmp[winmm.OrignalNotifyCallbackData] } }
__declspec(naked) void FakeOpenDriver() { _asm { jmp[winmm.OrignalOpenDriver] } }
__declspec(naked) void FakePlaySound() { _asm { jmp[winmm.OrignalPlaySound] } }
__declspec(naked) void FakePlaySoundA() { _asm { jmp[winmm.OrignalPlaySoundA] } }
__declspec(naked) void FakePlaySoundW() { _asm { jmp[winmm.OrignalPlaySoundW] } }
__declspec(naked) void FakeSendDriverMessage() { _asm { jmp[winmm.OrignalSendDriverMessage] } }
__declspec(naked) void FakeWOW32DriverCallback() { _asm { jmp[winmm.OrignalWOW32DriverCallback] } }
__declspec(naked) void FakeWOW32ResolveMultiMediaHandle() { _asm { jmp[winmm.OrignalWOW32ResolveMultiMediaHandle] } }
__declspec(naked) void FakeWOWAppExit() { _asm { jmp[winmm.OrignalWOWAppExit] } }
__declspec(naked) void Fakeaux32Message() { _asm { jmp[winmm.Orignalaux32Message] } }
__declspec(naked) void FakeauxGetDevCapsA() { _asm { jmp[winmm.OrignalauxGetDevCapsA] } }
__declspec(naked) void FakeauxGetDevCapsW() { _asm { jmp[winmm.OrignalauxGetDevCapsW] } }
__declspec(naked) void FakeauxGetNumDevs() { _asm { jmp[winmm.OrignalauxGetNumDevs] } }
__declspec(naked) void FakeauxGetVolume() { _asm { jmp[winmm.OrignalauxGetVolume] } }
__declspec(naked) void FakeauxOutMessage() { _asm { jmp[winmm.OrignalauxOutMessage] } }
__declspec(naked) void FakeauxSetVolume() { _asm { jmp[winmm.OrignalauxSetVolume] } }
__declspec(naked) void Fakejoy32Message() { _asm { jmp[winmm.Orignaljoy32Message] } }
__declspec(naked) void FakejoyConfigChanged() { _asm { jmp[winmm.OrignaljoyConfigChanged] } }
__declspec(naked) void FakejoyGetDevCapsA() { _asm { jmp[winmm.OrignaljoyGetDevCapsA] } }
__declspec(naked) void FakejoyGetDevCapsW() { _asm { jmp[winmm.OrignaljoyGetDevCapsW] } }
__declspec(naked) void FakejoyGetNumDevs() { _asm { jmp[winmm.OrignaljoyGetNumDevs] } }
__declspec(naked) void FakejoyGetPos() { _asm { jmp[winmm.OrignaljoyGetPos] } }
__declspec(naked) void FakejoyGetPosEx() { _asm { jmp[winmm.OrignaljoyGetPosEx] } }
__declspec(naked) void FakejoyGetThreshold() { _asm { jmp[winmm.OrignaljoyGetThreshold] } }
__declspec(naked) void FakejoyReleaseCapture() { _asm { jmp[winmm.OrignaljoyReleaseCapture] } }
__declspec(naked) void FakejoySetCapture() { _asm { jmp[winmm.OrignaljoySetCapture] } }
__declspec(naked) void FakejoySetThreshold() { _asm { jmp[winmm.OrignaljoySetThreshold] } }
__declspec(naked) void Fakemci32Message() { _asm { jmp[winmm.Orignalmci32Message] } }
__declspec(naked) void FakemciDriverNotify() { _asm { jmp[winmm.OrignalmciDriverNotify] } }
__declspec(naked) void FakemciDriverYield() { _asm { jmp[winmm.OrignalmciDriverYield] } }
__declspec(naked) void FakemciExecute() { _asm { jmp[winmm.OrignalmciExecute] } }
__declspec(naked) void FakemciFreeCommandResource() { _asm { jmp[winmm.OrignalmciFreeCommandResource] } }
__declspec(naked) void FakemciGetCreatorTask() { _asm { jmp[winmm.OrignalmciGetCreatorTask] } }
__declspec(naked) void FakemciGetDeviceIDA() { _asm { jmp[winmm.OrignalmciGetDeviceIDA] } }
__declspec(naked) void FakemciGetDeviceIDFromElementIDA() { _asm { jmp[winmm.OrignalmciGetDeviceIDFromElementIDA] } }
__declspec(naked) void FakemciGetDeviceIDFromElementIDW() { _asm { jmp[winmm.OrignalmciGetDeviceIDFromElementIDW] } }
__declspec(naked) void FakemciGetDeviceIDW() { _asm { jmp[winmm.OrignalmciGetDeviceIDW] } }
__declspec(naked) void FakemciGetDriverData() { _asm { jmp[winmm.OrignalmciGetDriverData] } }
__declspec(naked) void FakemciGetErrorStringA() { _asm { jmp[winmm.OrignalmciGetErrorStringA] } }
__declspec(naked) void FakemciGetErrorStringW() { _asm { jmp[winmm.OrignalmciGetErrorStringW] } }
__declspec(naked) void FakemciGetYieldProc() { _asm { jmp[winmm.OrignalmciGetYieldProc] } }
__declspec(naked) void FakemciLoadCommandResource() { _asm { jmp[winmm.OrignalmciLoadCommandResource] } }
__declspec(naked) void FakemciSendCommandA() { _asm { jmp[winmm.OrignalmciSendCommandA] } }
__declspec(naked) void FakemciSendCommandW() { _asm { jmp[winmm.OrignalmciSendCommandW] } }
__declspec(naked) void FakemciSendStringA() { _asm { jmp[winmm.OrignalmciSendStringA] } }
__declspec(naked) void FakemciSendStringW() { _asm { jmp[winmm.OrignalmciSendStringW] } }
__declspec(naked) void FakemciSetDriverData() { _asm { jmp[winmm.OrignalmciSetDriverData] } }
__declspec(naked) void FakemciSetYieldProc() { _asm { jmp[winmm.OrignalmciSetYieldProc] } }
__declspec(naked) void Fakemid32Message() { _asm { jmp[winmm.Orignalmid32Message] } }
__declspec(naked) void FakemidiConnect() { _asm { jmp[winmm.OrignalmidiConnect] } }
__declspec(naked) void FakemidiDisconnect() { _asm { jmp[winmm.OrignalmidiDisconnect] } }
__declspec(naked) void FakemidiInAddBuffer() { _asm { jmp[winmm.OrignalmidiInAddBuffer] } }
__declspec(naked) void FakemidiInClose() { _asm { jmp[winmm.OrignalmidiInClose] } }
__declspec(naked) void FakemidiInGetDevCapsA() { _asm { jmp[winmm.OrignalmidiInGetDevCapsA] } }
__declspec(naked) void FakemidiInGetDevCapsW() { _asm { jmp[winmm.OrignalmidiInGetDevCapsW] } }
__declspec(naked) void FakemidiInGetErrorTextA() { _asm { jmp[winmm.OrignalmidiInGetErrorTextA] } }
__declspec(naked) void FakemidiInGetErrorTextW() { _asm { jmp[winmm.OrignalmidiInGetErrorTextW] } }
__declspec(naked) void FakemidiInGetID() { _asm { jmp[winmm.OrignalmidiInGetID] } }
__declspec(naked) void FakemidiInGetNumDevs() { _asm { jmp[winmm.OrignalmidiInGetNumDevs] } }
__declspec(naked) void FakemidiInMessage() { _asm { jmp[winmm.OrignalmidiInMessage] } }
__declspec(naked) void FakemidiInOpen() { _asm { jmp[winmm.OrignalmidiInOpen] } }
__declspec(naked) void FakemidiInPrepareHeader() { _asm { jmp[winmm.OrignalmidiInPrepareHeader] } }
__declspec(naked) void FakemidiInReset() { _asm { jmp[winmm.OrignalmidiInReset] } }
__declspec(naked) void FakemidiInStart() { _asm { jmp[winmm.OrignalmidiInStart] } }
__declspec(naked) void FakemidiInStop() { _asm { jmp[winmm.OrignalmidiInStop] } }
__declspec(naked) void FakemidiInUnprepareHeader() { _asm { jmp[winmm.OrignalmidiInUnprepareHeader] } }
__declspec(naked) void FakemidiOutCacheDrumPatches() { _asm { jmp[winmm.OrignalmidiOutCacheDrumPatches] } }
__declspec(naked) void FakemidiOutCachePatches() { _asm { jmp[winmm.OrignalmidiOutCachePatches] } }
__declspec(naked) void FakemidiOutClose() { _asm { jmp[winmm.OrignalmidiOutClose] } }
__declspec(naked) void FakemidiOutGetDevCapsA() { _asm { jmp[winmm.OrignalmidiOutGetDevCapsA] } }
__declspec(naked) void FakemidiOutGetDevCapsW() { _asm { jmp[winmm.OrignalmidiOutGetDevCapsW] } }
__declspec(naked) void FakemidiOutGetErrorTextA() { _asm { jmp[winmm.OrignalmidiOutGetErrorTextA] } }
__declspec(naked) void FakemidiOutGetErrorTextW() { _asm { jmp[winmm.OrignalmidiOutGetErrorTextW] } }
__declspec(naked) void FakemidiOutGetID() { _asm { jmp[winmm.OrignalmidiOutGetID] } }
__declspec(naked) void FakemidiOutGetNumDevs() { _asm { jmp[winmm.OrignalmidiOutGetNumDevs] } }
__declspec(naked) void FakemidiOutGetVolume() { _asm { jmp[winmm.OrignalmidiOutGetVolume] } }
__declspec(naked) void FakemidiOutLongMsg() { _asm { jmp[winmm.OrignalmidiOutLongMsg] } }
__declspec(naked) void FakemidiOutMessage() { _asm { jmp[winmm.OrignalmidiOutMessage] } }
__declspec(naked) void FakemidiOutOpen() { _asm { jmp[winmm.OrignalmidiOutOpen] } }
__declspec(naked) void FakemidiOutPrepareHeader() { _asm { jmp[winmm.OrignalmidiOutPrepareHeader] } }
__declspec(naked) void FakemidiOutReset() { _asm { jmp[winmm.OrignalmidiOutReset] } }
__declspec(naked) void FakemidiOutSetVolume() { _asm { jmp[winmm.OrignalmidiOutSetVolume] } }
__declspec(naked) void FakemidiOutShortMsg() { _asm { jmp[winmm.OrignalmidiOutShortMsg] } }
__declspec(naked) void FakemidiOutUnprepareHeader() { _asm { jmp[winmm.OrignalmidiOutUnprepareHeader] } }
__declspec(naked) void FakemidiStreamClose() { _asm { jmp[winmm.OrignalmidiStreamClose] } }
__declspec(naked) void FakemidiStreamOpen() { _asm { jmp[winmm.OrignalmidiStreamOpen] } }
__declspec(naked) void FakemidiStreamOut() { _asm { jmp[winmm.OrignalmidiStreamOut] } }
__declspec(naked) void FakemidiStreamPause() { _asm { jmp[winmm.OrignalmidiStreamPause] } }
__declspec(naked) void FakemidiStreamPosition() { _asm { jmp[winmm.OrignalmidiStreamPosition] } }
__declspec(naked) void FakemidiStreamProperty() { _asm { jmp[winmm.OrignalmidiStreamProperty] } }
__declspec(naked) void FakemidiStreamRestart() { _asm { jmp[winmm.OrignalmidiStreamRestart] } }
__declspec(naked) void FakemidiStreamStop() { _asm { jmp[winmm.OrignalmidiStreamStop] } }
__declspec(naked) void FakemixerClose() { _asm { jmp[winmm.OrignalmixerClose] } }
__declspec(naked) void FakemixerGetControlDetailsA() { _asm { jmp[winmm.OrignalmixerGetControlDetailsA] } }
__declspec(naked) void FakemixerGetControlDetailsW() { _asm { jmp[winmm.OrignalmixerGetControlDetailsW] } }
__declspec(naked) void FakemixerGetDevCapsA() { _asm { jmp[winmm.OrignalmixerGetDevCapsA] } }
__declspec(naked) void FakemixerGetDevCapsW() { _asm { jmp[winmm.OrignalmixerGetDevCapsW] } }
__declspec(naked) void FakemixerGetID() { _asm { jmp[winmm.OrignalmixerGetID] } }
__declspec(naked) void FakemixerGetLineControlsA() { _asm { jmp[winmm.OrignalmixerGetLineControlsA] } }
__declspec(naked) void FakemixerGetLineControlsW() { _asm { jmp[winmm.OrignalmixerGetLineControlsW] } }
__declspec(naked) void FakemixerGetLineInfoA() { _asm { jmp[winmm.OrignalmixerGetLineInfoA] } }
__declspec(naked) void FakemixerGetLineInfoW() { _asm { jmp[winmm.OrignalmixerGetLineInfoW] } }
__declspec(naked) void FakemixerGetNumDevs() { _asm { jmp[winmm.OrignalmixerGetNumDevs] } }
__declspec(naked) void FakemixerMessage() { _asm { jmp[winmm.OrignalmixerMessage] } }
__declspec(naked) void FakemixerOpen() { _asm { jmp[winmm.OrignalmixerOpen] } }
__declspec(naked) void FakemixerSetControlDetails() { _asm { jmp[winmm.OrignalmixerSetControlDetails] } }
__declspec(naked) void FakemmDrvInstall() { _asm { jmp[winmm.OrignalmmDrvInstall] } }
__declspec(naked) void FakemmGetCurrentTask() { _asm { jmp[winmm.OrignalmmGetCurrentTask] } }
__declspec(naked) void FakemmTaskBlock() { _asm { jmp[winmm.OrignalmmTaskBlock] } }
__declspec(naked) void FakemmTaskCreate() { _asm { jmp[winmm.OrignalmmTaskCreate] } }
__declspec(naked) void FakemmTaskSignal() { _asm { jmp[winmm.OrignalmmTaskSignal] } }
__declspec(naked) void FakemmTaskYield() { _asm { jmp[winmm.OrignalmmTaskYield] } }
__declspec(naked) void FakemmioAdvance() { _asm { jmp[winmm.OrignalmmioAdvance] } }
__declspec(naked) void FakemmioAscend() { _asm { jmp[winmm.OrignalmmioAscend] } }
__declspec(naked) void FakemmioClose() { _asm { jmp[winmm.OrignalmmioClose] } }
__declspec(naked) void FakemmioCreateChunk() { _asm { jmp[winmm.OrignalmmioCreateChunk] } }
__declspec(naked) void FakemmioDescend() { _asm { jmp[winmm.OrignalmmioDescend] } }
__declspec(naked) void FakemmioFlush() { _asm { jmp[winmm.OrignalmmioFlush] } }
__declspec(naked) void FakemmioGetInfo() { _asm { jmp[winmm.OrignalmmioGetInfo] } }
__declspec(naked) void FakemmioInstallIOProcA() { _asm { jmp[winmm.OrignalmmioInstallIOProcA] } }
__declspec(naked) void FakemmioInstallIOProcW() { _asm { jmp[winmm.OrignalmmioInstallIOProcW] } }
__declspec(naked) void FakemmioOpenA() { _asm { jmp[winmm.OrignalmmioOpenA] } }
__declspec(naked) void FakemmioOpenW() { _asm { jmp[winmm.OrignalmmioOpenW] } }
__declspec(naked) void FakemmioRead() { _asm { jmp[winmm.OrignalmmioRead] } }
__declspec(naked) void FakemmioRenameA() { _asm { jmp[winmm.OrignalmmioRenameA] } }
__declspec(naked) void FakemmioRenameW() { _asm { jmp[winmm.OrignalmmioRenameW] } }
__declspec(naked) void FakemmioSeek() { _asm { jmp[winmm.OrignalmmioSeek] } }
__declspec(naked) void FakemmioSendMessage() { _asm { jmp[winmm.OrignalmmioSendMessage] } }
__declspec(naked) void FakemmioSetBuffer() { _asm { jmp[winmm.OrignalmmioSetBuffer] } }
__declspec(naked) void FakemmioSetInfo() { _asm { jmp[winmm.OrignalmmioSetInfo] } }
__declspec(naked) void FakemmioStringToFOURCCA() { _asm { jmp[winmm.OrignalmmioStringToFOURCCA] } }
__declspec(naked) void FakemmioStringToFOURCCW() { _asm { jmp[winmm.OrignalmmioStringToFOURCCW] } }
__declspec(naked) void FakemmioWrite() { _asm { jmp[winmm.OrignalmmioWrite] } }
__declspec(naked) void FakemmsystemGetVersion() { _asm { jmp[winmm.OrignalmmsystemGetVersion] } }
__declspec(naked) void Fakemod32Message() { _asm { jmp[winmm.Orignalmod32Message] } }
__declspec(naked) void Fakemxd32Message() { _asm { jmp[winmm.Orignalmxd32Message] } }
__declspec(naked) void FakesndPlaySoundA() { _asm { jmp[winmm.OrignalsndPlaySoundA] } }
__declspec(naked) void FakesndPlaySoundW() { _asm { jmp[winmm.OrignalsndPlaySoundW] } }
__declspec(naked) void Faketid32Message() { _asm { jmp[winmm.Orignaltid32Message] } }
__declspec(naked) void FaketimeBeginPeriod() { _asm { jmp[winmm.OrignaltimeBeginPeriod] } }
__declspec(naked) void FaketimeEndPeriod() { _asm { jmp[winmm.OrignaltimeEndPeriod] } }
__declspec(naked) void FaketimeGetDevCaps() { _asm { jmp[winmm.OrignaltimeGetDevCaps] } }
__declspec(naked) void FaketimeGetSystemTime() { _asm { jmp[winmm.OrignaltimeGetSystemTime] } }
__declspec(naked) void FaketimeGetTime() { _asm { jmp[winmm.OrignaltimeGetTime] } }
__declspec(naked) void FaketimeKillEvent() { _asm { jmp[winmm.OrignaltimeKillEvent] } }
__declspec(naked) void FaketimeSetEvent() { _asm { jmp[winmm.OrignaltimeSetEvent] } }
__declspec(naked) void FakewaveInAddBuffer() { _asm { jmp[winmm.OrignalwaveInAddBuffer] } }
__declspec(naked) void FakewaveInClose() { _asm { jmp[winmm.OrignalwaveInClose] } }
__declspec(naked) void FakewaveInGetDevCapsA() { _asm { jmp[winmm.OrignalwaveInGetDevCapsA] } }
__declspec(naked) void FakewaveInGetDevCapsW() { _asm { jmp[winmm.OrignalwaveInGetDevCapsW] } }
__declspec(naked) void FakewaveInGetErrorTextA() { _asm { jmp[winmm.OrignalwaveInGetErrorTextA] } }
__declspec(naked) void FakewaveInGetErrorTextW() { _asm { jmp[winmm.OrignalwaveInGetErrorTextW] } }
__declspec(naked) void FakewaveInGetID() { _asm { jmp[winmm.OrignalwaveInGetID] } }
__declspec(naked) void FakewaveInGetNumDevs() { _asm { jmp[winmm.OrignalwaveInGetNumDevs] } }
__declspec(naked) void FakewaveInGetPosition() { _asm { jmp[winmm.OrignalwaveInGetPosition] } }
__declspec(naked) void FakewaveInMessage() { _asm { jmp[winmm.OrignalwaveInMessage] } }
__declspec(naked) void FakewaveInOpen() { _asm { jmp[winmm.OrignalwaveInOpen] } }
__declspec(naked) void FakewaveInPrepareHeader() { _asm { jmp[winmm.OrignalwaveInPrepareHeader] } }
__declspec(naked) void FakewaveInReset() { _asm { jmp[winmm.OrignalwaveInReset] } }
__declspec(naked) void FakewaveInStart() { _asm { jmp[winmm.OrignalwaveInStart] } }
__declspec(naked) void FakewaveInStop() { _asm { jmp[winmm.OrignalwaveInStop] } }
__declspec(naked) void FakewaveInUnprepareHeader() { _asm { jmp[winmm.OrignalwaveInUnprepareHeader] } }
__declspec(naked) void FakewaveOutBreakLoop() { _asm { jmp[winmm.OrignalwaveOutBreakLoop] } }
__declspec(naked) void FakewaveOutClose() { _asm { jmp[winmm.OrignalwaveOutClose] } }
__declspec(naked) void FakewaveOutGetDevCapsA() { _asm { jmp[winmm.OrignalwaveOutGetDevCapsA] } }
__declspec(naked) void FakewaveOutGetDevCapsW() { _asm { jmp[winmm.OrignalwaveOutGetDevCapsW] } }
__declspec(naked) void FakewaveOutGetErrorTextA() { _asm { jmp[winmm.OrignalwaveOutGetErrorTextA] } }
__declspec(naked) void FakewaveOutGetErrorTextW() { _asm { jmp[winmm.OrignalwaveOutGetErrorTextW] } }
__declspec(naked) void FakewaveOutGetID() { _asm { jmp[winmm.OrignalwaveOutGetID] } }
__declspec(naked) void FakewaveOutGetNumDevs() { _asm { jmp[winmm.OrignalwaveOutGetNumDevs] } }
__declspec(naked) void FakewaveOutGetPitch() { _asm { jmp[winmm.OrignalwaveOutGetPitch] } }
__declspec(naked) void FakewaveOutGetPlaybackRate() { _asm { jmp[winmm.OrignalwaveOutGetPlaybackRate] } }
__declspec(naked) void FakewaveOutGetPosition() { _asm { jmp[winmm.OrignalwaveOutGetPosition] } }
__declspec(naked) void FakewaveOutGetVolume() { _asm { jmp[winmm.OrignalwaveOutGetVolume] } }
__declspec(naked) void FakewaveOutMessage() { _asm { jmp[winmm.OrignalwaveOutMessage] } }
__declspec(naked) void FakewaveOutOpen() { _asm { jmp[winmm.OrignalwaveOutOpen] } }
__declspec(naked) void FakewaveOutPause() { _asm { jmp[winmm.OrignalwaveOutPause] } }
__declspec(naked) void FakewaveOutPrepareHeader() { _asm { jmp[winmm.OrignalwaveOutPrepareHeader] } }
__declspec(naked) void FakewaveOutReset() { _asm { jmp[winmm.OrignalwaveOutReset] } }
__declspec(naked) void FakewaveOutRestart() { _asm { jmp[winmm.OrignalwaveOutRestart] } }
__declspec(naked) void FakewaveOutSetPitch() { _asm { jmp[winmm.OrignalwaveOutSetPitch] } }
__declspec(naked) void FakewaveOutSetPlaybackRate() { _asm { jmp[winmm.OrignalwaveOutSetPlaybackRate] } }
__declspec(naked) void FakewaveOutSetVolume() { _asm { jmp[winmm.OrignalwaveOutSetVolume] } }
__declspec(naked) void FakewaveOutUnprepareHeader() { _asm { jmp[winmm.OrignalwaveOutUnprepareHeader] } }
__declspec(naked) void FakewaveOutWrite() { _asm { jmp[winmm.OrignalwaveOutWrite] } }
__declspec(naked) void Fakewid32Message() { _asm { jmp[winmm.Orignalwid32Message] } }
__declspec(naked) void Fakewod32Message() { _asm { jmp[winmm.Orignalwod32Message] } }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	wchar_t path[MAX_PATH];
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		if (!winmm.dll) {
			memcpy(path + GetSystemDirectory(path, MAX_PATH - 11), L"\\winmm.dll", 22);
			winmm.dll = LoadLibraryW(path);
			if (!winmm.dll)
			{
				MessageBox(0, L"Cannot load original winmm.dll library", L"LanguageBarrier exception", MB_ICONERROR);
				ExitProcess(0);
			}
			winmm.OrignalCloseDriver = GetProcAddress(winmm.dll, "CloseDriver");
			winmm.OrignalDefDriverProc = GetProcAddress(winmm.dll, "DefDriverProc");
			winmm.OrignalDriverCallback = GetProcAddress(winmm.dll, "DriverCallback");
			winmm.OrignalDrvGetModuleHandle = GetProcAddress(winmm.dll, "DrvGetModuleHandle");
			winmm.OrignalGetDriverModuleHandle = GetProcAddress(winmm.dll, "GetDriverModuleHandle");
			winmm.OrignalNotifyCallbackData = GetProcAddress(winmm.dll, "NotifyCallbackData");
			winmm.OrignalOpenDriver = GetProcAddress(winmm.dll, "OpenDriver");
			winmm.OrignalPlaySound = GetProcAddress(winmm.dll, "PlaySound");
			winmm.OrignalPlaySoundA = GetProcAddress(winmm.dll, "PlaySoundA");
			winmm.OrignalPlaySoundW = GetProcAddress(winmm.dll, "PlaySoundW");
			winmm.OrignalSendDriverMessage = GetProcAddress(winmm.dll, "SendDriverMessage");
			winmm.OrignalWOW32DriverCallback = GetProcAddress(winmm.dll, "WOW32DriverCallback");
			winmm.OrignalWOW32ResolveMultiMediaHandle = GetProcAddress(winmm.dll, "WOW32ResolveMultiMediaHandle");
			winmm.OrignalWOWAppExit = GetProcAddress(winmm.dll, "WOWAppExit");
			winmm.Orignalaux32Message = GetProcAddress(winmm.dll, "aux32Message");
			winmm.OrignalauxGetDevCapsA = GetProcAddress(winmm.dll, "auxGetDevCapsA");
			winmm.OrignalauxGetDevCapsW = GetProcAddress(winmm.dll, "auxGetDevCapsW");
			winmm.OrignalauxGetNumDevs = GetProcAddress(winmm.dll, "auxGetNumDevs");
			winmm.OrignalauxGetVolume = GetProcAddress(winmm.dll, "auxGetVolume");
			winmm.OrignalauxOutMessage = GetProcAddress(winmm.dll, "auxOutMessage");
			winmm.OrignalauxSetVolume = GetProcAddress(winmm.dll, "auxSetVolume");
			winmm.Orignaljoy32Message = GetProcAddress(winmm.dll, "joy32Message");
			winmm.OrignaljoyConfigChanged = GetProcAddress(winmm.dll, "joyConfigChanged");
			winmm.OrignaljoyGetDevCapsA = GetProcAddress(winmm.dll, "joyGetDevCapsA");
			winmm.OrignaljoyGetDevCapsW = GetProcAddress(winmm.dll, "joyGetDevCapsW");
			winmm.OrignaljoyGetNumDevs = GetProcAddress(winmm.dll, "joyGetNumDevs");
			winmm.OrignaljoyGetPos = GetProcAddress(winmm.dll, "joyGetPos");
			winmm.OrignaljoyGetPosEx = GetProcAddress(winmm.dll, "joyGetPosEx");
			winmm.OrignaljoyGetThreshold = GetProcAddress(winmm.dll, "joyGetThreshold");
			winmm.OrignaljoyReleaseCapture = GetProcAddress(winmm.dll, "joyReleaseCapture");
			winmm.OrignaljoySetCapture = GetProcAddress(winmm.dll, "joySetCapture");
			winmm.OrignaljoySetThreshold = GetProcAddress(winmm.dll, "joySetThreshold");
			winmm.Orignalmci32Message = GetProcAddress(winmm.dll, "mci32Message");
			winmm.OrignalmciDriverNotify = GetProcAddress(winmm.dll, "mciDriverNotify");
			winmm.OrignalmciDriverYield = GetProcAddress(winmm.dll, "mciDriverYield");
			winmm.OrignalmciExecute = GetProcAddress(winmm.dll, "mciExecute");
			winmm.OrignalmciFreeCommandResource = GetProcAddress(winmm.dll, "mciFreeCommandResource");
			winmm.OrignalmciGetCreatorTask = GetProcAddress(winmm.dll, "mciGetCreatorTask");
			winmm.OrignalmciGetDeviceIDA = GetProcAddress(winmm.dll, "mciGetDeviceIDA");
			winmm.OrignalmciGetDeviceIDFromElementIDA = GetProcAddress(winmm.dll, "mciGetDeviceIDFromElementIDA");
			winmm.OrignalmciGetDeviceIDFromElementIDW = GetProcAddress(winmm.dll, "mciGetDeviceIDFromElementIDW");
			winmm.OrignalmciGetDeviceIDW = GetProcAddress(winmm.dll, "mciGetDeviceIDW");
			winmm.OrignalmciGetDriverData = GetProcAddress(winmm.dll, "mciGetDriverData");
			winmm.OrignalmciGetErrorStringA = GetProcAddress(winmm.dll, "mciGetErrorStringA");
			winmm.OrignalmciGetErrorStringW = GetProcAddress(winmm.dll, "mciGetErrorStringW");
			winmm.OrignalmciGetYieldProc = GetProcAddress(winmm.dll, "mciGetYieldProc");
			winmm.OrignalmciLoadCommandResource = GetProcAddress(winmm.dll, "mciLoadCommandResource");
			winmm.OrignalmciSendCommandA = GetProcAddress(winmm.dll, "mciSendCommandA");
			winmm.OrignalmciSendCommandW = GetProcAddress(winmm.dll, "mciSendCommandW");
			winmm.OrignalmciSendStringA = GetProcAddress(winmm.dll, "mciSendStringA");
			winmm.OrignalmciSendStringW = GetProcAddress(winmm.dll, "mciSendStringW");
			winmm.OrignalmciSetDriverData = GetProcAddress(winmm.dll, "mciSetDriverData");
			winmm.OrignalmciSetYieldProc = GetProcAddress(winmm.dll, "mciSetYieldProc");
			winmm.Orignalmid32Message = GetProcAddress(winmm.dll, "mid32Message");
			winmm.OrignalmidiConnect = GetProcAddress(winmm.dll, "midiConnect");
			winmm.OrignalmidiDisconnect = GetProcAddress(winmm.dll, "midiDisconnect");
			winmm.OrignalmidiInAddBuffer = GetProcAddress(winmm.dll, "midiInAddBuffer");
			winmm.OrignalmidiInClose = GetProcAddress(winmm.dll, "midiInClose");
			winmm.OrignalmidiInGetDevCapsA = GetProcAddress(winmm.dll, "midiInGetDevCapsA");
			winmm.OrignalmidiInGetDevCapsW = GetProcAddress(winmm.dll, "midiInGetDevCapsW");
			winmm.OrignalmidiInGetErrorTextA = GetProcAddress(winmm.dll, "midiInGetErrorTextA");
			winmm.OrignalmidiInGetErrorTextW = GetProcAddress(winmm.dll, "midiInGetErrorTextW");
			winmm.OrignalmidiInGetID = GetProcAddress(winmm.dll, "midiInGetID");
			winmm.OrignalmidiInGetNumDevs = GetProcAddress(winmm.dll, "midiInGetNumDevs");
			winmm.OrignalmidiInMessage = GetProcAddress(winmm.dll, "midiInMessage");
			winmm.OrignalmidiInOpen = GetProcAddress(winmm.dll, "midiInOpen");
			winmm.OrignalmidiInPrepareHeader = GetProcAddress(winmm.dll, "midiInPrepareHeader");
			winmm.OrignalmidiInReset = GetProcAddress(winmm.dll, "midiInReset");
			winmm.OrignalmidiInStart = GetProcAddress(winmm.dll, "midiInStart");
			winmm.OrignalmidiInStop = GetProcAddress(winmm.dll, "midiInStop");
			winmm.OrignalmidiInUnprepareHeader = GetProcAddress(winmm.dll, "midiInUnprepareHeader");
			winmm.OrignalmidiOutCacheDrumPatches = GetProcAddress(winmm.dll, "midiOutCacheDrumPatches");
			winmm.OrignalmidiOutCachePatches = GetProcAddress(winmm.dll, "midiOutCachePatches");
			winmm.OrignalmidiOutClose = GetProcAddress(winmm.dll, "midiOutClose");
			winmm.OrignalmidiOutGetDevCapsA = GetProcAddress(winmm.dll, "midiOutGetDevCapsA");
			winmm.OrignalmidiOutGetDevCapsW = GetProcAddress(winmm.dll, "midiOutGetDevCapsW");
			winmm.OrignalmidiOutGetErrorTextA = GetProcAddress(winmm.dll, "midiOutGetErrorTextA");
			winmm.OrignalmidiOutGetErrorTextW = GetProcAddress(winmm.dll, "midiOutGetErrorTextW");
			winmm.OrignalmidiOutGetID = GetProcAddress(winmm.dll, "midiOutGetID");
			winmm.OrignalmidiOutGetNumDevs = GetProcAddress(winmm.dll, "midiOutGetNumDevs");
			winmm.OrignalmidiOutGetVolume = GetProcAddress(winmm.dll, "midiOutGetVolume");
			winmm.OrignalmidiOutLongMsg = GetProcAddress(winmm.dll, "midiOutLongMsg");
			winmm.OrignalmidiOutMessage = GetProcAddress(winmm.dll, "midiOutMessage");
			winmm.OrignalmidiOutOpen = GetProcAddress(winmm.dll, "midiOutOpen");
			winmm.OrignalmidiOutPrepareHeader = GetProcAddress(winmm.dll, "midiOutPrepareHeader");
			winmm.OrignalmidiOutReset = GetProcAddress(winmm.dll, "midiOutReset");
			winmm.OrignalmidiOutSetVolume = GetProcAddress(winmm.dll, "midiOutSetVolume");
			winmm.OrignalmidiOutShortMsg = GetProcAddress(winmm.dll, "midiOutShortMsg");
			winmm.OrignalmidiOutUnprepareHeader = GetProcAddress(winmm.dll, "midiOutUnprepareHeader");
			winmm.OrignalmidiStreamClose = GetProcAddress(winmm.dll, "midiStreamClose");
			winmm.OrignalmidiStreamOpen = GetProcAddress(winmm.dll, "midiStreamOpen");
			winmm.OrignalmidiStreamOut = GetProcAddress(winmm.dll, "midiStreamOut");
			winmm.OrignalmidiStreamPause = GetProcAddress(winmm.dll, "midiStreamPause");
			winmm.OrignalmidiStreamPosition = GetProcAddress(winmm.dll, "midiStreamPosition");
			winmm.OrignalmidiStreamProperty = GetProcAddress(winmm.dll, "midiStreamProperty");
			winmm.OrignalmidiStreamRestart = GetProcAddress(winmm.dll, "midiStreamRestart");
			winmm.OrignalmidiStreamStop = GetProcAddress(winmm.dll, "midiStreamStop");
			winmm.OrignalmixerClose = GetProcAddress(winmm.dll, "mixerClose");
			winmm.OrignalmixerGetControlDetailsA = GetProcAddress(winmm.dll, "mixerGetControlDetailsA");
			winmm.OrignalmixerGetControlDetailsW = GetProcAddress(winmm.dll, "mixerGetControlDetailsW");
			winmm.OrignalmixerGetDevCapsA = GetProcAddress(winmm.dll, "mixerGetDevCapsA");
			winmm.OrignalmixerGetDevCapsW = GetProcAddress(winmm.dll, "mixerGetDevCapsW");
			winmm.OrignalmixerGetID = GetProcAddress(winmm.dll, "mixerGetID");
			winmm.OrignalmixerGetLineControlsA = GetProcAddress(winmm.dll, "mixerGetLineControlsA");
			winmm.OrignalmixerGetLineControlsW = GetProcAddress(winmm.dll, "mixerGetLineControlsW");
			winmm.OrignalmixerGetLineInfoA = GetProcAddress(winmm.dll, "mixerGetLineInfoA");
			winmm.OrignalmixerGetLineInfoW = GetProcAddress(winmm.dll, "mixerGetLineInfoW");
			winmm.OrignalmixerGetNumDevs = GetProcAddress(winmm.dll, "mixerGetNumDevs");
			winmm.OrignalmixerMessage = GetProcAddress(winmm.dll, "mixerMessage");
			winmm.OrignalmixerOpen = GetProcAddress(winmm.dll, "mixerOpen");
			winmm.OrignalmixerSetControlDetails = GetProcAddress(winmm.dll, "mixerSetControlDetails");
			winmm.OrignalmmDrvInstall = GetProcAddress(winmm.dll, "mmDrvInstall");
			winmm.OrignalmmGetCurrentTask = GetProcAddress(winmm.dll, "mmGetCurrentTask");
			winmm.OrignalmmTaskBlock = GetProcAddress(winmm.dll, "mmTaskBlock");
			winmm.OrignalmmTaskCreate = GetProcAddress(winmm.dll, "mmTaskCreate");
			winmm.OrignalmmTaskSignal = GetProcAddress(winmm.dll, "mmTaskSignal");
			winmm.OrignalmmTaskYield = GetProcAddress(winmm.dll, "mmTaskYield");
			winmm.OrignalmmioAdvance = GetProcAddress(winmm.dll, "mmioAdvance");
			winmm.OrignalmmioAscend = GetProcAddress(winmm.dll, "mmioAscend");
			winmm.OrignalmmioClose = GetProcAddress(winmm.dll, "mmioClose");
			winmm.OrignalmmioCreateChunk = GetProcAddress(winmm.dll, "mmioCreateChunk");
			winmm.OrignalmmioDescend = GetProcAddress(winmm.dll, "mmioDescend");
			winmm.OrignalmmioFlush = GetProcAddress(winmm.dll, "mmioFlush");
			winmm.OrignalmmioGetInfo = GetProcAddress(winmm.dll, "mmioGetInfo");
			winmm.OrignalmmioInstallIOProcA = GetProcAddress(winmm.dll, "mmioInstallIOProcA");
			winmm.OrignalmmioInstallIOProcW = GetProcAddress(winmm.dll, "mmioInstallIOProcW");
			winmm.OrignalmmioOpenA = GetProcAddress(winmm.dll, "mmioOpenA");
			winmm.OrignalmmioOpenW = GetProcAddress(winmm.dll, "mmioOpenW");
			winmm.OrignalmmioRead = GetProcAddress(winmm.dll, "mmioRead");
			winmm.OrignalmmioRenameA = GetProcAddress(winmm.dll, "mmioRenameA");
			winmm.OrignalmmioRenameW = GetProcAddress(winmm.dll, "mmioRenameW");
			winmm.OrignalmmioSeek = GetProcAddress(winmm.dll, "mmioSeek");
			winmm.OrignalmmioSendMessage = GetProcAddress(winmm.dll, "mmioSendMessage");
			winmm.OrignalmmioSetBuffer = GetProcAddress(winmm.dll, "mmioSetBuffer");
			winmm.OrignalmmioSetInfo = GetProcAddress(winmm.dll, "mmioSetInfo");
			winmm.OrignalmmioStringToFOURCCA = GetProcAddress(winmm.dll, "mmioStringToFOURCCA");
			winmm.OrignalmmioStringToFOURCCW = GetProcAddress(winmm.dll, "mmioStringToFOURCCW");
			winmm.OrignalmmioWrite = GetProcAddress(winmm.dll, "mmioWrite");
			winmm.OrignalmmsystemGetVersion = GetProcAddress(winmm.dll, "mmsystemGetVersion");
			winmm.Orignalmod32Message = GetProcAddress(winmm.dll, "mod32Message");
			winmm.Orignalmxd32Message = GetProcAddress(winmm.dll, "mxd32Message");
			winmm.OrignalsndPlaySoundA = GetProcAddress(winmm.dll, "sndPlaySoundA");
			winmm.OrignalsndPlaySoundW = GetProcAddress(winmm.dll, "sndPlaySoundW");
			winmm.Orignaltid32Message = GetProcAddress(winmm.dll, "tid32Message");
			winmm.OrignaltimeBeginPeriod = GetProcAddress(winmm.dll, "timeBeginPeriod");
			winmm.OrignaltimeEndPeriod = GetProcAddress(winmm.dll, "timeEndPeriod");
			winmm.OrignaltimeGetDevCaps = GetProcAddress(winmm.dll, "timeGetDevCaps");
			winmm.OrignaltimeGetSystemTime = GetProcAddress(winmm.dll, "timeGetSystemTime");
			winmm.OrignaltimeGetTime = GetProcAddress(winmm.dll, "timeGetTime");
			winmm.OrignaltimeKillEvent = GetProcAddress(winmm.dll, "timeKillEvent");
			winmm.OrignaltimeSetEvent = GetProcAddress(winmm.dll, "timeSetEvent");
			winmm.OrignalwaveInAddBuffer = GetProcAddress(winmm.dll, "waveInAddBuffer");
			winmm.OrignalwaveInClose = GetProcAddress(winmm.dll, "waveInClose");
			winmm.OrignalwaveInGetDevCapsA = GetProcAddress(winmm.dll, "waveInGetDevCapsA");
			winmm.OrignalwaveInGetDevCapsW = GetProcAddress(winmm.dll, "waveInGetDevCapsW");
			winmm.OrignalwaveInGetErrorTextA = GetProcAddress(winmm.dll, "waveInGetErrorTextA");
			winmm.OrignalwaveInGetErrorTextW = GetProcAddress(winmm.dll, "waveInGetErrorTextW");
			winmm.OrignalwaveInGetID = GetProcAddress(winmm.dll, "waveInGetID");
			winmm.OrignalwaveInGetNumDevs = GetProcAddress(winmm.dll, "waveInGetNumDevs");
			winmm.OrignalwaveInGetPosition = GetProcAddress(winmm.dll, "waveInGetPosition");
			winmm.OrignalwaveInMessage = GetProcAddress(winmm.dll, "waveInMessage");
			winmm.OrignalwaveInOpen = GetProcAddress(winmm.dll, "waveInOpen");
			winmm.OrignalwaveInPrepareHeader = GetProcAddress(winmm.dll, "waveInPrepareHeader");
			winmm.OrignalwaveInReset = GetProcAddress(winmm.dll, "waveInReset");
			winmm.OrignalwaveInStart = GetProcAddress(winmm.dll, "waveInStart");
			winmm.OrignalwaveInStop = GetProcAddress(winmm.dll, "waveInStop");
			winmm.OrignalwaveInUnprepareHeader = GetProcAddress(winmm.dll, "waveInUnprepareHeader");
			winmm.OrignalwaveOutBreakLoop = GetProcAddress(winmm.dll, "waveOutBreakLoop");
			winmm.OrignalwaveOutClose = GetProcAddress(winmm.dll, "waveOutClose");
			winmm.OrignalwaveOutGetDevCapsA = GetProcAddress(winmm.dll, "waveOutGetDevCapsA");
			winmm.OrignalwaveOutGetDevCapsW = GetProcAddress(winmm.dll, "waveOutGetDevCapsW");
			winmm.OrignalwaveOutGetErrorTextA = GetProcAddress(winmm.dll, "waveOutGetErrorTextA");
			winmm.OrignalwaveOutGetErrorTextW = GetProcAddress(winmm.dll, "waveOutGetErrorTextW");
			winmm.OrignalwaveOutGetID = GetProcAddress(winmm.dll, "waveOutGetID");
			winmm.OrignalwaveOutGetNumDevs = GetProcAddress(winmm.dll, "waveOutGetNumDevs");
			winmm.OrignalwaveOutGetPitch = GetProcAddress(winmm.dll, "waveOutGetPitch");
			winmm.OrignalwaveOutGetPlaybackRate = GetProcAddress(winmm.dll, "waveOutGetPlaybackRate");
			winmm.OrignalwaveOutGetPosition = GetProcAddress(winmm.dll, "waveOutGetPosition");
			winmm.OrignalwaveOutGetVolume = GetProcAddress(winmm.dll, "waveOutGetVolume");
			winmm.OrignalwaveOutMessage = GetProcAddress(winmm.dll, "waveOutMessage");
			winmm.OrignalwaveOutOpen = GetProcAddress(winmm.dll, "waveOutOpen");
			winmm.OrignalwaveOutPause = GetProcAddress(winmm.dll, "waveOutPause");
			winmm.OrignalwaveOutPrepareHeader = GetProcAddress(winmm.dll, "waveOutPrepareHeader");
			winmm.OrignalwaveOutReset = GetProcAddress(winmm.dll, "waveOutReset");
			winmm.OrignalwaveOutRestart = GetProcAddress(winmm.dll, "waveOutRestart");
			winmm.OrignalwaveOutSetPitch = GetProcAddress(winmm.dll, "waveOutSetPitch");
			winmm.OrignalwaveOutSetPlaybackRate = GetProcAddress(winmm.dll, "waveOutSetPlaybackRate");
			winmm.OrignalwaveOutSetVolume = GetProcAddress(winmm.dll, "waveOutSetVolume");
			winmm.OrignalwaveOutUnprepareHeader = GetProcAddress(winmm.dll, "waveOutUnprepareHeader");
			winmm.OrignalwaveOutWrite = GetProcAddress(winmm.dll, "waveOutWrite");
			winmm.Orignalwid32Message = GetProcAddress(winmm.dll, "wid32Message");
			winmm.Orignalwod32Message = GetProcAddress(winmm.dll, "wod32Message");
		}

		if (!lb::IsInitialised) {
			try {
				lb::LanguageBarrierInit();
			}
			catch (std::exception& e) {
				// if we're here, next attempts to initialize will probably
				// throw the same exception, no sense to retry initialization
				lb::IsInitialised = true;
				MessageBoxA(NULL, e.what(), "LanguageBarrier exception", MB_ICONSTOP);
			}
		}
		return TRUE;


		break;
	}
	case DLL_PROCESS_DETACH:
	{
		FreeLibrary(winmm.dll);
		winmm.dll = NULL;
	}
	break;
	}
	return TRUE;
}
