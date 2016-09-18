#include "LanguageBarrier.h"
#include "GameText.h"
#include "Game.h"
#include <fstream>

typedef void(__cdecl *DrawDialogueProc)(int fontNumber, int pageNumber,
                                        int opacity, int xOffset, int yOffset);
static DrawDialogueProc gameExeDrawDialogue = (DrawDialogueProc)0x44B500;
static DrawDialogueProc gameExeDrawDialogueReal = NULL;

typedef void(__cdecl *DrawDialogue2Proc)(int fontNumber, int pageNumber,
                                         int opacity);
static DrawDialogue2Proc gameExeDrawDialogue2 = (DrawDialogue2Proc)0x44B0D0;
static DrawDialogue2Proc gameExeDrawDialogue2Real = NULL;

typedef int(__cdecl *DialogueLayoutRelatedProc)(int unk0, int *unk1, int *unk2,
                                                int unk3, int unk4, int unk5,
                                                int unk6, int yOffset,
                                                int lineHeight);
static DialogueLayoutRelatedProc gameExeDialogueLayoutRelated =
    (DialogueLayoutRelatedProc)0x448790;
static DialogueLayoutRelatedProc gameExeDialogueLayoutRelatedReal = NULL;

typedef void(__cdecl *DrawGlyphProc)(int textureId, float glyphInTextureStartX,
                                     float glyphInTextureStartY,
                                     float glyphInTextureWidth,
                                     float glyphInTextureHeight,
                                     float displayStartX, float displayStartY,
                                     float displayEndX, float displayEndY,
                                     int color, uint32_t opacity);
static DrawGlyphProc gameExeDrawGlyph = (DrawGlyphProc)0x42F950;

static uintptr_t gameExeDialogueLayoutWidthLookup1 = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup1Return = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup2 = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup2Return = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup3 = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup3Return = NULL;

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

static uint8_t widths[8000];

// MSVC doesn't like having these inside namespaces
__declspec(naked) void dialogueLayoutWidthLookup1Hook() {
  __asm {
    movzx edx, widths[ecx]
    jmp gameExeDialogueLayoutWidthLookup1Return
  }
}

__declspec(naked) void dialogueLayoutWidthLookup2Hook() {
  __asm {
    push ebx
    movzx ebx, [lb::FONT_ROW_LENGTH]
    add eax, ebx
    movzx ecx, widths[eax]
    sub eax, ebx
    pop ebx
    jmp gameExeDialogueLayoutWidthLookup2Return
  }
}

__declspec(naked) void dialogueLayoutWidthLookup3Hook() {
  __asm {
    movzx ecx, widths[edx]
    jmp gameExeDialogueLayoutWidthLookup3Return
  }
}

