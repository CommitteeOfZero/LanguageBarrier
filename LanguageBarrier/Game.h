#ifndef __GAME_H__
#define __GAME_H__

#include <cstdint>

namespace lb {
void gameInit();
void gameLoadTexture(uint8_t textureId, void *buffer, size_t sz);
void *gameMountMpk(char *mountpoint, char *directory, char *filename);
}

#endif  // !__GAME_H__
