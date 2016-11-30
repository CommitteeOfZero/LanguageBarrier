#include "GameText.h"
#include <fstream>
#include <list>
#include <sstream>
#include <vector>
#include "Game.h"
#include "LanguageBarrier.h"
#include "SigScan.h"

typedef struct __declspec(align(4)) {
  char gap0[316];
  int somePageNumber;
  char gap140[12];
  char *pString;
} Sc3_t;

// this is my own, not from the game
typedef struct {
  int lines;
  int length;
  int textureStartX[lb::MAX_PROCESSED_STRING_LENGTH];
  int textureStartY[lb::MAX_PROCESSED_STRING_LENGTH];
  int textureWidth[lb::MAX_PROCESSED_STRING_LENGTH];
  int textureHeight[lb::MAX_PROCESSED_STRING_LENGTH];
  int displayStartX[lb::MAX_PROCESSED_STRING_LENGTH];
  int displayStartY[lb::MAX_PROCESSED_STRING_LENGTH];
  int displayEndX[lb::MAX_PROCESSED_STRING_LENGTH];
  int displayEndY[lb::MAX_PROCESSED_STRING_LENGTH];
  int color[lb::MAX_PROCESSED_STRING_LENGTH];
  int glyph[lb::MAX_PROCESSED_STRING_LENGTH];
  uint8_t linkNumber[lb::MAX_PROCESSED_STRING_LENGTH];
  int linkCharCount;
  int linkCount;
  int curLinkNumber;
  int curColor;
  bool error;
} ProcessedSc3String_t;

// also my own
typedef struct {
  char *start;
  char *end;
  uint16_t cost;
  bool startsWithSpace;
  bool endsWithLinebreak;
} StringWord_t;

typedef struct __declspec(align(4)) {
  int linkNumber;
  int displayX;
  int displayY;
  int displayWidth;
  int displayHeight;
} LinkMetrics_t;

typedef struct {
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
  int fontNumber[lb::MAX_DIALOGUE_PAGE_LENGTH];
  int charColor[lb::MAX_DIALOGUE_PAGE_LENGTH];
  int charOutlineColor[lb::MAX_DIALOGUE_PAGE_LENGTH];
  char glyphCol[lb::MAX_DIALOGUE_PAGE_LENGTH];
  char glyphRow[lb::MAX_DIALOGUE_PAGE_LENGTH];
  char glyphOrigWidth[lb::MAX_DIALOGUE_PAGE_LENGTH];
  char glyphOrigHeight[lb::MAX_DIALOGUE_PAGE_LENGTH];
  __int16 charDisplayX[lb::MAX_DIALOGUE_PAGE_LENGTH];
  __int16 charDisplayY[lb::MAX_DIALOGUE_PAGE_LENGTH];
  __int16 glyphDisplayWidth[lb::MAX_DIALOGUE_PAGE_LENGTH];
  __int16 glyphDisplayHeight[lb::MAX_DIALOGUE_PAGE_LENGTH];
  char field_BBBC[lb::MAX_DIALOGUE_PAGE_LENGTH];
  int field_C38C[lb::MAX_DIALOGUE_PAGE_LENGTH];
  char charDisplayOpacity[lb::MAX_DIALOGUE_PAGE_LENGTH];
} DialoguePage_t;

typedef void(__cdecl *DrawDialogueProc)(int fontNumber, int pageNumber,
                                        int opacity, int xOffset, int yOffset);
static DrawDialogueProc gameExeDrawDialogue =
    NULL;  // = (DrawDialogueProc)0x44B500;
static DrawDialogueProc gameExeDrawDialogueReal = NULL;

typedef void(__cdecl *DrawDialogue2Proc)(int fontNumber, int pageNumber,
                                         int opacity);
static DrawDialogue2Proc gameExeDrawDialogue2 =
    NULL;  // = (DrawDialogue2Proc)0x44B0D0;
static DrawDialogue2Proc gameExeDrawDialogue2Real = NULL;

