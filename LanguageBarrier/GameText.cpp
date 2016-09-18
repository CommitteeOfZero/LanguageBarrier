#include "LanguageBarrier.h"
#include "GameText.h"

typedef void(__cdecl *DrawDialogueProc)(int fontNumber, int pageNumber,
                                        int opacity, int xOffset, int yOffset);
static DrawDialogueProc gameExeDrawDialogue = (DrawDialogueProc)0x44B500;
static DrawDialogueProc gameExeDrawDialogueReal = NULL;

typedef void(__cdecl *DrawGlyphProc)(int textureId, float glyphInTextureStartX,
                                     float glyphInTextureStartY,
                                     float glyphInTextureWidth,
                                     float glyphInTextureHeight,
                                     float displayStartX, float displayStartY,
                                     float displayEndX, float displayEndY,
                                     int color, uint32_t opacity);
static DrawGlyphProc gameExeDrawGlyph = (DrawGlyphProc)0x42F950;

#define MAX_DIALOGUE_PAGE_LENGTH 2000
typedef struct dialoguePage {
  int field_0;
  int field_4;
  int drawNextPageNow;
  int pageLength;
  int field_10;
  char field_14;
  char field_15;
  char field_16;
  char field_17;
  int field_18;
  int field_1C;
  int field_20;
  int field_24;
  int field_28;
  int field_2C;
  int field_30;
  int field_34;
  int field_38;
  int fontNumber[MAX_DIALOGUE_PAGE_LENGTH];
  int charColor[MAX_DIALOGUE_PAGE_LENGTH];
  int charOutlineColor[MAX_DIALOGUE_PAGE_LENGTH];
  char glyphCol[MAX_DIALOGUE_PAGE_LENGTH];
  char glyphRow[MAX_DIALOGUE_PAGE_LENGTH];
  char glyphOrigWidth[MAX_DIALOGUE_PAGE_LENGTH];
  char glyphOrigHeight[MAX_DIALOGUE_PAGE_LENGTH];
  __int16 charDisplayX[MAX_DIALOGUE_PAGE_LENGTH];
  __int16 charDisplayY[MAX_DIALOGUE_PAGE_LENGTH];
  __int16 glyphDisplayWidth[MAX_DIALOGUE_PAGE_LENGTH];
  __int16 glyphDisplayHeight[MAX_DIALOGUE_PAGE_LENGTH];
  char field_BBBC[MAX_DIALOGUE_PAGE_LENGTH];
  int field_C38C[MAX_DIALOGUE_PAGE_LENGTH];
  char charDisplayOpacity[MAX_DIALOGUE_PAGE_LENGTH];
} dialoguePage_t;
static dialoguePage_t *gameExeDialoguePages = (dialoguePage_t *)0x164D680;

static uint8_t *gameExeGlyphWidthsFont1 = (uint8_t *)0x52C7F0;
static uint8_t *gameExeGlyphWidthsFont2 = (uint8_t *)0x52E058;

namespace lb {
void __cdecl drawDialogueHook(int fontNumber, int pageNumber, uint32_t opacity,
                              int xOffset, int yOffset);

void gameTextInit() {
  MH_CreateHook((LPVOID)gameExeDrawDialogue, drawDialogueHook,
                (LPVOID *)&gameExeDrawDialogueReal);
  MH_EnableHook((LPVOID)gameExeDrawDialogue);
}

void __cdecl drawDialogueHook(int fontNumber, int pageNumber, uint32_t opacity,
                              int xOffset, int yOffset) {
  dialoguePage_t *page = &gameExeDialoguePages[pageNumber];

  for (int i = 0; i < page->pageLength; i++) {
    if (fontNumber == page->fontNumber[i]) {
      int displayStartX = (page->charDisplayX[i] + xOffset) * COORDS_MULTIPLIER;
      int displayStartY = (page->charDisplayY[i] + yOffset) * COORDS_MULTIPLIER;

      uint32_t _opacity = (page->charDisplayOpacity[i] * opacity) >> 8;

      gameExeDrawGlyph(
          fontNumber + FIRST_FONT_ID,
          GLYPH_WIDTH * page->glyphCol[i] * COORDS_MULTIPLIER,
          GLYPH_HEIGHT * page->glyphRow[i] * COORDS_MULTIPLIER,
          page->glyphOrigWidth[i] * COORDS_MULTIPLIER,
          page->glyphOrigHeight[i] * COORDS_MULTIPLIER, displayStartX,
          displayStartY,
          displayStartX + (COORDS_MULTIPLIER * page->glyphDisplayWidth[i]),
          displayStartY + (COORDS_MULTIPLIER * page->glyphDisplayHeight[i]),
          page->charColor[i], _opacity);
    }
  }
}
}