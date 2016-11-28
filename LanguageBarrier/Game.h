#ifndef __GAME_H__
#define __GAME_H__

#include <cstdint>
#include "LanguageBarrier.h"

namespace lb {
// DEFAULT VALUES!
LB_GLOBAL uint32_t BGM_CLEAR;
LB_GLOBAL uint8_t MAX_LOADED_SCRIPTS;
LB_GLOBAL uint8_t MPK_ID_SCRIPT_MPK;
LB_GLOBAL uint8_t MPK_ID_BGM_MPK;
LB_GLOBAL uint8_t AUDIO_PLAYER_ID_BGM1;

void gameInit();
void gameLoadTexture(uint8_t textureId, void *buffer, size_t sz);
void *gameMountMpk(char *mountpoint, char *directory, char *filename);
void gameSetBgm(uint32_t fileId, bool shouldLoop);
uint32_t gameGetBgm();
bool gameGetBgmShouldLoop();
void gameSetBgmShouldPlay(bool shouldPlay);
bool gameGetBgmShouldPlay();
void gameSetBgmPaused(bool paused);
}

#endif  // !__GAME_H__
