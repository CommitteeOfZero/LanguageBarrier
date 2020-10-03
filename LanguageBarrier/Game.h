#ifndef __GAME_H__
#define __GAME_H__

#include <cstdint>
#include "LanguageBarrier.h"
#include <d3d11.h>

#ifndef GAME_H_IMPORT
#define GAME_H_IMPORT extern
#endif

#pragma pack(push, 1)
struct mpkObject {
  char field_0;
  char filename[11];
  int fileCount;
  char gap10[919];
  char field_C;
};

struct mgsVFSObject {
	char gap0[8];
	char archiveName[16];
	char gap1[10664];
};

struct mgsFileLoader {
	uint32_t unk01;
	uint32_t unk02;
	uint32_t fileId;
	uint32_t loadMode;
	char fileName[64];
	char gap0[228];
	mgsVFSObject* vfsObject;
};

struct MgsD3D11State
{
	uint8_t gap0[12];
	ID3D11Device* pid3d11deviceC;
	IDXGISwapChain* pidxgiswapchain10;
	D3D_FEATURE_LEVEL d3d_feature_level14;
	ID3D11DeviceContext* pid3d11devicecontext18;
};
extern MgsD3D11State* gameExePMgsD3D11State;


struct __declspec(align(4)) SurfaceStruct
{
	uint8_t gap_0[4];
	char field_4;
	uint8_t gap_5;
	uint16_t field_6;
	uint8_t gap_8[2];
	char field_A;
	uint8_t gap_B;
	uint32_t field_C;
	char field_10;
	uint8_t gap_11[3];
	uint32_t field_14;
	signed int field_18;
	uint32_t field_1C;
	char field_20;
	uint8_t gap_21[3];
	signed int field_24;
	signed __int16 field_28;
	uint8_t gap_2A[2];
	signed int field_2C;
	signed int field_30;
	signed int field_34;
	UINT field_38;
	UINT field_3C;
	int field_40;
	int field_44;
	float field_48;
	float field_4C;
	signed int field_50;
	uint8_t gap_54[80];
	char field_A4;
	uint8_t gap_A5[15];
	ID3D11Texture2D* texPtr[4];
	uint8_t gap_C4[32];
	ID3D11ShaderResourceView* shaderRscView;
	uint32_t field_E8;
	signed int field_EC;
	signed int field_F0;
	uint8_t gap_F4[64];
	uint32_t field_134;
	uint32_t field_138;
	uint32_t field_13C;
};


#pragma pack(pop)

typedef BOOL(__cdecl* GetFlagProc)(int flagId);
typedef void(__cdecl* SetFlagProc)(int flagId, BOOL value);
typedef BOOL(__cdecl* ChkViewDicProc)(int tipId, int unknown);
extern int* gameExeScrWork;
extern SurfaceStruct* surfaceArray;

namespace lb {
// DEFAULT VALUES!
LB_GLOBAL uint32_t BGM_CLEAR;
LB_GLOBAL uint8_t MPK_ID_SCRIPT_MPK;
LB_GLOBAL uint8_t MPK_ID_BGM_MPK;
LB_GLOBAL uint8_t AUDIO_PLAYER_ID_BGM1;
LB_GLOBAL uint32_t C0DATA_MOUNT_ID;

GAME_H_IMPORT GetFlagProc gameExeGetFlag;
GAME_H_IMPORT SetFlagProc gameExeSetFlag;
GAME_H_IMPORT ChkViewDicProc gameExeChkViewDic;



void gameInit();
void gameLoadTexture(uint16_t textureId, void* buffer, size_t sz);
void __cdecl setAreaParamsHook(int areaId, short params[24]);

mpkObject* gameMountMpk(char const* mountpoint, char const* directory,
                        char const* filename);
void gameSetBgm(uint32_t fileId, bool shouldLoop);
uint32_t gameGetBgm();
bool gameGetBgmShouldLoop();
void gameSetBgmShouldPlay(bool shouldPlay);
bool gameGetBgmShouldPlay();
void gameSetBgmPaused(bool paused);
bool gameGetBgmIsPlaying();
}  // namespace lb

#endif  // !__GAME_H__
