#include "GameText.h"
#include <fstream>
#include <list>
#include <sstream>
#include <vector>
#include <intrin.h>
#include "Config.h"
#include "Game.h"
#include "LanguageBarrier.h"
#include "Script.h"
#include "SigScan.h"

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
  int usedLineLength;
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

#define DEF_DIALOGUE_PAGE(name, size) \
  typedef struct {                    \
    int field_0;                      \
    int field_4;                      \
    int drawNextPageNow;              \
    int pageLength;                   \
    int field_10;                     \
    char field_14;                    \
    char field_15;                    \
    char field_16;                    \
    char field_17;                    \
    int field_18;                     \
    int field_1C;                     \
    int field_20;                     \
    int field_24;                     \
    int field_28;                     \
    int field_2C;                     \
    int field_30;                     \
    int field_34;                     \
    int field_38;                     \
    int fontNumber[size];             \
    int charColor[size];              \
    int charOutlineColor[size];       \
    char glyphCol[size];              \
    char glyphRow[size];              \
    char glyphOrigWidth[size];        \
    char glyphOrigHeight[size];       \
    __int16 charDisplayX[size];       \
    __int16 charDisplayY[size];       \
    __int16 glyphDisplayWidth[size];  \
    __int16 glyphDisplayHeight[size]; \
    char field_BBBC[size];            \
    int field_C38C[size];             \
    char charDisplayOpacity[size];    \
  } name;                             \
  static name *gameExeDialoguePages_##name = NULL;
DEF_DIALOGUE_PAGE(DialoguePage_t, 2000);
DEF_DIALOGUE_PAGE(CCDialoguePage_t, 600);

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
    float a11, float a12, signed int inColor, signed int opacity, int *a15,
    int *a16);
static Sg0DrawGlyph2Proc gameExeSg0DrawGlyph2 = NULL;
static Sg0DrawGlyph2Proc gameExeSg0DrawGlyph2Real = NULL;

typedef int(__cdecl *DrawRectangleProc)(float X, float Y, float width,
                                        float height, int color,
                                        uint32_t opacity);
static DrawRectangleProc gameExeDrawRectangle =
    NULL;  // = (DrawRectangleProc)0x42F890;

typedef int(__cdecl *DrawSpriteProc)(int textureId, float spriteX,
                                     float spriteY, float spriteWidth,
                                     float spriteHeight, float displayX,
                                     float displayY, int color, int opacity,
                                     int shaderId);
static DrawSpriteProc gameExeDrawSprite =
    NULL;  // = (DrawSpriteProc)0x431280; (CHAOS;CHILD)
static DrawSpriteProc gameExeDrawSpriteReal = NULL;

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

typedef int(__cdecl *GetRineInputRectangleProc)(int* lineLength, char *text, unsigned int baseGlyphSize);
static GetRineInputRectangleProc gameExeGetRineInputRectangle = NULL;
static GetRineInputRectangleProc gameExeGetRineInputRectangleReal = NULL;

typedef int(__cdecl *SetTipContentProc)(char *sc3string);
static SetTipContentProc gameExeSetTipContent =
    NULL;  // = (SetTipContentProc)0x44FB20;
static SetTipContentProc gameExeSetTipContentReal = NULL;

typedef void(__cdecl *DrawTipContentProc)(int textureId, int maskId, int startX,
                                          int startY, int maskStartY,
                                          int maskHeight, int a7, int color,
                                          int shadowColor, int opacity);
static DrawTipContentProc gameExeDrawTipContent =
    NULL;  // = (DrawTipContentProc)0x44FB70;
static DrawTipContentProc gameExeDrawTipContentReal = NULL;

static uintptr_t gameExeDialogueLayoutWidthLookup1 = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup1Return = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup2 = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup2Return = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup3 = NULL;
static uintptr_t gameExeDialogueLayoutWidthLookup3Return = NULL;
static uintptr_t gameExeTipsListWidthLookup = NULL;
static uintptr_t gameExeTipsListWidthLookupReturn = NULL;

typedef struct {
  int dx, dy;
  int fontSize;
} SingleLineOffset_t;
static std::map<uintptr_t, SingleLineOffset_t> retAddrToSingleLineFixes;

