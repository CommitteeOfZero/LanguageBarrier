#include "LanguageBarrier.h"
#include "GameText.h"
#include "Game.h"
#include <fstream>

typedef struct __declspec(align(4)) {
  char gap0[316];
  int somePageNumber;
  char gap140[16];
  char *pString;
} sc3_t;

// this is my own, not from the game
typedef struct {
  int lines;
  int length;
  int textureStartX[512];
  int textureStartY[512];
  int textureWidth[512];
  int textureHeight[512];
  int displayStartX[512];
  int displayStartY[512];
  int displayEndX[512];
  int displayEndY[512];
  int color[512];
  int glyph[512];
  uint8_t linkNumber[512];
  int linkCharCount;
  char *sc3StringNext;
} processedSc3String_t;
static const uint8_t NOT_A_LINK = 0xFF;

typedef struct __declspec(align(4)) {
  int linkNumber;
  int displayX;
  int displayY;
  int displayWidth;
  int displayHeight;
} LinkMetrics_t;

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

typedef int(__cdecl *DrawRectangleProc)(float X, float Y, float width,
                                        float height, int color,
                                        uint32_t opacity);
static DrawRectangleProc gameExeDrawRectangle = (DrawRectangleProc)0x42F890;

typedef int(__cdecl *DrawPhoneTextProc)(int textureId, int xOffset, int yOffset,
                                        int lineLength, char *sc3string,
                                        int lineSkipCount, int lineDisplayCount,
                                        int color, int baseGlyphSize,
                                        int opacity);
static DrawPhoneTextProc gameExeDrawPhoneText = (DrawPhoneTextProc)0x444F70;
static DrawPhoneTextProc gameExeDrawPhoneTextReal = NULL;

typedef int(__cdecl *GetSc3StringDisplayWidthProc)(char *string,
                                                   unsigned int maxCharacters,
                                                   int baseGlyphSize);
static GetSc3StringDisplayWidthProc gameExeGetSc3StringDisplayWidthFont1 =
    (GetSc3StringDisplayWidthProc)0x4462E0;
static GetSc3StringDisplayWidthProc gameExeGetSc3StringDisplayWidthFont1Real =
    NULL;
static GetSc3StringDisplayWidthProc gameExeGetSc3StringDisplayWidthFont2 =
    (GetSc3StringDisplayWidthProc)0x4461F0;
static GetSc3StringDisplayWidthProc gameExeGetSc3StringDisplayWidthFont2Real =
    NULL;

typedef int(__cdecl *Sc3EvalProc)(sc3_t *sc3, int *pOutResult);
static Sc3EvalProc gameExeSc3Eval = (Sc3EvalProc)0x4181D0;

typedef int(__cdecl *GetLinksFromSc3StringProc)(int xOffset, int yOffset,
                                                int lineLength, char *sc3string,
                                                int lineSkipCount,
                                                int lineDisplayCount,
                                                int baseGlyphSize,
                                                LinkMetrics_t *result);
static GetLinksFromSc3StringProc gameExeGetLinksFromSc3String =
    (GetLinksFromSc3StringProc)0x445EA0;
static GetLinksFromSc3StringProc gameExeGetLinksFromSc3StringReal = NULL;

typedef int(__cdecl *DrawInteractiveMailProc)(
    int textureId, int xOffset, int yOffset, signed int lineLength,
    char *sc3string, unsigned int lineSkipCount, unsigned int lineDisplayCount,
    int color, unsigned int baseGlyphSize, int opacity, int unselectedLinkColor,
    int selectedLinkColor, int selectedLink);
static DrawInteractiveMailProc gameExeDrawInteractiveMail =
    (DrawInteractiveMailProc)0x4453D0;
static DrawInteractiveMailProc gameExeDrawInteractiveMailReal = NULL;

typedef int(__cdecl *DrawLinkHighlightProc)(
    int xOffset, int yOffset, int lineLength, char *sc3string,
    unsigned int lineSkipCount, unsigned int lineDisplayCount, int color,
    unsigned int baseGlyphSize, int opacity, int selectedLink);
static DrawLinkHighlightProc gameExeDrawLinkHighlight =
    (DrawLinkHighlightProc)0x444B90;
static DrawLinkHighlightProc gameExeDrawLinkHighlightReal = NULL;

typedef int(__cdecl *GetSc3StringLineCountProc)(int lineLength, char *sc3string,
                                                unsigned int baseGlyphSize);
static GetSc3StringLineCountProc gameExeGetSc3StringLineCount =
    (GetSc3StringLineCountProc)0x442790;