typedef int(__cdecl *DialogueLayoutRelatedProc)(int unk0, int *unk1, int *unk2,
                                                int unk3, int unk4, int unk5,
                                                int unk6, int yOffset,
                                                int lineHeight);
static DialogueLayoutRelatedProc gameExeDialogueLayoutRelated =
    NULL;  // = (DialogueLayoutRelatedProc)0x448790;
static DialogueLayoutRelatedProc gameExeDialogueLayoutRelatedReal = NULL;

typedef void(__cdecl *DrawGlyphProc)(int textureId, float glyphInTextureStartX,
                                     float glyphInTextureStartY,
                                     float glyphInTextureWidth,
                                     float glyphInTextureHeight,
                                     float displayStartX, float displayStartY,
                                     float displayEndX, float displayEndY,
                                     int color, uint32_t opacity);
static DrawGlyphProc gameExeDrawGlyph = NULL;  // = (DrawGlyphProc)0x42F950;

typedef int(__cdecl *DrawRectangleProc)(float X, float Y, float width,
                                        float height, int color,
                                        uint32_t opacity);
static DrawRectangleProc gameExeDrawRectangle =
    NULL;  // = (DrawRectangleProc)0x42F890;

typedef int(__cdecl *DrawPhoneTextProc)(int textureId, int xOffset, int yOffset,
                                        int lineLength, char *sc3string,
                                        int lineSkipCount, int lineDisplayCount,
                                        int color, int baseGlyphSize,
                                        int opacity);
static DrawPhoneTextProc gameExeDrawPhoneText =
    NULL;  // = (DrawPhoneTextProc)0x444F70;
static DrawPhoneTextProc gameExeDrawPhoneTextReal = NULL;

typedef int(__cdecl *GetSc3StringDisplayWidthProc)(char *string,
                                                   unsigned int maxCharacters,
                                                   int baseGlyphSize);
static GetSc3StringDisplayWidthProc gameExeGetSc3StringDisplayWidthFont1 =
    NULL;  // = (GetSc3StringDisplayWidthProc)0x4462E0;
static GetSc3StringDisplayWidthProc gameExeGetSc3StringDisplayWidthFont1Real =
    NULL;
static GetSc3StringDisplayWidthProc gameExeGetSc3StringDisplayWidthFont2 =
    NULL;  // = (GetSc3StringDisplayWidthProc)0x4461F0;
static GetSc3StringDisplayWidthProc gameExeGetSc3StringDisplayWidthFont2Real =
    NULL;

typedef int(__cdecl *Sc3EvalProc)(Sc3_t *sc3, int *pOutResult);
static Sc3EvalProc gameExeSc3Eval = NULL;  // = (Sc3EvalProc)0x4181D0;

typedef int(__cdecl *GetLinksFromSc3StringProc)(int xOffset, int yOffset,
                                                int lineLength, char *sc3string,
                                                int lineSkipCount,
                                                int lineDisplayCount,
                                                int baseGlyphSize,
                                                LinkMetrics_t *result);
static GetLinksFromSc3StringProc gameExeGetLinksFromSc3String =
    NULL;  // = (GetLinksFromSc3StringProc)0x445EA0;
static GetLinksFromSc3StringProc gameExeGetLinksFromSc3StringReal = NULL;

typedef int(__cdecl *DrawInteractiveMailProc)(
    int textureId, int xOffset, int yOffset, signed int lineLength,
    char *sc3string, unsigned int lineSkipCount, unsigned int lineDisplayCount,
    int color, unsigned int baseGlyphSize, int opacity, int unselectedLinkColor,
    int selectedLinkColor, int selectedLink);
static DrawInteractiveMailProc gameExeDrawInteractiveMail =
    NULL;  // = (DrawInteractiveMailProc)0x4453D0;
static DrawInteractiveMailProc gameExeDrawInteractiveMailReal = NULL;

typedef int(__cdecl *DrawLinkHighlightProc)(
    int xOffset, int yOffset, int lineLength, char *sc3string,
    unsigned int lineSkipCount, unsigned int lineDisplayCount, int color,
    unsigned int baseGlyphSize, int opacity, int selectedLink);