typedef struct {
  float dx, dy;
  float width, height;
  float srcDx, srcDy;
} SpriteFix_t;
static std::map<uintptr_t, SpriteFix_t> retAddrToSpriteFixes;

static uintptr_t gameExeCcBacklogNamePosCode = NULL;       // = 0x00454FE9
static uintptr_t gameExeCcBacklogNamePosAdjustRet = NULL;  // = 0x00454FEF

static uint8_t *gameExeGlyphWidthsFont1 = NULL;       // = (uint8_t *)0x52C7F0;
static uint8_t *gameExeGlyphWidthsFont2 = NULL;       // = (uint8_t *)0x52E058;
static int *gameExeColors = NULL;                     // = (int *)0x52E1E8;
static int8_t *gameExeBacklogHighlightHeight = NULL;  // = (int8_t *)0x435DD4;

static int *gameExeCcBacklogCurLine =
    NULL;  // = (int*)0x017F9EF8; (CHAOS;CHILD)
static int *gameExeCcBacklogLineHeights =
    NULL;  // = (int*)0x017FA560; (CHAOS;CHILD)
static void *gameExeCcBacklogHighlightDrawRet = NULL;

static uint8_t widths[lb::TOTAL_NUM_FONT_CELLS];
static float SPLIT_FONT_OUTLINE_A_HEIGHT;

static std::string *fontBuffers[3] = {0};

static char *tipContent;

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

__declspec(naked) void tipsListWidthLookupHook() {
  __asm {
    movzx eax, widths[edx]
    jmp gameExeTipsListWidthLookupReturn
  }
}

__declspec(naked) void ccBacklogNamePosAdjustHook() {
  __asm {
    // copied code
    mov [edi+4],eax
    mov edi, [ebp+0x14]

    // ecx is y pos
    add ecx, [lb::DIALOGUE_REDESIGN_YOFFSET_SHIFT]

    jmp gameExeCcBacklogNamePosAdjustRet
  }
}

__declspec(naked) void ccSteamBacklogNamePosAdjustHook() {
  __asm {
    // copied code
        mov [ebx + 4], eax

        // eax is y pos
        mov eax, [ebp - 0x1AC]
        add eax, [lb::DIALOGUE_REDESIGN_YOFFSET_SHIFT]

        jmp gameExeCcBacklogNamePosAdjustRet
  }
}