static GetSc3StringLineCountProc gameExeGetSc3StringLineCountReal = NULL;

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
static int *gameExeColors = (int *)0x52E1E8;

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
int __cdecl drawPhoneTextHook(int textureId, int xOffset, int yOffset,
                              int lineLength, char *sc3string,
                              int lineSkipCount, int lineDisplayCount,
                              int color, int baseGlyphSize, int opacity);
void processSc3String(int xOffset, int yOffset, int lineLength, char *sc3string,
                      int lineCount, int color, int baseGlyphSize,
                      processedSc3String_t *result, bool measureOnly,
                      float multiplier, bool markError);
int __cdecl getSc3StringDisplayWidthHook(char *sc3string,
                                         unsigned int maxCharacters,
                                         int baseGlyphSize);
int __cdecl getLinksFromSc3StringHook(int xOffset, int yOffset, int lineLength,
                                      char *sc3string, int lineSkipCount,
                                      int lineDisplayCount, int baseGlyphSize,
                                      LinkMetrics_t *result);
int __cdecl drawInteractiveMailHook(int textureId, int xOffset, int yOffset,
                                    signed int lineLength, char *string,
                                    unsigned int lineSkipCount,
                                    unsigned int lineDisplayCount, int color,
                                    unsigned int glyphSize, int opacity,
                                    int unselectedLinkColor,
                                    int selectedLinkColor, int selectedLink);
int __cdecl drawLinkHighlightHook(int xOffset, int yOffset, int lineLength,
                                  char *sc3string, unsigned int lineSkipCount,
                                  unsigned int lineDisplayCount, int color,
                                  unsigned int baseGlyphSize, int opacity,
                                  int selectedLink);
int __cdecl getSc3StringLineCountHook(int lineLength, char *sc3string,
                                      unsigned int baseGlyphSize);
// There are a bunch more functions like these but I haven't seen them get hit
// during debugging and the original code *mostly* works okay if it recognises
// western text as variable-width
// (which some functions do, and others don't, except for symbols (also used in
// Western translations) it considers full-width)

