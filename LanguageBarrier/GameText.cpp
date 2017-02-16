#include "GameText.h"
#include <fstream>
#include <list>
#include <sstream>
#include <vector>
#include "Game.h"
#include "LanguageBarrier.h"
#include "SigScan.h"
#include "Config.h"

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

typedef int(__cdecl *DrawGlyphProc)(int textureId, float glyphInTextureStartX,
                                    float glyphInTextureStartY,
                                    float glyphInTextureWidth,
                                    float glyphInTextureHeight,
                                    float displayStartX, float displayStartY,
                                    float displayEndX, float displayEndY,
                                    int color, uint32_t opacity);
static DrawGlyphProc gameExeDrawGlyph = NULL;  // = (DrawGlyphProc)0x42F950;
static DrawGlyphProc gameExeDrawGlyphReal = NULL;

typedef unsigned int(__cdecl *Sg0DrawGlyph2Proc)(
	int textureId, int a2, float glyphInTextureStartX,
	float glyphInTextureStartY, float glyphInTextureWidth,
	float glyphInTextureHeight, float a7, float a8, float a9, float a10,
	float a11, float a12, float a13, float a14, signed int inColor,
	signed int opacity);
static Sg0DrawGlyph2Proc gameExeSg0DrawGlyph2 = NULL;
static Sg0DrawGlyph2Proc gameExeSg0DrawGlyph2Real = NULL;

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

typedef signed int(__cdecl *DrawSingleTextLineProc)(
    int textureId, int startX, signed int startY, unsigned int a4, char *string,
    signed int maxLength, int color, int glyphSize, signed int opacity);
static DrawSingleTextLineProc gameExeDrawSingleTextLine = NULL;
static DrawSingleTextLineProc gameExeDrawSingleTextLineReal = NULL;

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
static GetLinksFromSc3StringProc gameExeSghdGetLinksFromSc3String =
    NULL;  // = (GetLinksFromSc3StringProc)0x445EA0;
static GetLinksFromSc3StringProc gameExeSghdGetLinksFromSc3StringReal = NULL;

typedef int(__cdecl *DrawInteractiveMailProc)(
    int textureId, int xOffset, int yOffset, signed int lineLength,
    char *sc3string, unsigned int lineSkipCount, unsigned int lineDisplayCount,
    int color, unsigned int baseGlyphSize, int opacity, int unselectedLinkColor,
    int selectedLinkColor, int selectedLink);
static DrawInteractiveMailProc gameExeSghdDrawInteractiveMail =
    NULL;  // = (DrawInteractiveMailProc)0x4453D0;
static DrawInteractiveMailProc gameExeSghdDrawInteractiveMailReal = NULL;

typedef int(__cdecl *DrawLinkHighlightProc)(
    int xOffset, int yOffset, int lineLength, char *sc3string,
    unsigned int lineSkipCount, unsigned int lineDisplayCount, int color,
    unsigned int baseGlyphSize, int opacity, int selectedLink);
static DrawLinkHighlightProc gameExeSghdDrawLinkHighlight =
    NULL;  // = (DrawLinkHighlightProc)0x444B90;
static DrawLinkHighlightProc gameExeSghdDrawLinkHighlightReal = NULL;

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

static uintptr_t gameExeClearlistDrawRet1 = NULL;
static uintptr_t gameExeClearlistDrawRet2 = NULL;
static uintptr_t gameExeClearlistDrawRet3 = NULL;
static uintptr_t gameExeClearlistDrawRet4 = NULL;
static uintptr_t gameExeClearlistDrawRet5 = NULL;
static uintptr_t gameExeClearlistDrawRet6 = NULL;
static uintptr_t gameExeClearlistDrawRet7 = NULL;
static uintptr_t gameExeClearlistDrawRet8 = NULL;
static uintptr_t gameExeClearlistDrawRet9 = NULL;
static uintptr_t gameExeClearlistDrawRet10 = NULL;
static uintptr_t gameExeClearlistDrawRet11 = NULL;
static uintptr_t gameExeClearlistDrawRet12 = NULL;
static uintptr_t gameExeClearlistDrawRet13 = NULL;

static DialoguePage_t *gameExeDialoguePages =
    NULL;  // (DialoguePage_t *)0x164D680;

static uint8_t *gameExeGlyphWidthsFont1 = NULL;       // = (uint8_t *)0x52C7F0;
static uint8_t *gameExeGlyphWidthsFont2 = NULL;       // = (uint8_t *)0x52E058;
static int *gameExeColors = NULL;                     // = (int *)0x52E1E8;
static int8_t *gameExeBacklogHighlightHeight = NULL;  // = (int8_t *)0x435DD4;