namespace lb {
void __cdecl drawDialogueHook(int fontNumber, int pageNumber, uint32_t opacity,
                              int xOffset, int yOffset);
void __cdecl drawDialogue2Hook(int fontNumber, int pageNumber,
                               uint32_t opacity);
void __cdecl ccDrawDialogueHook(int fontNumber, int pageNumber,
                                uint32_t opacity, int xOffset, int yOffset);
void __cdecl ccDrawDialogue2Hook(int fontNumber, int pageNumber,
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
int __cdecl getRineInputRectangleHook(int* lineLength, char *text, unsigned int baseGlyphSize);
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
                                       float a12, signed int inColor,
                                       signed int opacity, int *a15, int *a16);
int __cdecl setTipContentHook(char *sc3string);
void __cdecl drawTipContentHook(int textureId, int maskId, int startX,
                                int startY, int maskStartY, int maskHeight,
                                int a7, int color, int shadowColor,
                                int opacity);
int __cdecl drawSpriteHook(int textureId, float spriteX, float spriteY,
                           float spriteWidth, float spriteHeight,
                           float displayX, float displayY, int color,
                           int opacity, int shaderId);
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
      SPLIT_FONT_OUTLINE_A_HEIGHT =
          floorf(4096 / outlineRowHeightScaled) * outlineRowHeightScaled;
    }
  }
  if (HAS_SPLIT_FONT && config["patch"].count("fontBFileName") == 1) {
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
  } else {
    // TODO (?): Split font support for non-sg0 drawGlyph
    gameExeDrawGlyph = (DrawGlyphProc)sigScan("game", "drawGlyph");
  }
  gameExeDrawRectangle = (DrawRectangleProc)sigScan("game", "drawRectangle");

  if (HAS_BACKLOG_UNDERLINE) {
    gameExeBacklogHighlightHeight =
        (int8_t *)sigScan("game", "backlogHighlightHeight");
    // gameExeBacklogHighlightHeight is (negative) offset (from vertical end of
    // glyph):
    // add eax,-0x22 (83 C0 DE) -> add eax,-0x17 (83 C0 E9)
    memset_perms(
        gameExeBacklogHighlightHeight,
        BACKLOG_HIGHLIGHT_DEFAULT_HEIGHT + BACKLOG_HIGHLIGHT_HEIGHT_SHIFT, 1);
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
  if (config["gamedef"].count("dialoguePageVersion") == 1 &&
      config["gamedef"]["dialoguePageVersion"].get<std::string>() == "cc") {
    gameExeDialoguePages_CCDialoguePage_t =
        (CCDialoguePage_t *)sigScan("game", "useOfDialoguePages");
    if (IMPROVE_DIALOGUE_OUTLINES) {
      scanCreateEnableHook(
          "game", "drawDialogue", (uintptr_t *)&gameExeDrawDialogue,
          (LPVOID)ccDrawDialogueHook, (LPVOID *)&gameExeDrawDialogueReal);
      scanCreateEnableHook(
          "game", "drawDialogue2", (uintptr_t *)&gameExeDrawDialogue2,
          (LPVOID)ccDrawDialogue2Hook, (LPVOID *)&gameExeDrawDialogue2Real);
    }
  } else {
    gameExeDialoguePages_DialoguePage_t =
        (DialoguePage_t *)sigScan("game", "useOfDialoguePages");
    if (IMPROVE_DIALOGUE_OUTLINES) {
      scanCreateEnableHook(
          "game", "drawDialogue", (uintptr_t *)&gameExeDrawDialogue,
          (LPVOID)drawDialogueHook, (LPVOID *)&gameExeDrawDialogueReal);
      scanCreateEnableHook(
          "game", "drawDialogue2", (uintptr_t *)&gameExeDrawDialogue2,
          (LPVOID)drawDialogue2Hook, (LPVOID *)&gameExeDrawDialogue2Real);
    }
  }

  scanCreateEnableHook("game", "dialogueLayoutRelated",
                       (uintptr_t *)&gameExeDialogueLayoutRelated,
                       (LPVOID)dialogueLayoutRelatedHook,
                       (LPVOID *)&gameExeDialogueLayoutRelatedReal);
  if (HAS_DRAW_PHONE_TEXT) {
    scanCreateEnableHook(
        "game", "drawPhoneText", (uintptr_t *)&gameExeDrawPhoneText,
        (LPVOID)drawPhoneTextHook, (LPVOID *)&gameExeDrawPhoneTextReal);
  }
  // compatibility with old configs
  if (NEEDS_CLEARLIST_TEXT_POSITION_ADJUST) {
    static const char clearlistConfigText[] = "["
      "{\"sigName\":\"clearlistDrawRet1\",\"dx\":-264,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet2\",\"dx\":-264,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet3\",\"dx\":-264,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet4\",\"dx\":-264,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet5\",\"dx\":-264,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet6\",\"dx\":-264,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet7\",\"dx\":-192,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet8\",\"dx\":-192,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet9\",\"dx\":-192,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet10\",\"dx\":-150,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet11\",\"dx\":-150,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet12\",\"dx\":-150,\"dy\":32},"
      "{\"sigName\":\"clearlistDrawRet13\",\"dx\":-150,\"dy\":32}"
      "]";
    const json clearlistConfig = json::parse(clearlistConfigText);
    json& arr = config["patch"]["singleTextLineFixes"];
    if (!arr.is_array())
      arr = json::array();
    arr.insert(arr.end(), clearlistConfig.begin(), clearlistConfig.end());
  }
  const auto& singleTextLineFixes = config["patch"].find("singleTextLineFixes");
  if (singleTextLineFixes != config["patch"].end() && singleTextLineFixes->is_array()) {
    scanCreateEnableHook("game", "drawSingleTextLine",
                         (uintptr_t *)&gameExeDrawSingleTextLine,
                         (LPVOID)drawSingleTextLineHook,
                         (LPVOID *)&gameExeDrawSingleTextLineReal);
    for (const json& item : *singleTextLineFixes) {
      if (!item.is_object())
        continue;
      auto sigNameIter = item.find("sigName");
      if (sigNameIter == item.end())
        continue;
      if (!sigNameIter->is_string())
        continue;
      const std::string& sigName = sigNameIter->get<std::string>();
      uintptr_t targetPtr = sigScan("game", sigName.c_str());
      if (!targetPtr)
        continue;
      SingleLineOffset_t& fix = retAddrToSingleLineFixes[targetPtr];
      auto iter = item.find("dx");
      if (iter != item.end() && iter->is_number_integer())
        fix.dx = iter->get<int>();
      else
        fix.dx = 0;
      iter = item.find("dy");
      if (iter != item.end() && iter->is_number_integer())
        fix.dy = iter->get<int>();
      else
        fix.dy = 0;
      iter = item.find("fontSize");
      if (iter != item.end() && iter->is_number_integer())
        fix.fontSize = iter->get<int>();
      else
        fix.fontSize = 0;
    }
  }
  const auto& spriteFixes = config["patch"].find("spriteFixes");
  if (spriteFixes != config["patch"].end() && spriteFixes->is_array()) {
    for (const json& item : *spriteFixes) {
      if (!item.is_object())
        continue;
      auto sigNameIter = item.find("sigName");
      if (sigNameIter == item.end() || !sigNameIter->is_string())
        continue;
      const std::string& sigName = sigNameIter->get<std::string>();
      uintptr_t targetPtr = sigScan("game", sigName.c_str());
      if (!targetPtr)
        continue;
      SpriteFix_t& fix = retAddrToSpriteFixes[targetPtr];
      const struct { const char* name; float* ptr; } name2var[] = {
        {"dx", &fix.dx},
        {"dy", &fix.dy},
        {"width", &fix.width},
        {"height", &fix.height},
        {"srcDx", &fix.srcDx},
        {"srcDy", &fix.srcDy},
      };
      for (size_t i = 0; i < sizeof(name2var) / sizeof(name2var[0]); i++) {
        auto it = item.find(name2var[i].name);
        if (it != item.end() && it->is_number())
          *name2var[i].ptr = it->get<float>();
        else
          *name2var[i].ptr = 0;
      }
    }
  }
  if (NEEDS_CC_BACKLOG_NAME_POS_ADJUST) {
    gameExeCcBacklogNamePosAdjustRet =
        sigScan("game", "ccBacklogNamePosAdjustRet");
    void *target;
    if (config["gamedef"].count("ccBacklogNamePosAdjustVersion") == 1 &&
        config["gamedef"]["ccBacklogNamePosAdjustVersion"].get<std::string>() ==
            "ccsteam") {
      target = ccSteamBacklogNamePosAdjustHook;
    } else {
      target = ccBacklogNamePosAdjustHook;
    }
    scanCreateEnableHook("game", "ccBacklogNamePosCode",
                         (uintptr_t *)&gameExeCcBacklogNamePosCode, target,
                         NULL);
  }
  // The following both have the same pattern and 'occurrence: 0' in the
  // signatures.json.
  // That's because after you hook one, the first match goes away.
  scanCreateEnableHook("game", "getSc3StringDisplayWidthFont1",
                       (uintptr_t *)&gameExeGetSc3StringDisplayWidthFont1,
                       (LPVOID)getSc3StringDisplayWidthHook,
                       (LPVOID *)&gameExeGetSc3StringDisplayWidthFont1Real);

  if (HAS_DOUBLE_GET_SC3_STRING_DISPLAY_WIDTH) {
    scanCreateEnableHook("game", "getSc3StringDisplayWidthFont2",
                         (uintptr_t *)&gameExeGetSc3StringDisplayWidthFont2,
                         (LPVOID)getSc3StringDisplayWidthHook,
                         (LPVOID *)&gameExeGetSc3StringDisplayWidthFont2Real);
  }
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
  if (HAS_GET_SC3_STRING_LINE_COUNT) {
    scanCreateEnableHook("game", "getSc3StringLineCount",
                         (uintptr_t *)&gameExeGetSc3StringLineCount,
                         (LPVOID)getSc3StringLineCountHook,
                         (LPVOID *)&gameExeGetSc3StringLineCountReal);
  }
  if (TIP_REIMPL) {
    scanCreateEnableHook(
        "game", "setTipContent", (uintptr_t *)&gameExeSetTipContent,
        (LPVOID)setTipContentHook, (LPVOID *)&gameExeSetTipContentReal);
    scanCreateEnableHook(
        "game", "drawTipContent", (uintptr_t *)&gameExeDrawTipContent,
        (LPVOID)drawTipContentHook, (LPVOID *)&gameExeDrawTipContentReal);
  }
  if (CC_BACKLOG_HIGHLIGHT || !retAddrToSpriteFixes.empty()) {
    scanCreateEnableHook("game", "drawSprite", (uintptr_t *)&gameExeDrawSprite,
                         (LPVOID)drawSpriteHook,
	                 (LPVOID *)&gameExeDrawSpriteReal);
  }
  if (CC_BACKLOG_HIGHLIGHT) {
    gameExeCcBacklogCurLine = (int *)sigScan("game", "useOfCcBacklogCurLine");
    gameExeCcBacklogLineHeights =
        (int *)sigScan("game", "useOfCcBacklogLineHeights");
    gameExeCcBacklogHighlightDrawRet =
        (void *)sigScan("game", "ccBacklogHighlightDrawRet");
  }

  ptrdiff_t lookup1retoffset;
  ptrdiff_t lookup2retoffset;
  ptrdiff_t lookup3retoffset;
  ptrdiff_t tipsListWidthRetoffset = 0x14;
  if (config["gamedef"].count("dialogueLayoutWidthLookupRetOffsets") == 1 &&
      config["gamedef"]["dialogueLayoutWidthLookupRetOffsets"]
              .get<std::string>() == "ccsteam") {
    lookup1retoffset = 0x2B;
    lookup2retoffset = 0x10;
    lookup3retoffset = 0x7;
  } else {
    lookup1retoffset = 0x27;
    lookup2retoffset = 0x12;
    lookup3retoffset = 0x7;
  }

  const json& signatures = config["gamedef"]["signatures"]["game"];
  int configretoffset = signatures["dialogueLayoutWidthLookup1"].value<int>("return", 0);
  if (configretoffset)
    lookup1retoffset = configretoffset;
  configretoffset = signatures["dialogueLayoutWidthLookup2"].value<int>("return", 0);
  if (configretoffset)
    lookup2retoffset = configretoffset;
  configretoffset = signatures["dialogueLayoutWidthLookup3"].value<int>("return", 0);
  if (configretoffset)
    lookup3retoffset = configretoffset;

  scanCreateEnableHook("game", "dialogueLayoutWidthLookup1",
                       &gameExeDialogueLayoutWidthLookup1,
                       dialogueLayoutWidthLookup1Hook, NULL);
  // we should have used the expression parser for these but oh well
  gameExeDialogueLayoutWidthLookup1Return = (uintptr_t)(
      (uint8_t *)gameExeDialogueLayoutWidthLookup1 + lookup1retoffset);
  scanCreateEnableHook("game", "dialogueLayoutWidthLookup2",
                       &gameExeDialogueLayoutWidthLookup2,
                       dialogueLayoutWidthLookup2Hook, NULL);
  gameExeDialogueLayoutWidthLookup2Return = (uintptr_t)(
      (uint8_t *)gameExeDialogueLayoutWidthLookup2 + lookup2retoffset);
  scanCreateEnableHook("game", "dialogueLayoutWidthLookup3",
                       &gameExeDialogueLayoutWidthLookup3,
                       dialogueLayoutWidthLookup3Hook, NULL);
  gameExeDialogueLayoutWidthLookup3Return = (uintptr_t)(
      (uint8_t *)gameExeDialogueLayoutWidthLookup3 + lookup3retoffset);
  if (signatures.count("tipsListWidthLookup") == 1) {
    configretoffset = signatures["tipsListWidthLookup"].value<int>("return", 0);
    if (configretoffset)
      tipsListWidthRetoffset = configretoffset;
    scanCreateEnableHook("game", "tipsListWidthLookup",
                         &gameExeTipsListWidthLookup,
                         tipsListWidthLookupHook, NULL);
    gameExeTipsListWidthLookupReturn =
        (uintptr_t)((uint8_t *)gameExeTipsListWidthLookup + tipsListWidthRetoffset);
  }
  if (signatures.count("getRineInputRectangle") == 1) {
    scanCreateEnableHook("game", "getRineInputRectangle",
                         (uintptr_t *)&gameExeGetRineInputRectangle,
                         (LPVOID)getRineInputRectangleHook,
                         (LPVOID *)&gameExeGetRineInputRectangleReal);
  }

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

#define DEF_DRAW_DIALOGUE_HOOK(funcName, pageType)                             \
                                                                               \
  void __cdecl funcName(int fontNumber, int pageNumber, uint32_t opacity,      \
                        int xOffset, int yOffset) {                            \
    pageType *page = &gameExeDialoguePages_##pageType[pageNumber];             \
                                                                               \
    for (int i = 0; i < page->pageLength; i++) {                               \
      if (fontNumber == page->fontNumber[i]) {                                 \
        int displayStartX =                                                    \
            (page->charDisplayX[i] + xOffset) * COORDS_MULTIPLIER;             \
        int displayStartY =                                                    \
            (page->charDisplayY[i] + yOffset) * COORDS_MULTIPLIER;             \
                                                                               \
        uint32_t _opacity = (page->charDisplayOpacity[i] * opacity) >> 8;      \
                                                                               \
        if (page->charOutlineColor[i] != -1) {                                 \
          gameExeDrawGlyph(                                                    \
              OUTLINE_TEXTURE_ID,                                              \
              OUTLINE_CELL_WIDTH * page->glyphCol[i] * COORDS_MULTIPLIER,      \
              OUTLINE_CELL_HEIGHT * page->glyphRow[i] * COORDS_MULTIPLIER,     \
              page->glyphOrigWidth[i] * COORDS_MULTIPLIER +                    \
                  (2 * OUTLINE_PADDING),                                       \
              page->glyphOrigHeight[i] * COORDS_MULTIPLIER +                   \
                  (2 * OUTLINE_PADDING),                                       \
              displayStartX - OUTLINE_PADDING,                                 \
              displayStartY - OUTLINE_PADDING,                                 \
              displayStartX +                                                  \
                  (COORDS_MULTIPLIER * page->glyphDisplayWidth[i]) +           \
                  OUTLINE_PADDING,                                             \
              displayStartY +                                                  \
                  (COORDS_MULTIPLIER * page->glyphDisplayHeight[i]) +          \
                  OUTLINE_PADDING,                                             \
              page->charOutlineColor[i], _opacity);                            \
        }                                                                      \
                                                                               \
        gameExeDrawGlyph(                                                      \
            fontNumber + FIRST_FONT_ID,                                        \
            FONT_CELL_WIDTH * page->glyphCol[i] * COORDS_MULTIPLIER,           \
            FONT_CELL_HEIGHT * page->glyphRow[i] * COORDS_MULTIPLIER,          \
            page->glyphOrigWidth[i] * COORDS_MULTIPLIER,                       \
            page->glyphOrigHeight[i] * COORDS_MULTIPLIER, displayStartX,       \
            displayStartY,                                                     \
            displayStartX + (COORDS_MULTIPLIER * page->glyphDisplayWidth[i]),  \
            displayStartY + (COORDS_MULTIPLIER * page->glyphDisplayHeight[i]), \
            page->charColor[i], _opacity);                                     \
      }                                                                        \
    }                                                                          \
  }
DEF_DRAW_DIALOGUE_HOOK(drawDialogueHook, DialoguePage_t);
DEF_DRAW_DIALOGUE_HOOK(ccDrawDialogueHook, CCDialoguePage_t);

void __cdecl drawDialogue2Hook(int fontNumber, int pageNumber,
                               uint32_t opacity) {
  // dunno if this is ever actually called but might as well
  drawDialogueHook(fontNumber, pageNumber, opacity, 0, 0);
}
void __cdecl ccDrawDialogue2Hook(int fontNumber, int pageNumber,
                                 uint32_t opacity) {
  ccDrawDialogueHook(fontNumber, pageNumber, opacity, 0, 0);
}

void semiTokeniseSc3String(char *sc3string, std::list<StringWord_t> &words,
                           int baseGlyphSize, int lineLength) {
  if (HAS_SGHD_PHONE) {
    lineLength -= 2 * SGHD_PHONE_X_PADDING;
  }

  ScriptThreadState sc3;
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
        sc3.pc = sc3string + 1;
        gameExeSc3Eval(&sc3, &sc3evalResult);
        sc3string = (char *)sc3.pc;
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
  ScriptThreadState sc3;
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
  int prevLineLength = 0;

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
      prevLineLength = curLineLength;
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
          sc3.pc = sc3string + 1;
          gameExeSc3Eval(&sc3, &sc3evalResult);
          sc3string = (char *)sc3.pc;
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
      prevLineLength = curLineLength;
      curLineLength = 0;
    }
  }

  if (curLineLength == 0) result->lines--;
  // TODO: check if this is now fixed in SGHD
  // result->lines++;

  result->linkCount = lastLinkNumber + 1;
  result->curColor = currentColor;
  result->curLinkNumber = curLinkNumber;
  result->usedLineLength = curLineLength ? curLineLength : prevLineLength;
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
  uintptr_t retaddr;
  __asm {
      push eax
      mov eax, [ebp + 4]
      mov retaddr, eax
      pop eax
  }
  auto fixIter = retAddrToSingleLineFixes.find(retaddr);
  if (fixIter != retAddrToSingleLineFixes.end()) {
    startX += fixIter->second.dx;
    startY += fixIter->second.dy;
    if (fixIter->second.fontSize)
      glyphSize = fixIter->second.fontSize;
  }
  return gameExeDrawSingleTextLineReal(textureId, startX, startY, a4, string,
                                       maxLength, color, glyphSize, opacity);
}

