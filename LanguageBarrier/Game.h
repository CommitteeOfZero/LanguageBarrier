#ifndef __GAME_H__
#define __GAME_H__

#include <cstdint>

namespace lb {
static const uint32_t BGM_CLEAR = 0xFFFFFFFF;

void gameInit();
void gameLoadTexture(uint8_t textureId, void *buffer, size_t sz);
void *gameMountMpk(char *mountpoint, char *directory, char *filename);
void gameSetBgm(uint32_t fileId);
}

#endif  // !__GAME_H__