static uint8_t widths[lb::TOTAL_NUM_FONT_CELLS];
static float SPLIT_FONT_OUTLINE_A_HEIGHT;

static std::string *fontBuffers[3] = {0};

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
signed int __cdecl drawSingleTextLineHook(int textureId, int startX,
                                          signed int startY, unsigned int a4,
                                          char *string, signed int maxLength,
                                          int color, int glyphSize,
                                          signed int opacity);
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
int __cdecl sghdGetLinksFromSc3StringHook(int xOffset, int yOffset,
                                          int lineLength, char *sc3string,
                                          int lineSkipCount,
                                          int lineDisplayCount,
                                          int baseGlyphSize,
                                          LinkMetrics_t *result);
int __cdecl sghdDrawInteractiveMailHook(
    int textureId, int xOffset, int yOffset, signed int lineLength,
    char *string, unsigned int lineSkipCount, unsigned int lineDisplayCount,
    int color, unsigned int glyphSize, int opacity, int unselectedLinkColor,
    int selectedLinkColor, int selectedLink);
int __cdecl sghdDrawLinkHighlightHook(int xOffset, int yOffset, int lineLength,
                                      char *sc3string,
                                      unsigned int lineSkipCount,
                                      unsigned int lineDisplayCount, int color,
                                      unsigned int baseGlyphSize, int opacity,
                                      int selectedLink);
int __cdecl getSc3StringLineCountHook(int lineLength, char *sc3string,
                                      unsigned int baseGlyphSize);
int __cdecl sg0DrawGlyphHook(int textureId, float glyphInTextureStartX,
	float glyphInTextureStartY,
	float glyphInTextureWidth,
	float glyphInTextureHeight, float displayStartX,
	float displayStartY, float displayEndX,
	float displayEndY, int color, uint32_t opacity);
unsigned int __cdecl sg0DrawGlyph2Hook(int textureId, int a2,
	float glyphInTextureStartX,
	float glyphInTextureStartY,
	float glyphInTextureWidth,
	float glyphInTextureHeight, float a7,
	float a8, float a9, float a10, float a11,
	float a12, float a13, float a14,
	signed int inColor, signed int opacity);
// There are a bunch more functions like these but I haven't seen them get hit
// during debugging and the original code *mostly* works okay if it recognises
// western text as variable-width
// (which some functions do, and others don't, except for symbols (also used in
// Western translations) it considers full-width)