int __cdecl getSc3StringDisplayWidthHook(char *sc3string,
                                         unsigned int maxCharacters,
                                         int baseGlyphSize) {
  if (!maxCharacters) maxCharacters = DEFAULT_MAX_CHARACTERS;
  ScriptThreadState sc3;
  int sc3evalResult;
  int result = 0;
  int i = 0;
  signed char c;
  while (i <= maxCharacters && (c = *sc3string) != -1) {
    if (c == 4) {
      sc3.pc = sc3string + 1;
      gameExeSc3Eval(&sc3, &sc3evalResult);
      sc3string = (char *)sc3.pc;
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
int __cdecl getRineInputRectangleHook(int* lineLength, char* text, unsigned int baseGlyphSize) {
  ProcessedSc3String_t str;
  int maxLineLength = (lineLength && *lineLength ? *lineLength : DEFAULT_LINE_LENGTH);

  std::list<StringWord_t> words;
  semiTokeniseSc3String(text, words, baseGlyphSize, maxLineLength);
  processSc3TokenList(0, 0, maxLineLength, words, LINECOUNT_DISABLE_OR_ERROR, 0,
                      baseGlyphSize, &str, true, 1.0f, -1, NOT_A_LINK, 0);
  *lineLength = str.usedLineLength;
  return str.lines + 1;
}
int sg0DrawGlyphHook(int textureId, float glyphInTextureStartX,
                     float glyphInTextureStartY, float glyphInTextureWidth,
                     float glyphInTextureHeight, float displayStartX,
                     float displayStartY, float displayEndX, float displayEndY,
                     int color, uint32_t opacity) {
  if (!HAS_SPLIT_FONT) {
    if (glyphInTextureStartY > 4080.0) {
      glyphInTextureStartY += 4080.0;
      --textureId;
    }
  } else if (textureId == OUTLINE_TEXTURE_ID) {
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
                               signed int inColor, signed int opacity, int *a15,
                               int *a16) {
  if (!HAS_SPLIT_FONT) {
    if (glyphInTextureStartY > 4080.0) {
      glyphInTextureStartY += 4080.0;
      --textureId;
    }
  } else if (textureId == OUTLINE_TEXTURE_ID) {
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
                                  a12, inColor, opacity, a15, a16);
}
int setTipContentHook(char *sc3string) {
  tipContent = sc3string;

  ProcessedSc3String_t str;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(tipContent, words, TIP_REIMPL_GLYPH_SIZE,
                        TIP_REIMPL_LINE_LENGTH);
  processSc3TokenList(0, 0, TIP_REIMPL_LINE_LENGTH, words, 255, 0,
                      TIP_REIMPL_GLYPH_SIZE, &str, false, 1, -1, NOT_A_LINK, 0);

  return str.displayEndY[str.length - 1];  // scroll height
}
void drawTipContentHook(int textureId, int maskId, int startX, int startY,
                        int maskStartY, int maskHeight, int a7, int color,
                        int shadowColor, int opacity) {
  ProcessedSc3String_t str;

  int dummy1;
  int dummy2;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(tipContent, words, TIP_REIMPL_GLYPH_SIZE,
                        TIP_REIMPL_LINE_LENGTH);
  processSc3TokenList(startX, startY, TIP_REIMPL_LINE_LENGTH, words, 255, color,
                      TIP_REIMPL_GLYPH_SIZE, &str, false, COORDS_MULTIPLIER, -1,
                      NOT_A_LINK, color);

  for (int i = 0; i < str.length; i++) {
    if (str.displayStartY[i] / COORDS_MULTIPLIER > maskStartY &&
        str.displayEndY[i] / COORDS_MULTIPLIER < (maskStartY + maskHeight)) {
      gameExeSg0DrawGlyph2(
          textureId, maskId, str.textureStartX[i], str.textureStartY[i],
          str.textureWidth[i], str.textureHeight[i],
          ((float)str.displayStartX[i] + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayStartY[i] + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayStartX[i] + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayStartY[i] +
           ((1.0f + (float)a7) * COORDS_MULTIPLIER)),
          ((float)str.displayEndX[i] + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayEndY[i] +
           ((1.0f + (float)a7) * COORDS_MULTIPLIER)),
          shadowColor, opacity, &dummy1, &dummy2);

      gameExeSg0DrawGlyph2(
          textureId, maskId, str.textureStartX[i], str.textureStartY[i],
          str.textureWidth[i], str.textureHeight[i], str.displayStartX[i],
          str.displayStartY[i], str.displayStartX[i],
          ((float)str.displayStartY[i] + ((float)a7 * COORDS_MULTIPLIER)),
          str.displayEndX[i],
          ((float)str.displayEndY[i] + ((float)a7 * COORDS_MULTIPLIER)),
          str.color[i], opacity, &dummy1, &dummy2);
    }
  }
}
int drawSpriteHook(int textureId, float spriteX, float spriteY,
                   float spriteWidth, float spriteHeight, float displayX,
                   float displayY, int color, int opacity, int shaderId) {
  if (CC_BACKLOG_HIGHLIGHT &&
      _ReturnAddress() == gameExeCcBacklogHighlightDrawRet) {
    spriteHeight =
        min((float)(gameExeCcBacklogLineHeights[*gameExeCcBacklogCurLine] +
                    CC_BACKLOG_HIGHLIGHT_HEIGHT_SHIFT) *
                COORDS_MULTIPLIER,
            CC_BACKLOG_HIGHLIGHT_SPRITE_HEIGHT);
    spriteY = CC_BACKLOG_HIGHLIGHT_SPRITE_Y;
    displayY += CC_BACKLOG_HIGHLIGHT_YOFFSET_SHIFT;
  }
  auto it = retAddrToSpriteFixes.find((uintptr_t)_ReturnAddress());
  if (it != retAddrToSpriteFixes.end()) {
    spriteX += it->second.srcDx;
    spriteY += it->second.srcDy;
    if (it->second.width)
      spriteWidth = it->second.width;
    if (it->second.height)
      spriteHeight = it->second.height;
    displayX += it->second.dx;
    displayY += it->second.dy;
  }
  return gameExeDrawSpriteReal(textureId, spriteX, spriteY, spriteWidth,
                               spriteHeight, displayX, displayY, color, opacity,
                               shaderId);
}
}  // namespace lb