static DrawLinkHighlightProc gameExeDrawLinkHighlight =
    NULL;  // = (DrawLinkHighlightProc)0x444B90;
static DrawLinkHighlightProc gameExeDrawLinkHighlightReal = NULL;

typedef int(__cdecl *GetSc3StringLineCountProc)(int lineLength, char *sc3string,
                                                unsigned int baseGlyphSize);
static GetSc3StringLineCountProc gameExeGetSc3StringLineCount =
    NULL;  // = (GetSc3StringLineCountProc)0x442790;
static GetSc3StringLineCountProc gameExeGetSc3StringLineCountReal = NULL;

static uintptr_t gameExeDialogueLayoutWidthLookup1 = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup1Return = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup2 = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup2Return = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup3 = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup3Return = NULL;

static DialoguePage_t *gameExeDialoguePages =
    NULL;  // (DialoguePage_t *)0x164D680;

static uint8_t *gameExeGlyphWidthsFont1 = NULL;       // = (uint8_t *)0x52C7F0;
static uint8_t *gameExeGlyphWidthsFont2 = NULL;       // = (uint8_t *)0x52E058;
static int *gameExeColors = NULL;                     // = (int *)0x52E1E8;
static int8_t *gameExeBacklogHighlightHeight = NULL;  // = (int8_t *)0x435DD4;

static uint8_t widths[lb::TOTAL_NUM_FONT_CELLS];

static std::string *outlineBuffer;

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
void semiTokeniseSc3String(char *sc3string, std::list<StringWord_t> &words,
                           int baseGlyphSize, int lineLength);
