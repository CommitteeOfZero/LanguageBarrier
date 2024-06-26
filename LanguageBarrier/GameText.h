#ifndef __GAMETEXT_H__
#define __GAMETEXT_H__

#include <cstdint>
#include "LanguageBarrier.h"

#ifndef GAMETEXT_H_IMPORT
#define GAMETEXT_H_IMPORT extern
#endif

namespace lb {
LB_GLOBAL uint8_t FIRST_FONT_ID;
LB_GLOBAL float COORDS_MULTIPLIER;
LB_GLOBAL uint8_t FONT_CELL_WIDTH;
LB_GLOBAL uint8_t FONT_CELL_HEIGHT;
LB_GLOBAL uint8_t FONT_ROW_LENGTH;
static const uint16_t TOTAL_NUM_FONT_CELLS = 8000;
LB_GLOBAL uint16_t GLYPH_RANGE_FULLWIDTH_START;
// TODO: make this JSON-configurable in some manner
static const uint16_t MAX_PROCESSED_STRING_LENGTH = 2000;
LB_GLOBAL uint16_t DEFAULT_LINE_LENGTH;
LB_GLOBAL uint16_t DEFAULT_MAX_CHARACTERS;
LB_GLOBAL float SGHD_LINK_UNDERLINE_GLYPH_X;
LB_GLOBAL float SGHD_LINK_UNDERLINE_GLYPH_Y;
// Careful: this also messes with the speaker markers (for spoken lines) and
// highlight in the backlog
// Taken care of with ccBacklogNamePosAdjustHook
LB_GLOBAL int DIALOGUE_REDESIGN_YOFFSET_SHIFT;
LB_GLOBAL int DIALOGUE_REDESIGN_LINEHEIGHT_SHIFT;
LB_GLOBAL bool HAS_BACKLOG_UNDERLINE;
LB_GLOBAL int8_t BACKLOG_HIGHLIGHT_DEFAULT_HEIGHT;
LB_GLOBAL int8_t BACKLOG_HIGHLIGHT_HEIGHT_SHIFT;
LB_GLOBAL float OUTLINE_PADDING;
LB_GLOBAL uint8_t OUTLINE_CELL_WIDTH;
LB_GLOBAL uint8_t OUTLINE_CELL_HEIGHT;
// arbitrarily chosen; I hope the game doesn't try to use this
LB_GLOBAL uint16_t OUTLINE_TEXTURE_ID;
static const int LINECOUNT_DISABLE_OR_ERROR = 0xFF;
static const uint8_t NOT_A_LINK = 0xFF;
LB_GLOBAL int SGHD_PHONE_X_PADDING;
LB_GLOBAL uint16_t GLYPH_ID_FULLWIDTH_SPACE;
LB_GLOBAL uint16_t GLYPH_ID_HALFWIDTH_SPACE;
LB_GLOBAL bool HAS_DOUBLE_GET_SC3_STRING_DISPLAY_WIDTH;
LB_GLOBAL bool HAS_DRAW_PHONE_TEXT;
LB_GLOBAL bool HAS_SGHD_PHONE;
LB_GLOBAL bool HAS_GET_SC3_STRING_LINE_COUNT;
LB_GLOBAL bool HAS_RINE;
LB_GLOBAL bool RINE_BLACK_NAMES;
LB_GLOBAL bool NEEDS_CLEARLIST_TEXT_POSITION_ADJUST;
LB_GLOBAL bool NEEDS_CC_BACKLOG_NAME_POS_ADJUST;
LB_GLOBAL bool IMPROVE_DIALOGUE_OUTLINES;
LB_GLOBAL bool HAS_SPLIT_FONT;
LB_GLOBAL bool TIP_REIMPL;
LB_GLOBAL int TIP_REIMPL_GLYPH_SIZE;
LB_GLOBAL int TIP_REIMPL_LINE_LENGTH;
LB_GLOBAL bool CC_BACKLOG_HIGHLIGHT;
LB_GLOBAL float CC_BACKLOG_HIGHLIGHT_SPRITE_Y;
LB_GLOBAL float CC_BACKLOG_HIGHLIGHT_SPRITE_HEIGHT;
LB_GLOBAL float CC_BACKLOG_HIGHLIGHT_HEIGHT_SHIFT;
LB_GLOBAL float CC_BACKLOG_HIGHLIGHT_YOFFSET_SHIFT;

GAMETEXT_H_IMPORT int* BacklogLineSave;
GAMETEXT_H_IMPORT int* BacklogDispLinePos;
GAMETEXT_H_IMPORT int* BacklogLineBufSize;
GAMETEXT_H_IMPORT int16_t* BacklogTextPos;
GAMETEXT_H_IMPORT int* BacklogLineBufUse;
GAMETEXT_H_IMPORT uint16_t* BacklogText;
GAMETEXT_H_IMPORT int* BacklogDispCurPosSX;
GAMETEXT_H_IMPORT int* BacklogDispCurPosEY;
GAMETEXT_H_IMPORT int* BacklogLineBufStartp;
GAMETEXT_H_IMPORT unsigned char* BacklogTextSize;
GAMETEXT_H_IMPORT int* BacklogLineBufEndp;
GAMETEXT_H_IMPORT int* BacklogBufStartp;
GAMETEXT_H_IMPORT int* MesFontColor;
GAMETEXT_H_IMPORT int* BacklogBufUse;
GAMETEXT_H_IMPORT int* BacklogDispCurPosEX;
GAMETEXT_H_IMPORT int* BacklogDispLineSize;
GAMETEXT_H_IMPORT int* BacklogDispPos;
GAMETEXT_H_IMPORT int* dword_948628;
GAMETEXT_H_IMPORT uint8_t* BacklogTextCo;
GAMETEXT_H_IMPORT int* BacklogLineVoice;
GAMETEXT_H_IMPORT int* BacklogDispLinePosY;
GAMETEXT_H_IMPORT int* BacklogDispCurPosSY;

void gameTextInit();
void fixSkipRN();
void fixLeadingZeroes();
int __cdecl getSc3StringDisplayWidthHook(char* sc3string,
                                         unsigned int maxCharacters,
                                         int baseGlyphSize);
int __cdecl drawSpriteHook(int textureId, float spriteX, float spriteY,
                           float spriteWidth, float spriteHeight,
                           float displayX, float displayY, int color,
                           int opacity, int shaderId);
}  // namespace lb

#endif  // !__GAMETEXT_H__