void gameTextInit() {
  if (IMPROVE_DIALOGUE_OUTLINES) {
    {
      std::stringstream ss;
      ss << "languagebarrier\\"
         << config["patch"]["fontOutlineAFileName"].get<std::string>();
      slurpFile(ss.str(), fontBuffers);
      gameLoadTexture(OUTLINE_TEXTURE_ID, (void *)(fontBuffers[0]->c_str()),
                      fontBuffers[0]->size());
    }
    if (HAS_SPLIT_FONT) {
      std::stringstream ss;
      ss << "languagebarrier\\"
         << config["patch"]["fontOutlineBFileName"].get<std::string>();
      slurpFile(ss.str(), fontBuffers + 1);
      gameLoadTexture(OUTLINE_TEXTURE_ID + 1, (void *)(fontBuffers[1]->c_str()),
                      fontBuffers[1]->size());

	  float outlineRowHeightScaled = OUTLINE_CELL_HEIGHT * COORDS_MULTIPLIER;
	  SPLIT_FONT_OUTLINE_A_HEIGHT = floorf(4096 / outlineRowHeightScaled) * outlineRowHeightScaled;
    }
  }
  if (HAS_SPLIT_FONT) {
    std::stringstream ss;
    ss << "languagebarrier\\"
       << config["patch"]["fontBFileName"].get<std::string>();
    slurpFile(ss.str(), fontBuffers + 2);
    gameLoadTexture(FIRST_FONT_ID + 1, (void *)(fontBuffers[2]->c_str()),
                    fontBuffers[2]->size());
    // FONT2_B
    gameLoadTexture(FIRST_FONT_ID + 3, (void *)(fontBuffers[2]->c_str()),
                    fontBuffers[2]->size());
  }
  // the game loads these asynchronously - I'm not sure how to be notified it's
  // done and I can free the buffers
  // so I'll just do it in a hook

  if (config["gamedef"]["drawGlyphVersion"].get<std::string>() == "sg0") {
	  scanCreateEnableHook("game", "drawGlyph", (uintptr_t *)&gameExeDrawGlyph,
		  (LPVOID)sg0DrawGlyphHook,
		  (LPVOID *)&gameExeDrawGlyphReal);
	  scanCreateEnableHook(
		  "game", "sg0DrawGlyph2", (uintptr_t *)&gameExeSg0DrawGlyph2,
		  (LPVOID)sg0DrawGlyph2Hook, (LPVOID *)&gameExeSg0DrawGlyph2Real);
  }
  else {
	  // TODO (?): Split font support for non-sg0 drawGlyph
	  gameExeDrawGlyph = (DrawGlyphProc)sigScan("game", "drawGlyph");
  }
  gameExeDrawRectangle = (DrawRectangleProc)sigScan("game", "drawRectangle");
  gameExeSc3Eval = (Sc3EvalProc)sigScan("game", "sc3Eval");

  if (HAS_BACKLOG_UNDERLINE) {
    gameExeBacklogHighlightHeight =
        (int8_t *)sigScan("game", "backlogHighlightHeight");
    // gameExeBacklogHighlightHeight is (negative) offset (from vertical end of
    // glyph):
    // add eax,-0x22 (83 C0 DE) -> add eax,-0x17 (83 C0 E9)
    DWORD oldProtect;
    VirtualProtect(gameExeBacklogHighlightHeight, 1, PAGE_READWRITE,
                   &oldProtect);
    *gameExeBacklogHighlightHeight =
        BACKLOG_HIGHLIGHT_DEFAULT_HEIGHT + BACKLOG_HIGHLIGHT_HEIGHT_SHIFT;
    VirtualProtect(gameExeBacklogHighlightHeight, 1, oldProtect, &oldProtect);
  }

  if (HAS_RINE) {
    void *call;

    call = (void *)sigScan("game", "rineNameOutlineUpperCall");
    memset_perms(call, INST_NOP, INST_CALL_LEN);

    if (RINE_BLACK_NAMES) {
      call = (void *)sigScan("game", "rineNameOutlineLowerCall");
      memset_perms(call, INST_NOP, INST_CALL_LEN);

      int *color = (int *)sigScan("game", "rineNameColor");
      memset_perms(color, 0, sizeof(int));
    }

    call = (void *)sigScan("game", "rineMessageShadowCall");
    memset_perms(call, INST_NOP, INST_CALL_LEN);
  }

  gameExeGlyphWidthsFont1 = (uint8_t *)sigScan("game", "useOfGlyphWidthsFont1");
  gameExeGlyphWidthsFont2 = (uint8_t *)sigScan("game", "useOfGlyphWidthsFont2");
  gameExeColors = (int *)sigScan("game", "useOfColors");
  gameExeDialoguePages =
      (DialoguePage_t *)sigScan("game", "useOfDialoguePages");

  if (IMPROVE_DIALOGUE_OUTLINES) {
    scanCreateEnableHook(
        "game", "drawDialogue", (uintptr_t *)&gameExeDrawDialogue,
        (LPVOID)drawDialogueHook, (LPVOID *)&gameExeDrawDialogueReal);
    scanCreateEnableHook(
        "game", "drawDialogue2", (uintptr_t *)&gameExeDrawDialogue2,
        (LPVOID)drawDialogue2Hook, (LPVOID *)&gameExeDrawDialogue2Real);
  }
  scanCreateEnableHook("game", "dialogueLayoutRelated",
                       (uintptr_t *)&gameExeDialogueLayoutRelated,
                       (LPVOID)dialogueLayoutRelatedHook,
                       (LPVOID *)&gameExeDialogueLayoutRelatedReal);
  scanCreateEnableHook(
      "game", "drawPhoneText", (uintptr_t *)&gameExeDrawPhoneText,
      (LPVOID)drawPhoneTextHook, (LPVOID *)&gameExeDrawPhoneTextReal);
  if (NEEDS_CLEARLIST_TEXT_POSITION_ADJUST) {
    scanCreateEnableHook("game", "drawSingleTextLine",
                         (uintptr_t *)&gameExeDrawSingleTextLine,
                         (LPVOID)drawSingleTextLineHook,
                         (LPVOID *)&gameExeDrawSingleTextLineReal);
    gameExeClearlistDrawRet1 = sigScan("game", "clearlistDrawRet1");
    gameExeClearlistDrawRet2 = sigScan("game", "clearlistDrawRet2");
    gameExeClearlistDrawRet3 = sigScan("game", "clearlistDrawRet3");
    gameExeClearlistDrawRet4 = sigScan("game", "clearlistDrawRet4");
    gameExeClearlistDrawRet5 = sigScan("game", "clearlistDrawRet5");
    gameExeClearlistDrawRet6 = sigScan("game", "clearlistDrawRet6");
    gameExeClearlistDrawRet7 = sigScan("game", "clearlistDrawRet7");
    gameExeClearlistDrawRet8 = sigScan("game", "clearlistDrawRet8");
    gameExeClearlistDrawRet9 = sigScan("game", "clearlistDrawRet9");
    gameExeClearlistDrawRet10 = sigScan("game", "clearlistDrawRet10");
    gameExeClearlistDrawRet11 = sigScan("game", "clearlistDrawRet11");
    gameExeClearlistDrawRet12 = sigScan("game", "clearlistDrawRet12");
    gameExeClearlistDrawRet13 = sigScan("game", "clearlistDrawRet13");
  }
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
  if (HAS_SGHD_PHONE) {
    scanCreateEnableHook("game", "sghdGetLinksFromSc3String",
                         (uintptr_t *)&gameExeSghdGetLinksFromSc3String,
                         (LPVOID)sghdGetLinksFromSc3StringHook,
                         (LPVOID *)&gameExeSghdGetLinksFromSc3StringReal);
    scanCreateEnableHook("game", "sghdDrawInteractiveMail",
                         (uintptr_t *)&gameExeSghdDrawInteractiveMail,
                         (LPVOID)sghdDrawInteractiveMailHook,
                         (LPVOID *)&gameExeSghdDrawInteractiveMailReal);
    scanCreateEnableHook("game", "sghdDrawLinkHighlight",
                         (uintptr_t *)&gameExeSghdDrawLinkHighlight,
                         (LPVOID)sghdDrawLinkHighlightHook,
                         (LPVOID *)&gameExeSghdDrawLinkHighlightReal);
  }
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
  for (size_t i = 0; i < sizeof(fontBuffers) / sizeof(*fontBuffers); i++) {
    if (fontBuffers[i] != NULL) {
      // let's just do this here, should be loaded by now...
      delete fontBuffers[i];
      fontBuffers[i] = NULL;
    }
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
            OUTLINE_CELL_WIDTH * page->glyphCol[i] * COORDS_MULTIPLIER,
            OUTLINE_CELL_HEIGHT * page->glyphRow[i] * COORDS_MULTIPLIER,
            page->glyphOrigWidth[i] * COORDS_MULTIPLIER + (2 * OUTLINE_PADDING),
            page->glyphOrigHeight[i] * COORDS_MULTIPLIER +
                (2 * OUTLINE_PADDING),
            displayStartX - OUTLINE_PADDING, displayStartY - OUTLINE_PADDING,
            displayStartX + (COORDS_MULTIPLIER * page->glyphDisplayWidth[i]) +
                OUTLINE_PADDING,
            displayStartY + (COORDS_MULTIPLIER * page->glyphDisplayHeight[i]) +
                OUTLINE_PADDING,
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
  if (HAS_SGHD_PHONE) {
    lineLength -= 2 * SGHD_PHONE_X_PADDING;
  }

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
        uint16_t glyphWidth =
            (baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
        if (glyphId == GLYPH_ID_FULLWIDTH_SPACE ||
            glyphId == GLYPH_ID_HALFWIDTH_SPACE) {
          word.end = sc3string - 1;
          words.push_back(word);
          word = {sc3string, NULL, glyphWidth, true, false};
        } else {
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
  if (HAS_SGHD_PHONE) {
    xOffset += SGHD_PHONE_X_PADDING;
    lineLength -= 2 * SGHD_PHONE_X_PADDING;
  }

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
  // TODO: check if this is now fixed in SGHD
  // result->lines++;

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

signed int drawSingleTextLineHook(int textureId, int startX, signed int startY,
                                  unsigned int a4, char *string,
                                  signed int maxLength, int color,
                                  int glyphSize, signed int opacity) {
  // yolo
  if (NEEDS_CLEARLIST_TEXT_POSITION_ADJUST) {
    uintptr_t retaddr;
    __asm {
		push eax
		mov eax, [ebp + 4]
		mov retaddr, eax
		pop eax
    }
    if (retaddr == gameExeClearlistDrawRet1 ||
        retaddr == gameExeClearlistDrawRet2 ||
        retaddr == gameExeClearlistDrawRet3 ||
        retaddr == gameExeClearlistDrawRet4 ||
        retaddr == gameExeClearlistDrawRet5 ||
        retaddr == gameExeClearlistDrawRet6) {
      startY += 32;
      startX -= 264;
    }
    else if (retaddr == gameExeClearlistDrawRet7 ||
             retaddr == gameExeClearlistDrawRet8 ||
             retaddr == gameExeClearlistDrawRet9) {
      startY += 32;
      startX -= 192;
    }
    else if (retaddr == gameExeClearlistDrawRet10 ||
             retaddr == gameExeClearlistDrawRet11 ||
             retaddr == gameExeClearlistDrawRet12 ||
             retaddr == gameExeClearlistDrawRet13) {
      startY += 32;
      startX -= 150;
    }
  }
  return gameExeDrawSingleTextLineReal(textureId, startX, startY, a4, string,
                                       maxLength, color, glyphSize, opacity);
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

int __cdecl sghdGetLinksFromSc3StringHook(int xOffset, int yOffset,
                                          int lineLength, char *sc3string,
                                          int lineSkipCount,
                                          int lineDisplayCount,
                                          int baseGlyphSize,
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
int __cdecl sghdDrawInteractiveMailHook(
    int textureId, int xOffset, int yOffset, signed int lineLength,
    char *sc3string, unsigned int lineSkipCount, unsigned int lineDisplayCount,
    int color, unsigned int baseGlyphSize, int opacity, int unselectedLinkColor,
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

      gameExeDrawGlyph(textureId, SGHD_LINK_UNDERLINE_GLYPH_X,
                       SGHD_LINK_UNDERLINE_GLYPH_Y, str.textureWidth[i],
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

int __cdecl sghdDrawLinkHighlightHook(int xOffset, int yOffset, int lineLength,
                                      char *sc3string,
                                      unsigned int lineSkipCount,
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
int sg0DrawGlyphHook(int textureId, float glyphInTextureStartX,
	float glyphInTextureStartY, float glyphInTextureWidth,
	float glyphInTextureHeight, float displayStartX,
	float displayStartY, float displayEndX, float displayEndY,
	int color, uint32_t opacity) {

	if (!HAS_SPLIT_FONT)
	{
		if (glyphInTextureStartY > 4080.0) {
			glyphInTextureStartY += 4080.0;
			--textureId;
		}
	}
	else if (textureId == OUTLINE_TEXTURE_ID) {
		float origStartY = glyphInTextureStartY;
		// undo the game's splitting
		if (glyphInTextureStartY > 4080.0) {
			glyphInTextureStartY += 4080.0;
			--textureId;
		}
		// split it ourselves
		if (origStartY >= SPLIT_FONT_OUTLINE_A_HEIGHT) {
			glyphInTextureStartY -= SPLIT_FONT_OUTLINE_A_HEIGHT;
			++textureId;
		}
	}
	return gameExeDrawGlyphReal(
		textureId, glyphInTextureStartX, glyphInTextureStartY,
		glyphInTextureWidth, glyphInTextureHeight, displayStartX, displayStartY,
		displayEndX, displayEndY, color, opacity);
}
unsigned int sg0DrawGlyph2Hook(int textureId, int a2,
	float glyphInTextureStartX,
	float glyphInTextureStartY,
	float glyphInTextureWidth,
	float glyphInTextureHeight, float a7, float a8,
	float a9, float a10, float a11, float a12,
	float a13, float a14, signed int inColor,
	signed int opacity) {
	if (!HAS_SPLIT_FONT)
	{
		if (glyphInTextureStartY > 4080.0) {
			glyphInTextureStartY += 4080.0;
			--textureId;
		}
	}
	else if (textureId == OUTLINE_TEXTURE_ID) {
		float origStartY = glyphInTextureStartY;
		// undo the game's splitting
		if (glyphInTextureStartY > 4080.0) {
			glyphInTextureStartY += 4080.0;
			--textureId;
		}
		// split it ourselves
		if (origStartY >= SPLIT_FONT_OUTLINE_A_HEIGHT) {
			glyphInTextureStartY -= SPLIT_FONT_OUTLINE_A_HEIGHT;
			++textureId;
		}
	}
	return gameExeSg0DrawGlyph2Real(textureId, a2, glyphInTextureStartX,
		glyphInTextureStartY, glyphInTextureWidth,
		glyphInTextureHeight, a7, a8, a9, a10, a11,
		a12, a13, a14, inColor, opacity);
}
}