void processSc3TokenList(int xOffset, int yOffset, int lineLength,
                         std::list<StringWord_t> &words, int lineCount,
                         int color, int baseGlyphSize,
                         ProcessedSc3String_t *result, bool measureOnly,
                         float multiplier, int lastLinkNumber,
                         int curLinkNumber, int currentColor);
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
  std::ifstream in("languagebarrier\\font-outline.png",
                   std::ios::in | std::ios::binary);
  in.seekg(0, std::ios::end);
  outlineBuffer = new std::string(in.tellg(), 0);
  in.seekg(0, std::ios::beg);
  in.read(&((*outlineBuffer)[0]), outlineBuffer->size());
  in.close();
  // gee I sure hope nothing important ever goes in OUTLINE_TEXTURE_ID...
  gameLoadTexture(OUTLINE_TEXTURE_ID, &((*outlineBuffer)[0]),
                  outlineBuffer->size());
  // the game loads this asynchronously - I'm not sure how to be notified it's
  // done and I can free the buffer
  // so I'll just do it in a hook

  gameExeDrawGlyph = (DrawGlyphProc)sigScan("game", "drawGlyph");
  gameExeDrawRectangle = (DrawRectangleProc)sigScan("game", "drawRectangle");
  gameExeSc3Eval = (Sc3EvalProc)sigScan("game", "sc3Eval");
  gameExeBacklogHighlightHeight =
      (int8_t *)sigScan("game", "backlogHighlightHeight");

  // gameExeBacklogHighlightHeight is (negative) offset (from vertical end of
  // glyph):
  // add eax,-0x22 (83 C0 DE) -> add eax,-0x17 (83 C0 E9)
  DWORD oldProtect;
  VirtualProtect(gameExeBacklogHighlightHeight, 1, PAGE_READWRITE, &oldProtect);
  *gameExeBacklogHighlightHeight =
      BACKLOG_HIGHLIGHT_DEFAULT_HEIGHT + BACKLOG_HIGHLIGHT_HEIGHT_SHIFT;
  VirtualProtect(gameExeBacklogHighlightHeight, 1, oldProtect, &oldProtect);

  gameExeGlyphWidthsFont1 = (uint8_t *)sigScan("game", "useOfGlyphWidthsFont1");
  gameExeGlyphWidthsFont2 = (uint8_t *)sigScan("game", "useOfGlyphWidthsFont2");
  gameExeColors = (int *)sigScan("game", "useOfColors");
  gameExeDialoguePages =
      (DialoguePage_t *)sigScan("game", "useOfDialoguePages");

  scanCreateEnableHook(
      "game", "drawDialogue", (uintptr_t *)&gameExeDrawDialogue,
      (LPVOID)drawDialogueHook, (LPVOID *)&gameExeDrawDialogueReal);
  scanCreateEnableHook(
      "game", "drawDialogue2", (uintptr_t *)&gameExeDrawDialogue2,
      (LPVOID)drawDialogue2Hook, (LPVOID *)&gameExeDrawDialogue2Real);
  scanCreateEnableHook("game", "dialogueLayoutRelated",
                       (uintptr_t *)&gameExeDialogueLayoutRelated,
                       (LPVOID)dialogueLayoutRelatedHook,
                       (LPVOID *)&gameExeDialogueLayoutRelatedReal);
  scanCreateEnableHook(
      "game", "drawPhoneText", (uintptr_t *)&gameExeDrawPhoneText,
      (LPVOID)drawPhoneTextHook, (LPVOID *)&gameExeDrawPhoneTextReal);
  // The following both have the same pattern and 'occurrence: 0' in the
  // signatures.json.
  // That's because after you hook one, the first match goes away.
  scanCreateEnableHook("game", "getSc3StringDisplayWidthFont1",
                       (uintptr_t *)&gameExeGetSc3StringDisplayWidthFont1,
                       (LPVOID)getSc3StringDisplayWidthHook,
                       (LPVOID *)&gameExeGetSc3StringDisplayWidthFont1Real);
  scanCreateEnableHook("game", "getSc3StringDisplayWidthFont2",
                       (uintptr_t *)&gameExeGetSc3StringDisplayWidthFont2,
                       (LPVOID)getSc3StringDisplayWidthHook,
                       (LPVOID *)&gameExeGetSc3StringDisplayWidthFont2Real);
  scanCreateEnableHook("game", "getLinksFromSc3String",
                       (uintptr_t *)&gameExeGetLinksFromSc3String,
                       (LPVOID)getLinksFromSc3StringHook,
                       (LPVOID *)&gameExeGetLinksFromSc3StringReal);
  scanCreateEnableHook("game", "drawInteractiveMail",
                       (uintptr_t *)&gameExeDrawInteractiveMail,
                       (LPVOID)drawInteractiveMailHook,
                       (LPVOID *)&gameExeDrawInteractiveMailReal);
  scanCreateEnableHook(
      "game", "drawLinkHighlight", (uintptr_t *)&gameExeDrawLinkHighlight,
      (LPVOID)drawLinkHighlightHook, (LPVOID *)&gameExeDrawLinkHighlightReal);
  scanCreateEnableHook("game", "getSc3StringLineCount",
                       (uintptr_t *)&gameExeGetSc3StringLineCount,
                       (LPVOID)getSc3StringLineCountHook,
                       (LPVOID *)&gameExeGetSc3StringLineCountReal);

  // no point using the expression parser for these since the code is
  // build-specific anyway
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

  FILE *widthsfile = fopen("languagebarrier\\widths.bin", "rb");
  fread(widths, 1, TOTAL_NUM_FONT_CELLS, widthsfile);
  fclose(widthsfile);
  memcpy(gameExeGlyphWidthsFont1, widths, GLYPH_RANGE_FULLWIDTH_START);
  memcpy(gameExeGlyphWidthsFont2, widths, GLYPH_RANGE_FULLWIDTH_START);
}

int __cdecl dialogueLayoutRelatedHook(int unk0, int *unk1, int *unk2, int unk3,
                                      int unk4, int unk5, int unk6, int yOffset,
                                      int lineHeight) {
  if (outlineBuffer != NULL) {
    // let's just do this here, should be loaded by now...
    delete outlineBuffer;
    outlineBuffer = NULL;
  }

  return gameExeDialogueLayoutRelatedReal(
      unk0, unk1, unk2, unk3, unk4, unk5, unk6,
      yOffset + DIALOGUE_REDESIGN_YOFFSET_SHIFT,
      lineHeight + DIALOGUE_REDESIGN_LINEHEIGHT_SHIFT);
}

