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
  char *sc3StringNext;
  bool error;
} ProcessedSc3String_t;

// also my own
typedef struct {
  uint16_t start;
  uint16_t end;
  uint16_t cost;
  bool startsWithSpace;
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

static uint8_t *gameExeGlyphWidthsFont1 = NULL;  // = (uint8_t *)0x52C7F0;
static uint8_t *gameExeGlyphWidthsFont2 = NULL;  // = (uint8_t *)0x52E058;
static int *gameExeColors = NULL;                // = (int *)0x52E1E8;

static uint8_t widths[lb::TOTAL_NUM_CHARACTERS];

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
void processSc3String(int xOffset, int yOffset, int lineLength, char *sc3string,
                      int lineCount, int color, int baseGlyphSize,
                      ProcessedSc3String_t *result, bool measureOnly,
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

  gameExeDialoguePages =
      (DialoguePage_t *)(*((uint32_t *)((uint8_t *)(gameExeDrawDialogue) +
                                        0x18)) -
                         0xC);
  gameExeGlyphWidthsFont1 =
      *(uint8_t **)((uint8_t *)(gameExeDrawPhoneText) + 0x83);
  gameExeGlyphWidthsFont2 =
      *(uint8_t **)((uint8_t *)(gameExeDrawPhoneText) + 0x74);
  gameExeColors =
      (int *)(*(uint32_t *)((uint8_t *)(gameExeDrawPhoneText) + 0x272) - 0x8);

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
  fread(widths, 1, TOTAL_NUM_CHARACTERS, widthsfile);
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

void processSc3String(int xOffset, int yOffset, int lineLength, char *sc3string,
                      int lineCount, int color, int baseGlyphSize,
                      ProcessedSc3String_t *result, bool measureOnly,
                      float multiplier, bool markError) {
  Sc3_t sc3;
  int sc3evalResult;

// Hack for enlarging the text in @channel threads (which have manual
// linebreaks, and thus a huge margin)
// Not sure whether we want to include this so I'll just leave it disabled for
// now
#if 0
  if (lineLength == 552) {
      baseGlyphSize = 24;
      lineLength = 800;
  }
#endif

  // some padding, to make things look nicer.
  // note that with more padding (e.g. xOffset += 5, lineLength -= 10) an extra
  // empty line may appear at the start of a mail
  // I'm not 100% sure why that is, and this'll probably come back to bite me
  // later, but whatever...
  xOffset += PHONE_X_PADDING;
  lineLength -= 2 * PHONE_X_PADDING;

  memset(result, 0, sizeof(ProcessedSc3String_t));

  int curProcessedStringLength = 0;
  int curLinkNumber = NOT_A_LINK;
  int lastLinkNumber = -1;
  signed char c;
  int currentColor = color;

  uint16_t baseGlyphWidth[MAX_DIALOGUE_PAGE_LENGTH];  // I'm *guessing* that
                                                      // this should fit
  uint8_t linkNumber[MAX_DIALOGUE_PAGE_LENGTH];
  int colors[MAX_DIALOGUE_PAGE_LENGTH];
  uint16_t glyphIds[MAX_DIALOGUE_PAGE_LENGTH];
  char *origNextStrings[MAX_DIALOGUE_PAGE_LENGTH];  // screw it
  memset(baseGlyphWidth, 0, sizeof(baseGlyphWidth));
  memset(linkNumber, 0, sizeof(linkNumber));
  memset(colors, 0, sizeof(colors));
  memset(glyphIds, 0, sizeof(glyphIds));
  memset(origNextStrings, 0, sizeof(origNextStrings));

  result->sc3StringNext = sc3string;

  if (lineCount == 0) return;

  bool done = false;
  while (!done) {
    while (sc3string != NULL) {
      c = *sc3string;
      switch (c) {
        case -1:
          done = true;
          if (markError) result->error = true;
          goto performWrap;
        case 0:
          sc3string++;
          goto performWrap;
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
          // not relevant for our purposes (the original functions skip them
          // too)
          sc3string++;
          break;
        default:
          if (c & 0x80 == 0)
          // if I read this correctly, the game originally just spins in an
          // infinite loop forever here
          // and I don't like that
          {
            if (markError) result->error = true;
            goto performWrap;
          }

          int glyphId = (uint8_t)sc3string[1] + ((c & 0x7f) << 8);
          sc3string += 2;
          int glyphWidth = (baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
          baseGlyphWidth[curProcessedStringLength] = glyphWidth;
          linkNumber[curProcessedStringLength] = curLinkNumber;
          glyphIds[curProcessedStringLength] = glyphId;
          colors[curProcessedStringLength] = currentColor;
          origNextStrings[curProcessedStringLength] = sc3string;
          curProcessedStringLength++;
      }
    }
  performWrap:
    // each word is an index of the last character in that word
    std::list<StringWord_t> words;
    StringWord_t word = {0, 0, 0, false};
    for (uint16_t i = 0; i < curProcessedStringLength; i++) {
      word.cost += baseGlyphWidth[i];
      if (i + 1 >= curProcessedStringLength ||
          glyphIds[i + 1] == GLYPH_ID_FULLWIDTH_SPACE ||
          glyphIds[i + 1] == GLYPH_ID_HALFWIDTH_SPACE) {
        word.end = i;
        words.push_back(word);
        word = {(uint16_t)(i + 1), 0, 0, true};
      }
    }

    // let's pretend there's only one kind of space
    // ...I hope we never have to support non-Latin scripts...
    uint16_t spaceCost = widths[GLYPH_ID_HALFWIDTH_SPACE];

    int curLineLength = 0;
    for (auto it = words.begin();
         it != words.end() && result->lines < lineCount; it++) {
      int nextCost = (it->startsWithSpace == true && curLineLength == 0)
                         ? it->cost - spaceCost
                         : it->cost;
      while (nextCost > lineLength) {
        int firstPartCost = 0;
        for (int j = it->start; j <= it->end && j < curProcessedStringLength;
             j++) {
          if (firstPartCost + baseGlyphWidth[j] > lineLength) {
            StringWord_t nextWord = {j, it->end, it->cost - firstPartCost,
                                     false};
            words.insert(std::next(it), nextWord);
            it->end = j - 1;
            it->cost = firstPartCost;
          } else
            firstPartCost += baseGlyphWidth[j];
        }
        nextCost = (it->startsWithSpace == true && curLineLength == 0)
                       ? it->cost - spaceCost
                       : it->cost;
      }
      if (curLineLength + nextCost >= lineLength) {
        curLineLength = 0;
        result->lines++;
      }

      for (int j = (curLineLength == 0 && it->startsWithSpace ? it->start + 1
                                                              : it->start);
           j <= it->end && j < curProcessedStringLength; j++) {
        sc3string = result->sc3StringNext = origNextStrings[j];
        if (result->lines >= lineCount) break;
        int k = result->length++;
        uint8_t curLinkNumber = linkNumber[j];
        if (curLinkNumber != NOT_A_LINK) {
          result->linkCharCount++;
        }
        uint16_t glyphWidth = baseGlyphWidth[j];
        curLineLength += glyphWidth;
        if (!measureOnly) {
          uint16_t glyphId = glyphIds[j];
          int currentColor = colors[j];
          // anything that's part of an array needs to go here, otherwise we
          // get buffer overflows with long mails
          result->linkNumber[k] = curLinkNumber;
          result->glyph[k] = glyphId;
          result->textureStartX[k] =
              FONT_CELL_WIDTH * multiplier * (glyphId % FONT_ROW_LENGTH);
          result->textureStartY[k] =
              FONT_CELL_HEIGHT * multiplier * (glyphId / FONT_ROW_LENGTH);
          result->textureWidth[k] = widths[glyphId] * multiplier;
          result->textureHeight[k] = FONT_CELL_HEIGHT * multiplier;
          result->displayStartX[k] =
              (xOffset + (curLineLength - glyphWidth)) * multiplier;
          result->displayStartY[k] =
              (yOffset + (result->lines * baseGlyphSize)) * multiplier;
          result->displayEndX[k] = (xOffset + curLineLength) * multiplier;
          result->displayEndY[k] =
              (yOffset + ((result->lines + 1) * baseGlyphSize)) * multiplier;
          result->color[k] = currentColor;
        }
      }
    }

    if (result->lines >= lineCount) done = true;
  }
  if (result->error && markError) result->lines = LINECOUNT_DISABLE_OR_ERROR;
}

int __cdecl drawPhoneTextHook(int textureId, int xOffset, int yOffset,
                              int lineLength, char *sc3string,
                              int lineSkipCount, int lineDisplayCount,
                              int color, int baseGlyphSize, int opacity) {
  ProcessedSc3String_t str;

  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

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
  ProcessedSc3String_t str;
  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  processSc3String(0, 0, lineLength, sc3string, LINECOUNT_DISABLE_OR_ERROR, 0,
                   baseGlyphSize, &str, true, 1.0f, false);
  return str.lines + 1;
}
}