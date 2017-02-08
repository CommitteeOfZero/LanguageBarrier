#ifndef __GAME_H__
#define __GAME_H__

#include <cstdint>

namespace lb {
static const uint32_t BGM_CLEAR = 0xFFFFFFFF;
static const uint8_t MAX_LOADED_SCRIPTS = 16;
static const uint8_t MPK_ID_SCRIPT_MPK = 3;
static const uint8_t MPK_ID_BGM_MPK = 7;
static const uint8_t AUDIO_PLAYER_ID_BGM1 = 7;

void gameInit();
void gameLoadTexture(uint16_t textureId, void *buffer, size_t sz);
void *gameMountMpk(char *mountpoint, char *directory, char *filename);
void gameSetBgm(uint32_t fileId, bool shouldLoop);
uint32_t gameGetBgm();
bool gameGetBgmShouldLoop();
void gameSetBgmShouldPlay(bool shouldPlay);
bool gameGetBgmShouldPlay();
void gameSetBgmPaused(bool paused);
}

#endif  // !__GAME_H__
