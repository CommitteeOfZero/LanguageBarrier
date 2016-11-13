#ifndef __GAMETEXT_H__
#define __GAMETEXT_H__

#include <cstdint>

namespace lb {
static const uint8_t FIRST_FONT_ID = 0x4E;
static const float COORDS_MULTIPLIER = 1.5f;
static const uint8_t FONT_CELL_WIDTH = 32;
static const uint8_t FONT_CELL_HEIGHT = 32;
static const uint8_t FONT_ROW_LENGTH = 64;
static const uint16_t TOTAL_NUM_CHARACTERS = 8000;
static const uint16_t GLYPH_RANGE_FULLWIDTH_START = 0x15F;
static const uint16_t MAX_DIALOGUE_PAGE_LENGTH = 2000;
static const uint16_t MAX_PROCESSED_STRING_LENGTH = 512;
static const uint16_t DEFAULT_LINE_LENGTH = 1280;
static const uint16_t DEFAULT_MAX_CHARACTERS = 255;
static const float UNDERLINE_GLYPH_X = 1009.5f;
static const float UNDERLINE_GLYPH_Y = 193.5f;
// Careful: this also messes with the speaker markers (for spoken lines) and
// highlight in the backlog
static const int DIALOGUE_REDESIGN_YOFFSET_SHIFT = 15;
static const int DIALOGUE_REDESIGN_LINEHEIGHT_SHIFT = -3;
static const float OUTLINE_EXTRA_X = 4.0f;
// arbitrarily chosen; I hope the game doesn't try to use this
static const uint8_t OUTLINE_TEXTURE_ID = 0xF7;
static const int LINECOUNT_DISABLE_OR_ERROR = 0xFF;
static const uint8_t NOT_A_LINK = 0xFF;
static const int PHONE_X_PADDING = 2;
static const uint16_t GLYPH_ID_FULLWIDTH_SPACE = 0;
static const uint16_t GLYPH_ID_HALFWIDTH_SPACE = 63;

void gameTextInit();
}

#endif  // !__GAMETEXT_H__