namespace lb {
void __cdecl drawDialogueHook(int fontNumber, int pageNumber, uint32_t opacity,
                              int xOffset, int yOffset);
void __cdecl drawDialogue2Hook(int fontNumber, int pageNumber,
                               uint32_t opacity);
int __cdecl dialogueLayoutRelatedHook(int unk0, int *unk1, int *unk2, int unk3,
                                      int unk4, int unk5, int unk6, int yOffset,
                                      int lineHeight);

void gameTextInit() {
  FILE *widthsfile = fopen("languagebarrier\\widths.bin", "rb");
  fread(widths, 1, 8000, widthsfile);
  fclose(widthsfile);
  memcpy(gameExeGlyphWidthsFont1, widths, 231);
  memcpy(gameExeGlyphWidthsFont2, widths, 231);

  std::ifstream in("languagebarrier\\font-outline.png",
                   std::ios::in | std::ios::binary);
  in.seekg(0, std::ios::end);
  std::string *outlineBuffer = new std::string(in.tellg(), 0);
  in.seekg(0, std::ios::beg);
  in.read(&((*outlineBuffer)[0]), outlineBuffer->size());
  in.close();
  gameLoadTexture(0xF7, &((*outlineBuffer)[0]), outlineBuffer->size());
  // the game loads this asynchronously - I'm not sure how to be notified it's
  // done and I can free the buffer

  MH_CreateHook((LPVOID)gameExeDrawDialogue, drawDialogueHook,
                (LPVOID *)&gameExeDrawDialogueReal);
  MH_EnableHook((LPVOID)gameExeDrawDialogue);
  MH_CreateHook((LPVOID)gameExeDrawDialogue2, drawDialogue2Hook,
                (LPVOID *)&gameExeDrawDialogue2Real);
  MH_EnableHook((LPVOID)gameExeDrawDialogue);
  MH_CreateHook((LPVOID)gameExeDialogueLayoutRelated, dialogueLayoutRelatedHook,
                (LPVOID *)&gameExeDialogueLayoutRelatedReal);
  MH_EnableHook((LPVOID)gameExeDialogueLayoutRelated);

  scanCreateEnableHook("game", "dialogueLayoutWidthLookup1",
                       &gameExeDialogueLayoutWidthLookup1,
                       dialogueLayoutWidthLookup1Hook, NULL);
  gameExeDialogueLayoutWidthLookup1Return =
      (uintptr_t)((uint8_t *)gameExeDialogueLayoutWidthLookup1 + 0x27);
  scanCreateEnableHook("game", "dialogueLayoutWidthLookup2",
                       &gameExeDialogueLayoutWidthLookup2,
                       dialogueLayoutWidthLookup2Hook, NULL);
  gameExeDialogueLayoutWidthLookup2Return =
      (uintptr_t)((uint8_t *)gameExeDialogueLayoutWidthLookup2 + 0x12);
  scanCreateEnableHook("game", "dialogueLayoutWidthLookup3",
                       &gameExeDialogueLayoutWidthLookup3,
                       dialogueLayoutWidthLookup3Hook, NULL);
  gameExeDialogueLayoutWidthLookup3Return =
      (uintptr_t)((uint8_t *)gameExeDialogueLayoutWidthLookup3 + 0x7);
}

int __cdecl dialogueLayoutRelatedHook(int unk0, int *unk1, int *unk2, int unk3,
                                      int unk4, int unk5, int unk6, int yOffset,
                                      int lineHeight) {
  return gameExeDialogueLayoutRelatedReal(unk0, unk1, unk2, unk3, unk4, unk5,
                                          unk6, yOffset + 12, lineHeight - 3);
}

void __cdecl drawDialogueHook(int fontNumber, int pageNumber, uint32_t opacity,
                              int xOffset, int yOffset) {
  dialoguePage_t *page = &gameExeDialoguePages[pageNumber];

  for (int i = 0; i < page->pageLength; i++) {
    if (fontNumber == page->fontNumber[i]) {
      int displayStartX = (page->charDisplayX[i] + xOffset) * COORDS_MULTIPLIER;
      int displayStartY = (page->charDisplayY[i] + yOffset) * COORDS_MULTIPLIER;

      uint32_t _opacity = (page->charDisplayOpacity[i] * opacity) >> 8;

      if (page->charOutlineColor[i] != -1) {
        gameExeDrawGlyph(
            0xF7, GLYPH_WIDTH * page->glyphCol[i] * COORDS_MULTIPLIER,
            GLYPH_HEIGHT * page->glyphRow[i] * COORDS_MULTIPLIER,
            page->glyphOrigWidth[i] * COORDS_MULTIPLIER + 8.0f,
            page->glyphOrigHeight[i] * COORDS_MULTIPLIER, displayStartX - 4.0f,
            displayStartY,
            displayStartX + (COORDS_MULTIPLIER * page->glyphDisplayWidth[i]) +
                4.0f,
            displayStartY + (COORDS_MULTIPLIER * page->glyphDisplayHeight[i]),
            page->charOutlineColor[i], _opacity);
      }

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

void __cdecl drawDialogue2Hook(int fontNumber, int pageNumber,
                               uint32_t opacity) {
  // dunno if this is ever actually called but might as well
  drawDialogueHook(fontNumber, pageNumber, opacity, 0, 0);
}
}