void gameTextInit() {
  FILE *widthsfile = fopen("languagebarrier\\widths.bin", "rb");
  fread(widths, 1, 8000, widthsfile);
  fclose(widthsfile);
  memcpy(gameExeGlyphWidthsFont1, widths, 0x15F);
  memcpy(gameExeGlyphWidthsFont2, widths, 0x15F);

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
  MH_CreateHook((LPVOID)gameExeDrawPhoneText, drawPhoneTextHook,
                (LPVOID *)&gameExeDrawPhoneTextReal);
  MH_EnableHook((LPVOID)gameExeDrawPhoneText);
  MH_CreateHook((LPVOID)gameExeGetSc3StringDisplayWidthFont1,
                getSc3StringDisplayWidthHook,
                (LPVOID *)&gameExeGetSc3StringDisplayWidthFont1Real);
  MH_EnableHook((LPVOID)gameExeGetSc3StringDisplayWidthFont1);
  MH_CreateHook((LPVOID)gameExeGetSc3StringDisplayWidthFont2,
                getSc3StringDisplayWidthHook,
                (LPVOID *)&gameExeGetSc3StringDisplayWidthFont2Real);
  MH_EnableHook((LPVOID)gameExeGetSc3StringDisplayWidthFont2);
  MH_CreateHook((LPVOID)gameExeGetLinksFromSc3String, getLinksFromSc3StringHook,
                (LPVOID *)&gameExeGetLinksFromSc3StringReal);
  MH_EnableHook((LPVOID)gameExeGetLinksFromSc3String);
  MH_CreateHook((LPVOID)gameExeDrawInteractiveMail, drawInteractiveMailHook,
                (LPVOID *)&gameExeDrawInteractiveMailReal);
  MH_EnableHook((LPVOID)gameExeDrawInteractiveMail);
  MH_CreateHook((LPVOID)gameExeDrawLinkHighlight, drawLinkHighlightHook,
                (LPVOID *)&gameExeDrawLinkHighlightReal);
  MH_EnableHook((LPVOID)gameExeDrawLinkHighlight);
  MH_CreateHook((LPVOID)gameExeGetSc3StringLineCount, getSc3StringLineCountHook,
                (LPVOID *)&gameExeGetSc3StringLineCountReal);
  MH_EnableHook((LPVOID)gameExeGetSc3StringLineCount);

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

void processSc3String(int xOffset, int yOffset, int lineLength, char *sc3string,
                      int lineCount, int color, int baseGlyphSize,
                      processedSc3String_t *result, bool measureOnly,
                      float multiplier, bool markError) {
  sc3_t sc3;
  int sc3evalResult;

  memset(result, 0, sizeof(processedSc3String_t));

  int curLineLength = 0;
  int curLinkNumber = NOT_A_LINK;  // not a link
  int lastLinkNumber = -1;
  signed char c;
  int currentColor = color;

  while (result->lines < lineCount) {
    c = *sc3string;
    switch (c) {
      case -1:
        if (markError) result->lines = 0xFF;
        goto ret;
      case 0:
        // linebreak
        result->lines++;
        sc3string++;
        curLineLength = 0;
        break;
      case 4:
        // embedded sc3 expression, for changing color
        sc3.pString = sc3string + 1;
        gameExeSc3Eval(&sc3, &sc3evalResult);
        sc3string = sc3.pString;
        if (color)
          currentColor = gameExeColors[2 * sc3evalResult];
        else
          currentColor = gameExeColors[2 * sc3evalResult + 1];
        break;
      case 9:
        // link start
        curLinkNumber = ++lastLinkNumber;
        sc3string++;
        break;
      case 0xB:
        // link end
        curLinkNumber = NOT_A_LINK;
        sc3string++;
        break;
      case 0x1E:
        // SA says these are ruby text start markers
        // not relevant for our purposes (the original functions skip them too)
        sc3string++;
        break;
      default:
        if (c & 0x80 == 0)
        // if I read this correctly, the game originally just spins in an
        // infinite loop forever here
        // and I don't like that
        {
          if (markError) result->lines = 0xFF;
          goto ret;
        }

        int i = result->length;
        int glyphId = (uint8_t)sc3string[1] + ((c & 0x7f) << 8);
        sc3string += 2;
        int glyphWidth = (baseGlyphSize * widths[glyphId]) / GLYPH_WIDTH;
        curLineLength += glyphWidth;
        if (curLineLength + glyphWidth > lineLength) {
          curLineLength = glyphWidth;
          result->lines++;
        }
        if (result->lines < lineCount) {
          result->length++;
          if (curLinkNumber != NOT_A_LINK) {
            result->linkCharCount++;
          }
          if (!measureOnly) {
            // anything that's part of an array needs to go here, otherwise we
            // get buffer overflows with long mails
            result->linkNumber[i] = curLinkNumber;
            result->glyph[i] = glyphId;
            result->textureStartX[i] =
                GLYPH_WIDTH * multiplier * (glyphId % FONT_ROW_LENGTH);
            result->textureStartY[i] =
                GLYPH_HEIGHT * multiplier * (glyphId / FONT_ROW_LENGTH);
            result->textureWidth[i] = widths[glyphId] * multiplier;
            result->textureHeight[i] = GLYPH_HEIGHT * multiplier;
            result->displayStartX[i] =
                (xOffset + (curLineLength - glyphWidth)) * multiplier;
            result->displayStartY[i] =
                (yOffset + (result->lines * baseGlyphSize)) * multiplier;
            result->displayEndX[i] = (xOffset + curLineLength) * multiplier;
            result->displayEndY[i] =
                (yOffset + ((result->lines + 1) * baseGlyphSize)) * multiplier;
            result->color[i] = currentColor;
          }
        }
    }
  }
ret:
  result->lines = min(result->lines, lineCount);
  result->sc3StringNext = sc3string;
}

int __cdecl drawPhoneTextHook(int textureId, int xOffset, int yOffset,
                              int lineLength, char *sc3string,
                              int lineSkipCount, int lineDisplayCount,
                              int color, int baseGlyphSize, int opacity) {
  processedSc3String_t str;

  if (!lineLength) lineLength = 1280;

  processSc3String(xOffset, yOffset, lineLength, sc3string, lineSkipCount,
                   color, baseGlyphSize, &str, true, COORDS_MULTIPLIER, true);
  processSc3String(xOffset, yOffset, lineLength, str.sc3StringNext,
                   lineDisplayCount, color, baseGlyphSize, &str, false,
                   COORDS_MULTIPLIER, true);

  for (int i = 0; i < str.length; i++) {
    gameExeDrawGlyph(textureId, str.textureStartX[i], str.textureStartY[i],
                     str.textureWidth[i], str.textureHeight[i],
                     str.displayStartX[i], str.displayStartY[i],
                     str.displayEndX[i], str.displayEndY[i], str.color[i],
                     opacity);
  }
  return str.lines;
}

int __cdecl getSc3StringDisplayWidthHook(char *sc3string,
                                         unsigned int maxCharacters,
                                         int baseGlyphSize) {
  if (!maxCharacters) maxCharacters = 255;
  sc3_t sc3;
  int sc3evalResult;
  int result = 0;
  int i = 0;
  signed char c;
  while (i <= maxCharacters && (c = *sc3string) != -1) {
    if (c == 4) {
      sc3.pString = sc3string + 1;
      gameExeSc3Eval(&sc3, &sc3evalResult);
      sc3string = sc3.pString;
    } else if (c < 0) {
      int glyphId = (uint8_t)sc3string[1] + ((c & 0x7f) << 8);
      result += (baseGlyphSize * widths[glyphId]) / GLYPH_WIDTH;
      i++;
      sc3string += 2;
    }
  }
  return result;
}

int __cdecl getLinksFromSc3StringHook(int xOffset, int yOffset, int lineLength,
                                      char *sc3string, int lineSkipCount,
                                      int lineDisplayCount, int baseGlyphSize,
                                      LinkMetrics_t *result) {
  processedSc3String_t str;

  if (!lineLength) lineLength = 1280;

  processSc3String(xOffset, yOffset, lineLength, sc3string, lineSkipCount, 0,
                   baseGlyphSize, &str, true, 1.0f, true);
  processSc3String(xOffset, yOffset, lineLength, str.sc3StringNext,
                   lineDisplayCount, 0, baseGlyphSize, &str, false, 1.0f, true);

  int j = 0;
  for (int i = 0; i < str.length; i++) {
    if (str.linkNumber[i] != NOT_A_LINK) {
      result[j].linkNumber = str.linkNumber[i];
      result[j].displayX = str.displayStartX[i];
      result[j].displayY = str.displayStartY[i];
      result[j].displayWidth = str.displayEndX[i] - str.displayStartX[i];
      result[j].displayHeight = str.displayEndY[i] - str.displayStartY[i];

      j++;
      if (j >= str.linkCharCount) return str.linkCharCount;
    }
  }
}

int __cdecl drawInteractiveMailHook(int textureId, int xOffset, int yOffset,
                                    signed int lineLength, char *sc3string,
                                    unsigned int lineSkipCount,
                                    unsigned int lineDisplayCount, int color,
                                    unsigned int baseGlyphSize, int opacity,
                                    int unselectedLinkColor,
                                    int selectedLinkColor, int selectedLink) {
  processedSc3String_t str;

  if (!lineLength) lineLength = 1280;

  processSc3String(xOffset, yOffset, lineLength, sc3string, lineSkipCount,
                   color, baseGlyphSize, &str, true, COORDS_MULTIPLIER, true);
  processSc3String(xOffset, yOffset, lineLength, str.sc3StringNext,
                   lineDisplayCount, color, baseGlyphSize, &str, false,
                   COORDS_MULTIPLIER, true);

  for (int i = 0; i < str.length; i++) {
    int curColor = str.color[i];
    if (str.linkNumber[i] != NOT_A_LINK) {
      if (str.linkNumber[i] == selectedLink)
        curColor = selectedLinkColor;
      else
        curColor = unselectedLinkColor;

      gameExeDrawGlyph(textureId, 1009.5, 193.5, str.textureWidth[i],
                       str.textureHeight[i], str.displayStartX[i],
                       str.displayStartY[i], str.displayEndX[i],
                       str.displayEndY[i], curColor, opacity);
    }

    gameExeDrawGlyph(textureId, str.textureStartX[i], str.textureStartY[i],
                     str.textureWidth[i], str.textureHeight[i],
                     str.displayStartX[i], str.displayStartY[i],
                     str.displayEndX[i], str.displayEndY[i], curColor, opacity);
  }
  return str.lines;
}

int __cdecl drawLinkHighlightHook(int xOffset, int yOffset, int lineLength,
                                  char *sc3string, unsigned int lineSkipCount,
                                  unsigned int lineDisplayCount, int color,
                                  unsigned int baseGlyphSize, int opacity,
                                  int selectedLink) {
  processedSc3String_t str;

  if (!lineLength) lineLength = 1280;

  processSc3String(xOffset, yOffset, lineLength, sc3string, lineSkipCount,
                   color, baseGlyphSize, &str, true, COORDS_MULTIPLIER, true);
  processSc3String(xOffset, yOffset, lineLength, str.sc3StringNext,
                   lineDisplayCount, color, baseGlyphSize, &str, false,
                   COORDS_MULTIPLIER, true);

  if (selectedLink == NOT_A_LINK) return str.lines;

  for (int i = 0; i < str.length; i++) {
    if (str.linkNumber[i] == selectedLink) {
      gameExeDrawRectangle(str.displayStartX[i], str.displayStartY[i],
                           str.displayEndX[i] - str.displayStartX[i],
                           str.displayEndY[i] - str.displayStartY[i], color,
                           opacity);
    }
  }
  return str.lines;
}

// This is used to set bounds for scrolling
int __cdecl getSc3StringLineCountHook(int lineLength, char *sc3string,
                                      unsigned int baseGlyphSize) {
  processedSc3String_t str;
  if (!lineLength) lineLength = 1280;

  processSc3String(0, 0, lineLength, sc3string, 0xFF, 0, baseGlyphSize, &str,
                   true, 1.0f, false);
  return str.lines;
}
}