void __cdecl drawDialogueHook(int fontNumber, int pageNumber, uint32_t opacity,
                              int xOffset, int yOffset) {
  DialoguePage_t *page = &gameExeDialoguePages[pageNumber];

  for (int i = 0; i < page->pageLength; i++) {
    if (fontNumber == page->fontNumber[i]) {
      int displayStartX = (page->charDisplayX[i] + xOffset) * COORDS_MULTIPLIER;
      int displayStartY = (page->charDisplayY[i] + yOffset) * COORDS_MULTIPLIER;

      uint32_t _opacity = (page->charDisplayOpacity[i] * opacity) >> 8;

      if (page->charOutlineColor[i] != -1) {
        gameExeDrawGlyph(
            OUTLINE_TEXTURE_ID,
            FONT_CELL_WIDTH * page->glyphCol[i] * COORDS_MULTIPLIER,
            FONT_CELL_HEIGHT * page->glyphRow[i] * COORDS_MULTIPLIER,
            page->glyphOrigWidth[i] * COORDS_MULTIPLIER + (2 * OUTLINE_EXTRA_X),
            page->glyphOrigHeight[i] * COORDS_MULTIPLIER,
            displayStartX - OUTLINE_EXTRA_X, displayStartY,
            displayStartX + (COORDS_MULTIPLIER * page->glyphDisplayWidth[i]) +
                OUTLINE_EXTRA_X,
            displayStartY + (COORDS_MULTIPLIER * page->glyphDisplayHeight[i]),
            page->charOutlineColor[i], _opacity);
      }

      gameExeDrawGlyph(
          fontNumber + FIRST_FONT_ID,
          FONT_CELL_WIDTH * page->glyphCol[i] * COORDS_MULTIPLIER,
          FONT_CELL_HEIGHT * page->glyphRow[i] * COORDS_MULTIPLIER,
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

void semiTokeniseSc3String(char *sc3string, std::list<StringWord_t> &words,
                           int baseGlyphSize, int lineLength) {
  lineLength -= 2 * SGHD_PHONE_X_PADDING;

  Sc3_t sc3;
  int sc3evalResult;
  StringWord_t word = {sc3string, NULL, 0, false, false};
  char c;
  while (sc3string != NULL) {
    c = *sc3string;
    switch (c) {
      case -1:
        word.end = sc3string - 1;
        words.push_back(word);
        return;
      case 0:
        word.end = sc3string - 1;
        word.endsWithLinebreak = true;
        words.push_back(word);
        word = {++sc3string, NULL, 0, false, false};
        break;
      case 4:
        sc3.pString = sc3string + 1;
        gameExeSc3Eval(&sc3, &sc3evalResult);
        sc3string = sc3.pString;
        break;
      case 9:
      case 0xB:
      case 0x1E:
        sc3string++;
        break;
      default:
        int glyphId = (uint8_t)sc3string[1] + ((c & 0x7f) << 8);
        if (glyphId == GLYPH_ID_FULLWIDTH_SPACE ||
            glyphId == GLYPH_ID_HALFWIDTH_SPACE) {
          word.end = sc3string - 1;
          words.push_back(word);
          word = {sc3string, NULL, widths[glyphId], true, false};
        } else {
          int glyphWidth = (baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
          if (word.cost + glyphWidth > lineLength) {
            word.end = sc3string - 1;
            words.push_back(word);
            word = {sc3string, NULL, 0, false, false};
          }
          word.cost += glyphWidth;
        }
        sc3string += 2;
        break;
    }
  }
}

void processSc3TokenList(int xOffset, int yOffset, int lineLength,
                         std::list<StringWord_t> &words, int lineCount,
                         int color, int baseGlyphSize,
                         ProcessedSc3String_t *result, bool measureOnly,
                         float multiplier, int lastLinkNumber,
                         int curLinkNumber, int currentColor) {
  Sc3_t sc3;
  int sc3evalResult;

  // some padding, to make things look nicer.
  // note that with more padding (e.g. xOffset += 5, lineLength -= 10) an extra
  // empty line may appear at the start of a mail
  // I'm not 100% sure why that is, and this'll probably come back to bite me
  // later, but whatever...
  xOffset += SGHD_PHONE_X_PADDING;
  lineLength -= 2 * SGHD_PHONE_X_PADDING;

  memset(result, 0, sizeof(ProcessedSc3String_t));

  int curProcessedStringLength = 0;
  int curLineLength = 0;

  int spaceCost =
      (widths[GLYPH_ID_FULLWIDTH_SPACE] * baseGlyphSize) / FONT_CELL_WIDTH;

  for (auto it = words.begin(); it != words.end(); it++) {
    if (result->lines >= lineCount) {
      words.erase(words.begin(), it);
      break;
    }
    int wordCost =
        it->cost -
        ((curLineLength == 0 && it->startsWithSpace == true) ? spaceCost : 0);
    if (curLineLength + wordCost > lineLength) {
      if (curLineLength != 0 && it->startsWithSpace == true)
        wordCost -= spaceCost;
      result->lines++;
      curLineLength = 0;
    }
    if (result->lines >= lineCount) {
      words.erase(words.begin(), it);
      break;
    };

    char c;
    char *sc3string = (curLineLength == 0 && it->startsWithSpace == true)
                          ? it->start + 2
                          : it->start;
    while (sc3string <= it->end) {
      c = *sc3string;
      switch (c) {
        case -1:
          goto afterWord;
          break;
        case 0:
          goto afterWord;
          break;
        case 4:
          sc3.pString = sc3string + 1;
          gameExeSc3Eval(&sc3, &sc3evalResult);
          sc3string = sc3.pString;
          if (color)
            currentColor = gameExeColors[2 * sc3evalResult];
          else
            currentColor = gameExeColors[2 * sc3evalResult + 1];
          break;
        case 9:
          curLinkNumber = ++lastLinkNumber;
          sc3string++;
          break;
        case 0xB:
          curLinkNumber = NOT_A_LINK;
          sc3string++;
          break;
        case 0x1E:
          sc3string++;
          break;
        default:
          int glyphId = (uint8_t)sc3string[1] + ((c & 0x7f) << 8);
          int i = result->length;

          if (result->lines >= lineCount) break;
          if (curLinkNumber != NOT_A_LINK) {
            result->linkCharCount++;
          }
          uint16_t glyphWidth =
              (baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
          curLineLength += glyphWidth;
          if (!measureOnly) {
            // anything that's part of an array needs to go here, otherwise we
            // get buffer overflows with long mails
            result->linkNumber[i] = curLinkNumber;
            result->glyph[i] = glyphId;
            result->textureStartX[i] =
                FONT_CELL_WIDTH * multiplier * (glyphId % FONT_ROW_LENGTH);
            result->textureStartY[i] =
                FONT_CELL_HEIGHT * multiplier * (glyphId / FONT_ROW_LENGTH);
            result->textureWidth[i] = widths[glyphId] * multiplier;
            result->textureHeight[i] = FONT_CELL_HEIGHT * multiplier;
            result->displayStartX[i] =
                (xOffset + (curLineLength - glyphWidth)) * multiplier;
            result->displayStartY[i] =
                (yOffset + (result->lines * baseGlyphSize)) * multiplier;
            result->displayEndX[i] = (xOffset + curLineLength) * multiplier;
            result->displayEndY[i] =
                (yOffset + ((result->lines + 1) * baseGlyphSize)) * multiplier;
            result->color[i] = currentColor;
          }
          result->length++;
          sc3string += 2;
          break;
      }
    }
  afterWord:
    if (it->endsWithLinebreak) {
      result->lines++;
      curLineLength = 0;
    }
  }

  if (curLineLength == 0) result->lines--;
  // For some reason we come up one line too short for some mails (e.g. Moeka's
  // first). Unfortunately, this workaround also adds a blank line below mail
  // subjects, but it's better than having them be unreadable.
  result->lines++;

  result->linkCount = lastLinkNumber + 1;
  result->curColor = currentColor;
  result->curLinkNumber = curLinkNumber;
}

int __cdecl drawPhoneTextHook(int textureId, int xOffset, int yOffset,
                              int lineLength, char *sc3string,
                              int lineSkipCount, int lineDisplayCount,
                              int color, int baseGlyphSize, int opacity) {
  ProcessedSc3String_t str;

  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(sc3string, words, baseGlyphSize, lineLength);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineSkipCount, color,
                      baseGlyphSize, &str, true, COORDS_MULTIPLIER, -1,
                      NOT_A_LINK, color);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineDisplayCount,
                      color, baseGlyphSize, &str, false, COORDS_MULTIPLIER,
                      str.linkCount - 1, str.curLinkNumber, str.curColor);

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
  if (!maxCharacters) maxCharacters = DEFAULT_MAX_CHARACTERS;
  Sc3_t sc3;
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
      result += (baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
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
  ProcessedSc3String_t str;

  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(sc3string, words, baseGlyphSize, lineLength);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineSkipCount, 0,
                      baseGlyphSize, &str, true, 1.0f, -1, NOT_A_LINK, 0);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineDisplayCount, 0,
                      baseGlyphSize, &str, false, 1.0f, str.linkCount - 1,
                      str.curLinkNumber, str.curColor);

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
  return j;
}

// This is also used for @channel threads
int __cdecl drawInteractiveMailHook(int textureId, int xOffset, int yOffset,
                                    signed int lineLength, char *sc3string,
                                    unsigned int lineSkipCount,
                                    unsigned int lineDisplayCount, int color,
                                    unsigned int baseGlyphSize, int opacity,
                                    int unselectedLinkColor,
                                    int selectedLinkColor, int selectedLink) {
  ProcessedSc3String_t str;

  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(sc3string, words, baseGlyphSize, lineLength);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineSkipCount, color,
                      baseGlyphSize, &str, true, COORDS_MULTIPLIER, -1,
                      NOT_A_LINK, color);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineDisplayCount,
                      color, baseGlyphSize, &str, false, COORDS_MULTIPLIER,
                      str.linkCount - 1, str.curLinkNumber, str.curColor);

  for (int i = 0; i < str.length; i++) {
    int curColor = str.color[i];
    if (str.linkNumber[i] != NOT_A_LINK) {
      if (str.linkNumber[i] == selectedLink)
        curColor = selectedLinkColor;
      else
        curColor = unselectedLinkColor;

      gameExeDrawGlyph(
          textureId, UNDERLINE_GLYPH_X, UNDERLINE_GLYPH_Y, str.textureWidth[i],
          str.textureHeight[i], str.displayStartX[i], str.displayStartY[i],
          str.displayEndX[i], str.displayEndY[i], curColor, opacity);
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
  ProcessedSc3String_t str;

  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(sc3string, words, baseGlyphSize, lineLength);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineSkipCount, color,
                      baseGlyphSize, &str, true, COORDS_MULTIPLIER, -1,
                      NOT_A_LINK, color);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineDisplayCount,
                      color, baseGlyphSize, &str, false, COORDS_MULTIPLIER,
                      str.linkCount - 1, str.curLinkNumber, str.curColor);

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
  ProcessedSc3String_t str;
  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(sc3string, words, baseGlyphSize, lineLength);
  processSc3TokenList(0, 0, lineLength, words, LINECOUNT_DISABLE_OR_ERROR, 0,
                      baseGlyphSize, &str, true, 1.0f, -1, NOT_A_LINK, 0);
  return str.lines + 1;
}
}