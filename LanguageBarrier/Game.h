#ifndef __GAME_H__
#define __GAME_H__

#include <cstdint>
#include "LanguageBarrier.h"

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
#pragma pack(pop)

typedef BOOL(__cdecl* GetFlagProc)(int flagId);
typedef void(__cdecl* SetFlagProc)(int flagId, BOOL value);
typedef BOOL(__cdecl* ChkViewDicProc)(int tipId, int unknown);

namespace lb {
// DEFAULT VALUES!
LB_GLOBAL uint32_t BGM_CLEAR;
LB_GLOBAL uint8_t MPK_ID_SCRIPT_MPK;
LB_GLOBAL uint8_t MPK_ID_BGM_MPK;
LB_GLOBAL uint8_t AUDIO_PLAYER_ID_BGM1;

GAME_H_IMPORT GetFlagProc gameExeGetFlag;
GAME_H_IMPORT SetFlagProc gameExeSetFlag;
GAME_H_IMPORT ChkViewDicProc gameExeChkViewDic;

void gameInit();
void gameLoadTexture(uint16_t textureId, void *buffer, size_t sz);
mpkObject *gameMountMpk(char *mountpoint, char *directory, char *filename);
void gameSetBgm(uint32_t fileId, bool shouldLoop);
uint32_t gameGetBgm();
bool gameGetBgmShouldLoop();
void gameSetBgmShouldPlay(bool shouldPlay);
bool gameGetBgmShouldPlay();
void gameSetBgmPaused(bool paused);
bool gameGetBgmIsPlaying();
}  // namespace lb

#endif  // !__GAME_H__
