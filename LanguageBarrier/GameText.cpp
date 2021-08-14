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
#include "TextRendering.h"
#include <d3d9.h>
#include <string_view>
#include <string>
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
  char text[lb::MAX_PROCESSED_STRING_LENGTH];
} ProcessedSc3String_t;

// also my own
typedef struct {
  char* start;
  char* end;
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

#define DEF_DIALOGUE_PAGE(name, size, opacityType) \
  typedef struct {                                 \
    int field_0;                                   \
    int field_4;                                   \
    int drawNextPageNow;                           \
    int pageLength;                                \
    int field_10;                                  \
    char field_14;                                 \
    char field_15;                                 \
    char field_16;                                 \
    char field_17;                                 \
    int field_18;                                  \
    int field_1C;                                  \
    int field_20;                                  \
    int field_24;                                  \
    int field_28;                                  \
    int field_2C;                                  \
    int field_30;                                  \
    int field_34;                                  \
    int field_38;                                  \
    int fontNumber[size];                          \
    int charColor[size];                           \
    int charOutlineColor[size];                    \
    char glyphCol[size];                           \
    char glyphRow[size];                           \
    char glyphOrigWidth[size];                     \
    char glyphOrigHeight[size];                    \
    __int16 charDisplayX[size];                    \
    __int16 charDisplayY[size];                    \
    __int16 glyphDisplayWidth[size];               \
    __int16 glyphDisplayHeight[size];              \
    char field_BBBC[size];                         \
    int field_C38C[size];                          \
    opacityType charDisplayOpacity[size];          \
  } name;                                          \
  static name* gameExeDialoguePages_##name = NULL;
DEF_DIALOGUE_PAGE(DialoguePage_t, 2000, char);
DEF_DIALOGUE_PAGE(CCDialoguePage_t, 600, char);
DEF_DIALOGUE_PAGE(RNEDialoguePage_t, 2200, int16_t);
DEF_DIALOGUE_PAGE(RNDDialoguePage_t, 600, int16_t);

typedef void(__cdecl* DrawDialogueProc)(int fontNumber, int pageNumber,
                                        int opacity, int xOffset, int yOffset);
static DrawDialogueProc gameExeDrawDialogue =
    NULL;  // = (DrawDialogueProc)0x44B500;
static DrawDialogueProc gameExeDrawDialogueReal = NULL;

typedef void(__cdecl* DrawDialogue2Proc)(int fontNumber, int pageNumber,
                                         int opacity);
static DrawDialogue2Proc gameExeDrawDialogue2 =
    NULL;  // = (DrawDialogue2Proc)0x44B0D0;
static DrawDialogue2Proc gameExeDrawDialogue2Real = NULL;

typedef int(__cdecl* rnDrawTextHookProc)(signed int textureId, int a2,
                                         signed int startY, unsigned int a4,
                                         uint8_t* a5, signed int startX,
                                         int color, int height, int opacity);
static rnDrawTextHookProc rnDrawText = NULL;
static rnDrawTextHookProc rnDrawTextReal = NULL;

struct MultiplierData {
  float xOffset = 1.0f;
  float yOffset = 1.0f;
  float textureHeight = 1.0f;
  float textureWidth = 1.0f;
  float displayYOffset = 0.0f;
};

typedef int(__cdecl* gslFillHookProc)(int id, int a1, int a2, int a3, int a4,
                                      int r, int g, int b, int a);
static gslFillHookProc gameExegslFill = NULL;
static gslFillHookProc gameExegslFillReal = NULL;
int __cdecl gslFillHook(int id, int a1, int a2, int a3, int a4, int r, int g,
                        int b, int a);

static uintptr_t gameExeRenderMode = NULL;
static uintptr_t gameExeShaderPtr = NULL;
static uintptr_t gameExeBlendMode = NULL;
static int* gameExeLanguage = NULL;

std::string gameId;

typedef int(__cdecl* drawTwipoContentHookProc)(int textureId, int a2, int a3,
                                               unsigned int a4, int a5,
                                               unsigned int a6, char* sc3,
                                               int a8, int a9, uint32_t opacity,
                                               int a11, int a12, int a13,
                                               int a14);
static drawTwipoContentHookProc rnDrawTwipoContent = NULL;
static drawTwipoContentHookProc rnDrawTwipoContentReal = NULL;

typedef int(__cdecl* drawTipMessageHookProc)(int textureId, int a2, int a3,
                                             char* a4, unsigned int a5,
                                             int color, unsigned int a7,
                                             uint32_t opacity);
static drawTipMessageHookProc rnDrawTipMessage = NULL;
static drawTipMessageHookProc rnDrawTipMessageReal = NULL;

typedef int(__cdecl* drawChatMessageHookProc)(int a2, float a3, float a4,
                                              float a5, char* a6, float a7,
                                              int color, float a9,
                                              uint32_t opacity);
static drawChatMessageHookProc rnDrawChatMessage = NULL;
static drawChatMessageHookProc rnDrawChatMessageReal = NULL;
typedef int(__cdecl* drawPhoneCallNameProc)(int textureId, int maskId,
                                            int startX, int startY,
                                            int maskStartY, int maskHeight,
                                            unsigned int a7, char* a8, int a9,
                                            int color, unsigned int a11,
                                            signed int opacity);
static drawPhoneCallNameProc rnDrawPhoneCallName = NULL;
static drawPhoneCallNameProc rnDrawPhoneCallNameReal = NULL;

typedef int(__cdecl* DialogueLayoutRelatedProc)(int unk0, int* unk1, int* unk2,
                                                int unk3, int unk4, int unk5,
                                                int unk6, int yOffset,
                                                int lineHeight);
static DialogueLayoutRelatedProc gameExeDialogueLayoutRelated =
    NULL;  // = (DialogueLayoutRelatedProc)0x448790;
static DialogueLayoutRelatedProc gameExeDialogueLayoutRelatedReal = NULL;

typedef int(__cdecl* DrawGlyphProc)(int textureId, float glyphInTextureStartX,
                                    float glyphInTextureStartY,
                                    float glyphInTextureWidth,
                                    float glyphInTextureHeight,
                                    float displayStartX, float displayStartY,
                                    float displayEndX, float displayEndY,
                                    int color, uint32_t opacity);
static DrawGlyphProc gameExeDrawGlyph = NULL;  // = (DrawGlyphProc)0x42F950;
static DrawGlyphProc gameExeDrawGlyphReal = NULL;

typedef unsigned int(__cdecl* Sg0DrawGlyph3Proc)(
    int textureId, int maskTextureId, int textureStartX, int textureStartY,
    int textureSizeX, int textureSizeY, int startPosX, int startPosY,
    int EndPosX, int EndPosY, int color, int opacity);
static Sg0DrawGlyph3Proc gameExeSg0DrawGlyph3 = NULL;
static Sg0DrawGlyph3Proc gameExeSg0DrawGlyph3Real = NULL;

typedef unsigned int(__cdecl* DrawSpriteMaskInternalProc)(float* a1, int a2,
                                                          float* a3, float* a4,
                                                          void* a5, float a6,
                                                          int a62, int a7,
                                                          int a8, int a9);
static DrawSpriteMaskInternalProc GameExeDrawSpriteMaskInternal = NULL;

typedef unsigned int(__cdecl* GetShaderProc)(int);
static GetShaderProc GameExeGetShader = NULL;

typedef unsigned int(__cdecl* Sg0DrawGlyph2Proc)(
    int textureId, int a2, float glyphInTextureStartX,
    float glyphInTextureStartY, float glyphInTextureWidth,
    float glyphInTextureHeight, float a7, float a8, float a9, float a10,
    float a11, float a12, signed int inColor, signed int opacity, int* a15,
    int* a16);
static Sg0DrawGlyph2Proc gameExeSg0DrawGlyph2 = NULL;
static Sg0DrawGlyph2Proc gameExeSg0DrawGlyph2Real = NULL;

typedef int(__cdecl* DrawRectangleProc)(float X, float Y, float width,
                                        float height, int color,
                                        uint32_t opacity);
static DrawRectangleProc gameExeDrawRectangle =
    NULL;  // = (DrawRectangleProc)0x42F890;

typedef int(__cdecl* DrawSpriteProc)(int textureId, float spriteX,
                                     float spriteY, float spriteWidth,
                                     float spriteHeight, float displayX,
                                     float displayY, int color, int opacity,
                                     int shaderId);
static DrawSpriteProc gameExeDrawSprite =
    NULL;  // = (DrawSpriteProc)0x431280; (CHAOS;CHILD)
static DrawSpriteProc gameExeDrawSpriteReal = NULL;

typedef int(__cdecl* SetDialoguePageValuesProc)(int, uint8_t*);
static SetDialoguePageValuesProc gameExeSetDialoguePageValues =
    NULL;  // = (DrawSpriteProc)0x431280; (CHAOS;CHILD)
static SetDialoguePageValuesProc gameExeSetDialoguePageValuesReal = NULL;

typedef void(__cdecl* DrawBacklogContentProc)(int textureId, int maskTextureId,
                                              int startX, int startY,
                                              unsigned int maskY,
                                              int maskHeight, int opacity,
                                              int index);
static DrawBacklogContentProc gameExeDrawBacklogContent =
    NULL;  // = (DrawSpriteProc)0x431280; (CHAOS;CHILD)
static DrawBacklogContentProc gameExeDrawBacklogContentReal = NULL;

typedef int(__cdecl* DrawPhoneTextProc)(int textureId, int xOffset, int yOffset,
                                        int lineLength, char* sc3string,
                                        int lineSkipCount, int lineDisplayCount,
                                        int color, int baseGlyphSize,
                                        int opacity);
static DrawPhoneTextProc gameExeDrawPhoneText =
    NULL;  // = (DrawPhoneTextProc)0x444F70;
static DrawPhoneTextProc gameExeDrawPhoneTextReal = NULL;

typedef signed int(__cdecl* DrawSingleTextLineProc)(
    int textureId, int startX, signed int startY, unsigned int a4, char* string,
    signed int maxLength, int color, int glyphSize, signed int opacity);
static DrawSingleTextLineProc gameExeDrawSingleTextLine = NULL;
static DrawSingleTextLineProc gameExeDrawSingleTextLineReal = NULL;

typedef int(__cdecl* GetSc3StringDisplayWidthProc)(char* string,
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

typedef int(__cdecl* GetLinksFromSc3StringProc)(int xOffset, int yOffset,
                                                int lineLength, char* sc3string,
                                                int lineSkipCount,
                                                int lineDisplayCount,
                                                int baseGlyphSize,
                                                LinkMetrics_t* result);
static GetLinksFromSc3StringProc gameExeSghdGetLinksFromSc3String =
    NULL;  // = (GetLinksFromSc3StringProc)0x445EA0;
static GetLinksFromSc3StringProc gameExeSghdGetLinksFromSc3StringReal = NULL;

typedef int(__cdecl* DrawInteractiveMailProc)(
    int textureId, int xOffset, int yOffset, signed int lineLength,
    char* sc3string, unsigned int lineSkipCount, unsigned int lineDisplayCount,
    int color, unsigned int baseGlyphSize, int opacity, int unselectedLinkColor,
    int selectedLinkColor, int selectedLink);
static DrawInteractiveMailProc gameExeSghdDrawInteractiveMail =
    NULL;  // = (DrawInteractiveMailProc)0x4453D0;
static DrawInteractiveMailProc gameExeSghdDrawInteractiveMailReal = NULL;

typedef int(__cdecl* DrawLinkHighlightProc)(
    int xOffset, int yOffset, int lineLength, char* sc3string,
    unsigned int lineSkipCount, unsigned int lineDisplayCount, int color,
    unsigned int baseGlyphSize, int opacity, int selectedLink);
static DrawLinkHighlightProc gameExeSghdDrawLinkHighlight =
    NULL;  // = (DrawLinkHighlightProc)0x444B90;
static DrawLinkHighlightProc gameExeSghdDrawLinkHighlightReal = NULL;

typedef int(__cdecl* GetSc3StringLineCountProc)(int lineLength, char* sc3string,
                                                unsigned int baseGlyphSize);
static GetSc3StringLineCountProc gameExeGetSc3StringLineCount =
    NULL;  // = (GetSc3StringLineCountProc)0x442790;
static GetSc3StringLineCountProc gameExeGetSc3StringLineCountReal = NULL;

typedef int(__cdecl* GetRineInputRectangleProc)(int* lineLength, char* text,
                                                unsigned int baseGlyphSize);
static GetRineInputRectangleProc gameExeGetRineInputRectangle = NULL;
static GetRineInputRectangleProc gameExeGetRineInputRectangleReal = NULL;

typedef int(__cdecl* SetTipContentProc)(char* sc3string);
static SetTipContentProc gameExeSetTipContent =
    NULL;  // = (SetTipContentProc)0x44FB20;
static SetTipContentProc gameExeSetTipContentReal = NULL;

typedef void(__cdecl* DrawTipContentProc)(int textureId, int maskId, int startX,
                                          int startY, int maskStartY,
                                          int maskHeight, int a7, int color,
                                          int shadowColor, int opacity);
static DrawTipContentProc gameExeDrawTipContent =
    NULL;  // = (DrawTipContentProc)0x44FB70;
static DrawTipContentProc gameExeDrawTipContentReal = NULL;

typedef void(__cdecl* DrawReportContentProc)(
    int a1, int a2, int a3, int a4, int a5, unsigned int a6, unsigned int a7,
    unsigned int a8, unsigned int a9, char* a10, unsigned int a11,
    unsigned int a12, int a13, int a14, int a15, float a16);
static DrawReportContentProc gameExeDrawReportContent =
    NULL;  // = (DrawTipContentProc)0x44FB70;
static DrawReportContentProc gameExeDrawReportContentReal = NULL;

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

static uint8_t* gameExeGlyphWidthsFont1 = NULL;       // = (uint8_t *)0x52C7F0;
static uint8_t* gameExeGlyphWidthsFont2 = NULL;       // = (uint8_t *)0x52E058;
static int* gameExeColors = NULL;                     // = (int *)0x52E1E8;
static int8_t* gameExeBacklogHighlightHeight = NULL;  // = (int8_t *)0x435DD4;

static int* gameExeCcBacklogCurLine =
    NULL;  // = (int*)0x017F9EF8; (CHAOS;CHILD)
static int* gameExeCcBacklogLineHeights =
    NULL;  // = (int*)0x017FA560; (CHAOS;CHILD)
static void* gameExeCcBacklogHighlightDrawRet = NULL;

static uint8_t widths[lb::TOTAL_NUM_FONT_CELLS];
static float SPLIT_FONT_OUTLINE_A_HEIGHT;

static std::string* fontBuffers[3] = {0};

static char* tipContent;

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
		mov[edi + 4], eax
		mov edi, [ebp + 0x14]

    // ecx is y pos
		add ecx, [lb::DIALOGUE_REDESIGN_YOFFSET_SHIFT]

		jmp gameExeCcBacklogNamePosAdjustRet
  }
}

__declspec(naked) void ccSteamBacklogNamePosAdjustHook() {
  __asm {
    // copied code
		mov[ebx + 4], eax

        // eax is y pos
		mov eax, [ebp - 0x1AC]
		add eax, [lb::DIALOGUE_REDESIGN_YOFFSET_SHIFT]

		jmp gameExeCcBacklogNamePosAdjustRet
  }
}

int* TextNum;
char** viewMessageList;

BOOL __cdecl skipFix(int a1, int a2) {
  if (a1 == 0xFFFF) return 0;

  char flBit[] = {1, 2, 4, 8, 16, 32, 64, 128};
  char* MesView = *viewMessageList;
  return (MesView[(a2 + TextNum[2 * a1]) >> 3] &
          (unsigned __int8)flBit[(a2 + TextNum[2 * a1]) & 7]) != 0;
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
void __cdecl rnDrawDialogueHook(int fontNumber, int pageNumber,
                                uint32_t opacity, int xOffset, int yOffset);
void __cdecl rnDDrawDialogueHook(int fontNumber, int pageNumber,
                                 uint32_t opacity, int xOffset, int yOffset);
void __cdecl rnDrawDialogue2Hook(int fontNumber, int pageNumber,
                                 uint32_t opacity);
void __cdecl rnDDrawDialogue2Hook(int fontNumber, int pageNumber,
                                  uint32_t opacity);
int __cdecl dialogueLayoutRelatedHook(int unk0, int* unk1, int* unk2, int unk3,
                                      int unk4, int unk5, int unk6, int yOffset,
                                      int lineHeight);
int __cdecl drawPhoneTextHook(int textureId, int xOffset, int yOffset,
                              int lineLength, char* sc3string,
                              int lineSkipCount, int lineDisplayCount,
                              int color, int baseGlyphSize, int opacity);
signed int __cdecl drawSingleTextLineHook(int textureId, int startX,
                                          signed int startY, unsigned int a4,
                                          char* string, signed int maxLength,
                                          int color, int glyphSize,
                                          signed int opacity);
void semiTokeniseSc3String(char* sc3string, std::list<StringWord_t>& words,
                           int baseGlyphSize, int lineLength);

void processSc3TokenList(int xOffset, int yOffset, int lineLength,
                         std::list<StringWord_t>& words, int lineCount,
                         int color, int baseGlyphSize,
                         ProcessedSc3String_t* result, bool measureOnly,
                         float multiplier, int lastLinkNumber,
                         int curLinkNumber, int currentColor, int lineHeight,
                         const MultiplierData* mData);
int __cdecl sghdGetLinksFromSc3StringHook(int xOffset, int yOffset,
                                          int lineLength, char* sc3string,
                                          int lineSkipCount,
                                          int lineDisplayCount,
                                          int baseGlyphSize,
                                          LinkMetrics_t* result);
int __cdecl sghdDrawInteractiveMailHook(
    int textureId, int xOffset, int yOffset, signed int lineLength,
    char* string, unsigned int lineSkipCount, unsigned int lineDisplayCount,
    int color, unsigned int glyphSize, int opacity, int unselectedLinkColor,
    int selectedLinkColor, int selectedLink);
int __cdecl sghdDrawLinkHighlightHook(int xOffset, int yOffset, int lineLength,
                                      char* sc3string,
                                      unsigned int lineSkipCount,
                                      unsigned int lineDisplayCount, int color,
                                      unsigned int baseGlyphSize, int opacity,
                                      int selectedLink);
int __cdecl getSc3StringLineCountHook(int lineLength, char* sc3string,
                                      unsigned int baseGlyphSize);
int __cdecl getRineInputRectangleHook(int* lineLength, char* text,
                                      unsigned int baseGlyphSize);
int __cdecl sg0DrawGlyphHook(int textureId, float glyphInTextureStartX,
                             float glyphInTextureStartY,
                             float glyphInTextureWidth,
                             float glyphInTextureHeight, float displayStartX,
                             float displayStartY, float displayEndX,
                             float displayEndY, int color, uint32_t opacity);
int __cdecl rnDrawGlyphHook(int textureId, float glyphInTextureStartX,
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
                                       signed int opacity, int* a15, int* a16);
unsigned int sg0DrawGlyph3Hook(int textureId, int a2, int a3, int a4, int a5,
                               int a6, int a7, int a8, int a9, int a10, int a11,
                               int a12);
int __cdecl setTipContentHook(char* sc3string);
void __cdecl drawTipContentHook(int textureId, int maskId, int startX,
                                int startY, int maskStartY, int maskHeight,
                                int a7, int color, int shadowColor,
                                int opacity);
void drawReportContentHook(int textureId, int maskId, int a3, int a4,
                           int startX, int startY, unsigned int maskWidth,
                           unsigned int a8, unsigned int a9, char* a10,
                           unsigned int a11, unsigned int a12, int opacity,
                           int a14, int a15, float a16);
int __cdecl rnDrawTextHook(signed int textureId, int a2, signed int startY,
                           unsigned int a4, uint8_t* a5, signed int startX,
                           int color, int height, int opacity);
void __cdecl DrawBacklogContentHookRND(int textureId, int maskTextureId,
                                       int startX, int startY,
                                       unsigned int maskY, int maskHeight,
                                       int opacity, int index);
void __cdecl DrawBacklogContentHookRNE(int textureId, int maskTextureId,
                                       int startX, int startY,
                                       unsigned int maskY, int maskHeight,
                                       int opacity, int index);

int __cdecl drawTwipoContentHook(int textureId, int a2, int a3, unsigned int a4,
                                 int a5, unsigned int a6, char* sc3, int a8,
                                 int a9, uint32_t opacity, int a11, int a12,
                                 int a13, int a14);
int __cdecl drawTipMessageHook(int textureId, int a2, int a3, char* a4,
                               unsigned int a5, int color, unsigned int a7,
                               uint32_t opacity);
int __cdecl drawChatMessageHook(int a2, float a3, float a4, float a5, char* a6,
                                float a7, int color, float a9,
                                uint32_t opacity);
void drawPhoneCallNameHook(int textureId, int maskId, int startX, int startY,
                           int maskStartY, int maskHeight, unsigned int a7,
                           char* a8, int a9, int color, unsigned int a11,
                           signed int opacity);
int __cdecl SetDialoguePageValuesHook(int page, uint8_t* data);

int* BacklogLineSave;
int* BacklogDispLinePos;
int* BacklogLineBufSize;
int16_t* BacklogTextPos;
int* BacklogLineBufUse;
uint16_t* BacklogText;
int* BacklogDispCurPosSX;
int* BacklogDispCurPosEY;
int* BacklogLineBufStartp;
unsigned char* BacklogTextSize;
int* BacklogLineBufEndp;
int* BacklogBufStartp;
int* MesFontColor;
int* BacklogBufUse;
int* BacklogDispCurPosEX;
int* BacklogDispLineSize;
int* BacklogDispPos;
int* dword_948628;
int* dword_AEDDB0;
uint8_t* BacklogTextCo;
int* BacklogLineVoice;
int* BacklogDispLinePosY;
int* BacklogDispCurPosSY;

int __cdecl gslFillHook(int id, int a1, int a2, int a3, int a4, int r, int g,
                        int b, int a) {
  return gameExegslFillReal(id, a1, a2, a3, a4, r, g, b, a);
}

// There are a bunch more functions like these but I haven't seen them get hit
// during debugging and the original code *mostly* works okay if it recognises
// western text as variable-width
// (which some functions do, and others don't, except for symbols (also used in
// Western translations) it considers full-width)

enum GameID { CC, SG, SG0, RNE, RND };

GameID currentGame;
bool UseNewTextSystem = false;

void gameTextInit() {
  if (config["gamedef"].count("dialoguePageVersion") == 1) {
    if (config["gamedef"]["dialoguePageVersion"].get<std::string>() == "rn") {
      currentGame = RNE;
    } else if (config["gamedef"]["dialoguePageVersion"].get<std::string>() ==
               "rnd") {
      currentGame = RND;
    }
  }
  if (config["patch"].count("useNewTextSystem") == 1)
    UseNewTextSystem = config["patch"]["useNewTextSystem"].get<bool>();

  if (currentGame == RNE || currentGame == RND) {
    fixLeadingZeroes();

    gameExeRenderMode = *(uintptr_t*)sigScan("game", "renderMode");
    gameExeBlendMode = *(uintptr_t*)sigScan("game", "blendMode");
    gameExeShaderPtr = *(uintptr_t*)sigScan("game", "shaderPtr");

    gameExeLanguage = *(int**)sigScan("game", "CurrentLanguage");
  }

  if (currentGame == RNE) {
    fixSkipRN();
  }

  if (currentGame != RNE && currentGame != RND) {
    if (IMPROVE_DIALOGUE_OUTLINES) {
      {
        std::stringstream ss;
        ss << "languagebarrier\\"
           << config["patch"]["fontOutlineAFileName"].get<std::string>();
        slurpFile(ss.str(), fontBuffers);
        gameLoadTexture(OUTLINE_TEXTURE_ID, (void*)(fontBuffers[0]->c_str()),
                        fontBuffers[0]->size());
      }
      if (HAS_SPLIT_FONT) {
        std::stringstream ss;
        ss << "languagebarrier\\"
           << config["patch"]["fontOutlineBFileName"].get<std::string>();
        slurpFile(ss.str(), fontBuffers + 1);
        gameLoadTexture(OUTLINE_TEXTURE_ID + 1,
                        (void*)(fontBuffers[1]->c_str()),
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
      gameLoadTexture(FIRST_FONT_ID + 1, (void*)(fontBuffers[2]->c_str()),
                      fontBuffers[2]->size());
      // FONT2_B
      gameLoadTexture(FIRST_FONT_ID + 3, (void*)(fontBuffers[2]->c_str()),
                      fontBuffers[2]->size());
    }
  }
  // the game loads these asynchronously - I'm not sure how to be notified it's
  // done and I can free the buffers
  // so I'll just do it in a hook

  if (config["gamedef"]["drawGlyphVersion"].get<std::string>() == "sg0") {
    scanCreateEnableHook("game", "drawGlyph", (uintptr_t*)&gameExeDrawGlyph,
                         (LPVOID)sg0DrawGlyphHook,
                         (LPVOID*)&gameExeDrawGlyphReal);
    scanCreateEnableHook(
        "game", "sg0DrawGlyph2", (uintptr_t*)&gameExeSg0DrawGlyph2,
        (LPVOID)sg0DrawGlyph2Hook, (LPVOID*)&gameExeSg0DrawGlyph2Real);
  } else if (config["gamedef"]["drawGlyphVersion"].get<std::string>() == "rn") {
    scanCreateEnableHook("game", "drawGlyph", (uintptr_t*)&gameExeDrawGlyph,
                         (LPVOID)rnDrawGlyphHook,
                         (LPVOID*)&gameExeDrawGlyphReal);
    scanCreateEnableHook(
        "game", "sg0DrawGlyph2", (uintptr_t*)&gameExeSg0DrawGlyph2,
        (LPVOID)sg0DrawGlyph2Hook, (LPVOID*)&gameExeSg0DrawGlyph2Real);

    scanCreateEnableHook(
        "game", "sg0DrawGlyph3", (uintptr_t*)&gameExeSg0DrawGlyph3,
        (LPVOID)sg0DrawGlyph3Hook, (LPVOID*)&gameExeSg0DrawGlyph3Real);
    GameExeDrawSpriteMaskInternal =
        (DrawSpriteMaskInternalProc)sigScan("game", "drawSpriteMaskInternal");
    GameExeGetShader = (GetShaderProc)sigScan("game", "getShader");

    scanCreateEnableHook("game", "rnDrawText", (uintptr_t*)&rnDrawText,
                         (LPVOID)rnDrawTextHook, (LPVOID*)&rnDrawTextReal);

  } else {
    // TODO (?): Split font support for non-sg0 drawGlyph
    gameExeDrawGlyph = (DrawGlyphProc)sigScan("game", "drawGlyph");
  }
  gameExeDrawRectangle = (DrawRectangleProc)sigScan("game", "drawRectangle");

  if (HAS_BACKLOG_UNDERLINE) {
    gameExeBacklogHighlightHeight =
        (int8_t*)sigScan("game", "backlogHighlightHeight");
    // gameExeBacklogHighlightHeight is (negative) offset (from vertical end of
    // glyph):
    // add eax,-0x22 (83 C0 DE) -> add eax,-0x17 (83 C0 E9)
    memset_perms(
        gameExeBacklogHighlightHeight,
        BACKLOG_HIGHLIGHT_DEFAULT_HEIGHT + BACKLOG_HIGHLIGHT_HEIGHT_SHIFT, 1);
  }

  if (HAS_RINE) {
    void* call;

    call = (void*)sigScan("game", "rineNameOutlineUpperCall");
    memset_perms(call, INST_NOP, INST_CALL_LEN);

    if (RINE_BLACK_NAMES) {
      call = (void*)sigScan("game", "rineNameOutlineLowerCall");
      memset_perms(call, INST_NOP, INST_CALL_LEN);

      int* color = (int*)sigScan("game", "rineNameColor");
      memset_perms(color, 0, sizeof(int));
    }

    call = (void*)sigScan("game", "rineMessageShadowCall");
    memset_perms(call, INST_NOP, INST_CALL_LEN);
  }
  gameExeGlyphWidthsFont1 = (uint8_t*)sigScan("game", "useOfGlyphWidthsFont1");
  gameExeGlyphWidthsFont2 = (uint8_t*)sigScan("game", "useOfGlyphWidthsFont2");
  gameExeColors = (int*)sigScan("game", "useOfColors");

  scanCreateEnableHook("game", "gslFill", (uintptr_t*)&gameExegslFill,
                       (LPVOID)&gslFillHook, (LPVOID*)&gameExegslFillReal);

  if (currentGame == RNE || currentGame == RND) {
    scanCreateEnableHook(
        "game", "drawTwipoContent", (uintptr_t*)&rnDrawTwipoContent,
        (LPVOID)drawTwipoContentHook, (LPVOID*)&rnDrawTwipoContentReal);
    scanCreateEnableHook(
        "game", "drawObtainedTipMessage", (uintptr_t*)&rnDrawTipMessage,
        (LPVOID)drawTipMessageHook, (LPVOID*)&rnDrawTipMessageReal);
    scanCreateEnableHook(
        "game", "drawChatTextBox", (uintptr_t*)&rnDrawChatMessage,
        (LPVOID)drawChatMessageHook, (LPVOID*)&rnDrawChatMessageReal);
    scanCreateEnableHook(
        "game", "drawPhoneCallNameText", (uintptr_t*)&rnDrawPhoneCallName,
        (LPVOID)drawPhoneCallNameHook, (LPVOID*)&rnDrawPhoneCallNameReal);

    BacklogDispLinePos = (int*)sigScan("game", "BacklogDispLinePos");
    BacklogLineBufSize = (int*)sigScan("game", "BacklogLineBufSize");
    BacklogTextPos = (int16_t*)sigScan("game", "BacklogTextPos");
    BacklogLineBufUse = (int*)sigScan("game", "BacklogLineBufUse");
    BacklogText = (uint16_t*)sigScan("game", "BacklogText");
    BacklogDispCurPosSX = (int*)sigScan("game", "BacklogDispCurPosSX");
    BacklogDispCurPosEY = (int*)sigScan("game", "BacklogDispCurPosEY");
    BacklogLineBufStartp = (int*)sigScan("game", "BacklogLineBufStartp");
    BacklogTextSize = (unsigned char*)sigScan("game", "BacklogTextSize");
    BacklogLineBufEndp = (int*)sigScan("game", "BacklogLineBufEndp");
    BacklogBufStartp = (int*)sigScan("game", "BacklogBufStartp");
    MesFontColor = (int*)sigScan("game", "MesFontColor");
    BacklogBufUse = (int*)sigScan("game", "BacklogBufUse");
    BacklogDispCurPosEX = (int*)sigScan("game", "BacklogDispCurPosEX");
    BacklogDispLineSize = (int*)sigScan("game", "BacklogDispLineSize");
    BacklogDispPos = (int*)sigScan("game", "BacklogDispPos");
    dword_948628 = (int*)sigScan("game", "dword_948628");
    BacklogTextCo = (uint8_t*)sigScan("game", "BacklogTextCo");
    BacklogDispLinePosY = (int*)sigScan("game", "BacklogDispLinePosY");
    BacklogDispCurPosSY = (int*)sigScan("game", "BacklogDispCurPosSY");

    scanCreateEnableHook(
        "game", "drawReportContent", (uintptr_t*)&gameExeDrawReportContent,
        (LPVOID)drawReportContentHook, (LPVOID*)&gameExeDrawReportContentReal);

    scanCreateEnableHook("game", "drawSprite", (uintptr_t*)&gameExeDrawSprite,
                         (LPVOID)drawSpriteHook,
                         (LPVOID*)&gameExeDrawSpriteReal);
  }

  if (config["gamedef"].count("dialoguePageVersion") == 1 &&
      config["gamedef"]["dialoguePageVersion"].get<std::string>() == "cc") {
    gameExeDialoguePages_CCDialoguePage_t =
        (CCDialoguePage_t*)sigScan("game", "useOfDialoguePages");
    if (IMPROVE_DIALOGUE_OUTLINES) {
      scanCreateEnableHook(
          "game", "drawDialogue", (uintptr_t*)&gameExeDrawDialogue,
          (LPVOID)ccDrawDialogueHook, (LPVOID*)&gameExeDrawDialogueReal);
      scanCreateEnableHook(
          "game", "drawDialogue2", (uintptr_t*)&gameExeDrawDialogue2,
          (LPVOID)ccDrawDialogue2Hook, (LPVOID*)&gameExeDrawDialogue2Real);
    }
  } else if (config["gamedef"].count("dialoguePageVersion") == 1 &&
             config["gamedef"]["dialoguePageVersion"].get<std::string>() ==
                 "rn") {
    gameExeDialoguePages_RNEDialoguePage_t =
        (RNEDialoguePage_t*)sigScan("game", "useOfDialoguePages");
    SurfaceWrapper::game = 0;

    scanCreateEnableHook(
        "game", "drawDialogue", (uintptr_t*)&gameExeDrawDialogue,
        (LPVOID)rnDrawDialogueHook, (LPVOID*)&gameExeDrawDialogueReal);
    scanCreateEnableHook(
        "game", "drawDialogue2", (uintptr_t*)&gameExeDrawDialogue2,
        (LPVOID)rnDrawDialogue2Hook, (LPVOID*)&gameExeDrawDialogue2Real);
    scanCreateEnableHook("game", "drawBacklogContent",
                         (uintptr_t*)&gameExeDrawBacklogContent,
                         (LPVOID)DrawBacklogContentHookRNE,
                         (LPVOID*)&gameExeDrawBacklogContentReal);
  } else if (config["gamedef"].count("dialoguePageVersion") == 1 &&
             config["gamedef"]["dialoguePageVersion"].get<std::string>() ==
                 "rnd") {
    SurfaceWrapper::game = 1;
    gameExeDialoguePages_RNDDialoguePage_t =
        (RNDDialoguePage_t*)sigScan("game", "useOfDialoguePages");
    scanCreateEnableHook(
        "game", "drawDialogue2", (uintptr_t*)&gameExeDrawDialogue2,
        (LPVOID)rnDDrawDialogue2Hook, (LPVOID*)&gameExeDrawDialogue2Real);
    scanCreateEnableHook(
        "game", "drawDialogue", (uintptr_t*)&gameExeDrawDialogue,
        (LPVOID)rnDDrawDialogueHook, (LPVOID*)&gameExeDrawDialogueReal);
    dword_AEDDB0 = (int*)sigScan("game", "dword_AEDDB0");

    scanCreateEnableHook("game", "drawBacklogContent",
                         (uintptr_t*)&gameExeDrawBacklogContent,
                         (LPVOID)DrawBacklogContentHookRND,
                         (LPVOID*)&gameExeDrawBacklogContentReal);
    TextRendering::Get().dialogueSettings =
        (uint16_t*)sigScan("game", "dialoguePageSettings");
    scanCreateEnableHook("game", "setDialoguePageValues",
                         (uintptr_t*)&gameExeSetDialoguePageValues,
                         (LPVOID)SetDialoguePageValuesHook,
                         (LPVOID*)&gameExeSetDialoguePageValuesReal);

    auto call = (void*)sigScan("game", "backlogHighlight");
    memset_perms(call, INST_NOP, 3);
  } else {
    gameExeDialoguePages_DialoguePage_t =
        (DialoguePage_t*)sigScan("game", "useOfDialoguePages");
    if (IMPROVE_DIALOGUE_OUTLINES) {
      scanCreateEnableHook(
          "game", "drawDialogue", (uintptr_t*)&gameExeDrawDialogue,
          (LPVOID)drawDialogueHook, (LPVOID*)&gameExeDrawDialogueReal);
      scanCreateEnableHook(
          "game", "drawDialogue2", (uintptr_t*)&gameExeDrawDialogue2,
          (LPVOID)drawDialogue2Hook, (LPVOID*)&gameExeDrawDialogue2Real);
    }
  }
  if (currentGame != RNE && currentGame != RND) {
    scanCreateEnableHook("game", "dialogueLayoutRelated",
                         (uintptr_t*)&gameExeDialogueLayoutRelated,
                         (LPVOID)dialogueLayoutRelatedHook,
                         (LPVOID*)&gameExeDialogueLayoutRelatedReal);
  }
  if (HAS_DRAW_PHONE_TEXT) {
    scanCreateEnableHook(
        "game", "drawPhoneText", (uintptr_t*)&gameExeDrawPhoneText,
        (LPVOID)drawPhoneTextHook, (LPVOID*)&gameExeDrawPhoneTextReal);
  }
  // compatibility with old configs
  if (NEEDS_CLEARLIST_TEXT_POSITION_ADJUST) {
    static const char clearlistConfigText[] =
        "["
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
    if (!arr.is_array()) arr = json::array();
    arr.insert(arr.end(), clearlistConfig.begin(), clearlistConfig.end());
  }
  const auto& singleTextLineFixes = config["patch"].find("singleTextLineFixes");
  if (singleTextLineFixes != config["patch"].end() &&
      singleTextLineFixes->is_array()) {
    scanCreateEnableHook("game", "drawSingleTextLine",
                         (uintptr_t*)&gameExeDrawSingleTextLine,
                         (LPVOID)drawSingleTextLineHook,
                         (LPVOID*)&gameExeDrawSingleTextLineReal);
    for (const json& item : *singleTextLineFixes) {
      if (!item.is_object()) continue;
      auto sigNameIter = item.find("sigName");
      if (sigNameIter == item.end()) continue;
      if (!sigNameIter->is_string()) continue;
      const std::string& sigName = sigNameIter->get<std::string>();
      uintptr_t targetPtr = sigScan("game", sigName.c_str());
      if (!targetPtr) continue;
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
      if (!item.is_object()) continue;
      auto sigNameIter = item.find("sigName");
      if (sigNameIter == item.end() || !sigNameIter->is_string()) continue;
      const std::string& sigName = sigNameIter->get<std::string>();
      uintptr_t targetPtr = sigScan("game", sigName.c_str());
      if (!targetPtr) continue;
      SpriteFix_t& fix = retAddrToSpriteFixes[targetPtr];
      const struct {
        const char* name;
        float* ptr;
      } name2var[] = {
          {"dx", &fix.dx},         {"dy", &fix.dy},       {"width", &fix.width},
          {"height", &fix.height}, {"srcDx", &fix.srcDx}, {"srcDy", &fix.srcDy},
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
    void* target;
    if (config["gamedef"].count("ccBacklogNamePosAdjustVersion") == 1 &&
        config["gamedef"]["ccBacklogNamePosAdjustVersion"].get<std::string>() ==
            "ccsteam") {
      target = ccSteamBacklogNamePosAdjustHook;
    } else {
      target = ccBacklogNamePosAdjustHook;
    }
    scanCreateEnableHook("game", "ccBacklogNamePosCode",
                         (uintptr_t*)&gameExeCcBacklogNamePosCode, target,
                         NULL);
  }
  // The following both have the same pattern and 'occurrence: 0' in the
  // signatures.json.
  // That's because after you hook one, the first match goes away.
  scanCreateEnableHook("game", "getSc3StringDisplayWidthFont1",
                       (uintptr_t*)&gameExeGetSc3StringDisplayWidthFont1,
                       (LPVOID)getSc3StringDisplayWidthHook,
                       (LPVOID*)&gameExeGetSc3StringDisplayWidthFont1Real);

  if (HAS_DOUBLE_GET_SC3_STRING_DISPLAY_WIDTH) {
    scanCreateEnableHook("game", "getSc3StringDisplayWidthFont2",
                         (uintptr_t*)&gameExeGetSc3StringDisplayWidthFont2,
                         (LPVOID)getSc3StringDisplayWidthHook,
                         (LPVOID*)&gameExeGetSc3StringDisplayWidthFont2Real);
  }
  if (HAS_SGHD_PHONE) {
    scanCreateEnableHook("game", "sghdGetLinksFromSc3String",
                         (uintptr_t*)&gameExeSghdGetLinksFromSc3String,
                         (LPVOID)sghdGetLinksFromSc3StringHook,
                         (LPVOID*)&gameExeSghdGetLinksFromSc3StringReal);
    scanCreateEnableHook("game", "sghdDrawInteractiveMail",
                         (uintptr_t*)&gameExeSghdDrawInteractiveMail,
                         (LPVOID)sghdDrawInteractiveMailHook,
                         (LPVOID*)&gameExeSghdDrawInteractiveMailReal);
    scanCreateEnableHook("game", "sghdDrawLinkHighlight",
                         (uintptr_t*)&gameExeSghdDrawLinkHighlight,
                         (LPVOID)sghdDrawLinkHighlightHook,
                         (LPVOID*)&gameExeSghdDrawLinkHighlightReal);
  }
  if (HAS_GET_SC3_STRING_LINE_COUNT) {
    scanCreateEnableHook("game", "getSc3StringLineCount",
                         (uintptr_t*)&gameExeGetSc3StringLineCount,
                         (LPVOID)getSc3StringLineCountHook,
                         (LPVOID*)&gameExeGetSc3StringLineCountReal);
  }
  if (TIP_REIMPL) {
    scanCreateEnableHook(
        "game", "setTipContent", (uintptr_t*)&gameExeSetTipContent,
        (LPVOID)setTipContentHook, (LPVOID*)&gameExeSetTipContentReal);
    scanCreateEnableHook(
        "game", "drawTipContent", (uintptr_t*)&gameExeDrawTipContent,
        (LPVOID)drawTipContentHook, (LPVOID*)&gameExeDrawTipContentReal);
  }
  if (CC_BACKLOG_HIGHLIGHT || !retAddrToSpriteFixes.empty()) {
    scanCreateEnableHook("game", "drawSprite", (uintptr_t*)&gameExeDrawSprite,
                         (LPVOID)drawSpriteHook,
                         (LPVOID*)&gameExeDrawSpriteReal);
  }

  if (CC_BACKLOG_HIGHLIGHT) {
    gameExeCcBacklogCurLine = (int*)sigScan("game", "useOfCcBacklogCurLine");
    gameExeCcBacklogLineHeights =
        (int*)sigScan("game", "useOfCcBacklogLineHeights");
    gameExeCcBacklogHighlightDrawRet =
        (void*)sigScan("game", "ccBacklogHighlightDrawRet");
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
  int configretoffset =
      signatures["dialogueLayoutWidthLookup1"].value<int>("return", 0);
  if (configretoffset) lookup1retoffset = configretoffset;
  configretoffset =
      signatures["dialogueLayoutWidthLookup2"].value<int>("return", 0);
  if (configretoffset) lookup2retoffset = configretoffset;
  configretoffset =
      signatures["dialogueLayoutWidthLookup3"].value<int>("return", 0);
  if (configretoffset) lookup3retoffset = configretoffset;

  scanCreateEnableHook("game", "dialogueLayoutWidthLookup1",
                       &gameExeDialogueLayoutWidthLookup1,
                       dialogueLayoutWidthLookup1Hook, NULL);
  // we should have used the expression parser for these but oh well
  gameExeDialogueLayoutWidthLookup1Return = (uintptr_t)(
      (uint8_t*)gameExeDialogueLayoutWidthLookup1 + lookup1retoffset);
  scanCreateEnableHook("game", "dialogueLayoutWidthLookup2",
                       &gameExeDialogueLayoutWidthLookup2,
                       dialogueLayoutWidthLookup2Hook, NULL);
  gameExeDialogueLayoutWidthLookup2Return = (uintptr_t)(
      (uint8_t*)gameExeDialogueLayoutWidthLookup2 + lookup2retoffset);
  if (currentGame != RNE && currentGame != RND) {
    scanCreateEnableHook("game", "dialogueLayoutWidthLookup3",
                         &gameExeDialogueLayoutWidthLookup3,
                         dialogueLayoutWidthLookup3Hook, NULL);
    gameExeDialogueLayoutWidthLookup3Return = (uintptr_t)(
        (uint8_t*)gameExeDialogueLayoutWidthLookup3 + lookup3retoffset);
  }
  if (signatures.count("tipsListWidthLookup") == 1) {
    configretoffset = signatures["tipsListWidthLookup"].value<int>("return", 0);
    if (configretoffset) tipsListWidthRetoffset = configretoffset;
    scanCreateEnableHook("game", "tipsListWidthLookup",
                         &gameExeTipsListWidthLookup, tipsListWidthLookupHook,
                         NULL);
    gameExeTipsListWidthLookupReturn = (uintptr_t)(
        (uint8_t*)gameExeTipsListWidthLookup + tipsListWidthRetoffset);
  }
  if (signatures.count("getRineInputRectangle") == 1) {
    scanCreateEnableHook("game", "getRineInputRectangle",
                         (uintptr_t*)&gameExeGetRineInputRectangle,
                         (LPVOID)getRineInputRectangleHook,
                         (LPVOID*)&gameExeGetRineInputRectangleReal);
  }
  if (signatures.count("calcSpeakerNameLength")) {
    unsigned char* ptr =
        (unsigned char*)sigScan("game", "calcSpeakerNameLength");
    if (ptr) {
      // swap "shl eax,7" (3 bytes) and "div ecx" (2 bytes)
      unsigned char swapped[5];
      swapped[0] = ptr[3];
      swapped[1] = ptr[4];
      swapped[2] = ptr[0];
      swapped[3] = ptr[1];
      swapped[4] = ptr[2];
      memcpy_perms(ptr, swapped, 5);
    }
  }

  if (currentGame == RNE || currentGame == RND) {
    TextRendering::Get().Init(gameExeGlyphWidthsFont1, gameExeGlyphWidthsFont2,
                              (FontDataLanguage)*gameExeLanguage);
  } else {
    FILE* widthsfile = fopen("languagebarrier\\widths.bin", "rb");
    fread(widths, 1, TOTAL_NUM_FONT_CELLS, widthsfile);
    fclose(widthsfile);
    memcpy(gameExeGlyphWidthsFont1, widths, GLYPH_RANGE_FULLWIDTH_START);
    memcpy(gameExeGlyphWidthsFont2, widths, GLYPH_RANGE_FULLWIDTH_START);
  }
}

void fixSkipRN() {
  viewMessageList = *(char***)sigScan("game", "ViewedMsgArray");
  uintptr_t dummy;
  TextNum = *(int**)sigScan("game", "TextNum");

  auto checkTextSkip =
      scanCreateEnableHook("game", "checkViewText", &dummy, skipFix, 0);
}

void fixLeadingZeroes() {
  static char* dateTime = "%04d/%02d/%02d %02d:%02d:%02d";
  static char* dateTime2 = "%10d";
  static char* dateTime3 = "%010d";

  int leadingZeroFixAddr1 = sigScan("game", "leadingZeroFix1");
  int leadingZeroFixAddr2 = sigScan("game", "leadingZeroFix2");
  int leadingZeroFixAddr3 = sigScan("game", "leadingZeroFix3");

  memcpy_perms((void*)leadingZeroFixAddr1, &dateTime2, sizeof(char*));
  memcpy_perms((void*)leadingZeroFixAddr2, &dateTime3, sizeof(char*));
  memcpy_perms((void*)leadingZeroFixAddr3, &dateTime, sizeof(char*));

  if (config["gamedef"]["dialoguePageVersion"].get<std::string>() == "rnd") {
    void* leadingZeroFixAddr1RND = (void*)sigScan("game", "leadingZeroFix1RND");
    void* leadingZeroFixAddr2RND = (void*)sigScan("game", "leadingZeroFix2RND");
    void* leadingZeroFixAddr3RND = (void*)sigScan("game", "leadingZeroFix3RND");
    memset_perms(leadingZeroFixAddr1RND, 1, 1);
    memset_perms(leadingZeroFixAddr2RND, 1, 1);
    memset_perms(leadingZeroFixAddr3RND, 1, 1);
  }
}

int __cdecl dialogueLayoutRelatedHook(int unk0, int* unk1, int* unk2, int unk3,
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
    pageType* page = &gameExeDialoguePages_##pageType[pageNumber];             \
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
            FIRST_FONT_ID,                                                     \
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

#define DEF_RNDRAW_DIALOGUE_HOOK(funcName, pageType)                           \
                                                                               \
  void __cdecl funcName(int fontNumber, int pageNumber, uint32_t opacity,      \
                        int xOffset, int yOffset) {                            \
    pageType* page = &gameExeDialoguePages_##pageType[pageNumber];             \
    if (!TextRendering::Get().enabled)                                         \
      return gameExeDrawDialogueReal(fontNumber, pageNumber, opacity, xOffset, \
                                     yOffset);                                 \
                                                                               \
    bool newline = true;                                                       \
    float displayStartX =                                                      \
        (page->charDisplayX[0] + xOffset) * COORDS_MULTIPLIER;                 \
    float displayStartY =                                                      \
        (page->charDisplayY[0] + yOffset) * COORDS_MULTIPLIER;                 \
    for (int i = 0; i < page->pageLength; i++) {                               \
      if (fontNumber == page->fontNumber[i]) {                                 \
        int glyphSize = page->glyphDisplayHeight[i];                           \
        if (i == 0 ||                                                          \
            i > 0 && page->charDisplayY[i] != page->charDisplayY[i - 1]) {     \
          newline = true;                                                      \
        } else                                                                 \
          newline = false;                                                     \
                                                                               \
        if (newline == false) {                                                \
          __int16 fontSize = page->glyphDisplayHeight[i] * 1.5f;               \
                                                                               \
          uint32_t currentChar =                                               \
              page->glyphCol[i - 1] +                                          \
              page->glyphRow[i - 1] * TextRendering::Get().GLYPHS_PER_ROW;     \
          auto glyphInfo = TextRendering::Get()                                \
                               .getFont(fontSize, false)                       \
                               ->getGlyphInfo(currentChar, Regular);           \
          displayStartX += glyphInfo->advance;                                 \
        } else {                                                               \
          displayStartX =                                                      \
              (page->charDisplayX[i] + xOffset) * COORDS_MULTIPLIER;           \
        }                                                                      \
                                                                               \
        uint32_t _opacity = (page->charDisplayOpacity[i] * opacity) >> 8;      \
                                                                               \
        if (page->charOutlineColor[i] != -1) {                                 \
          {                                                                    \
            uint32_t currentChar =                                             \
                page->glyphCol[i] +                                            \
                page->glyphRow[i] * TextRendering::Get().GLYPHS_PER_ROW;       \
            wchar_t cChar = TextRendering::Get().fullCharMap[currentChar];     \
            const auto glyphInfo =                                             \
                TextRendering::Get()                                           \
                    .getFont(page->glyphDisplayHeight[i] * 1.5f, false)        \
                    ->getGlyphInfo(currentChar, FontType::Outline);            \
            displayStartY = (page->charDisplayY[i] + yOffset) * 1.5f;          \
                                                                               \
            __int16 fontSize = page->glyphDisplayHeight[i] * 1.5f;             \
            float yOffset = -6.0f * fontSize / 48.0f;                          \
                                                                               \
            TextRendering::Get().replaceFontSurface(fontSize);                 \
            if (glyphInfo->width && glyphInfo->rows)                           \
                                                                               \
              gameExeDrawSpriteReal(                                           \
                  TextRendering::Get().OUTLINE_TEXTURE_ID, glyphInfo->x,       \
                  glyphInfo->y, glyphInfo->width, glyphInfo->rows,             \
                  round(displayStartX + glyphInfo->left),                      \
                  round(yOffset + displayStartY + fontSize - glyphInfo->top),  \
                  page->charOutlineColor[i], _opacity, 4);                     \
          }                                                                    \
        }                                                                      \
                                                                               \
        {                                                                      \
          uint32_t currentChar =                                               \
              page->glyphCol[i] +                                              \
              page->glyphRow[i] * TextRendering::Get().GLYPHS_PER_ROW;         \
          auto glyphInfo =                                                     \
              TextRendering::Get()                                             \
                  .getFont(page->glyphDisplayHeight[i] * 1.5f, false)          \
                  ->getGlyphInfo(currentChar, FontType::Regular);              \
                                                                               \
          displayStartY = (page->charDisplayY[i] + yOffset) * 1.5f;            \
          float xRatio = ((float)page->glyphDisplayWidth[i] /                  \
                          (float)page->glyphOrigWidth[i]);                     \
                                                                               \
          TextRendering::Get().replaceFontSurface(                             \
              page->glyphDisplayHeight[i] * 1.5);                              \
          __int16 fontSize = page->glyphDisplayHeight[i] * 1.5f;               \
          float yOffset = -6.0f * fontSize / 48.0f;                            \
                                                                               \
          if (glyphInfo->width && glyphInfo->rows)                             \
            gameExeDrawSpriteReal(                                             \
                TextRendering::Get().FONT_TEXTURE_ID, glyphInfo->x,            \
                glyphInfo->y, glyphInfo->width, glyphInfo->rows,               \
                round(displayStartX + glyphInfo->left),                        \
                round(yOffset + displayStartY + fontSize - glyphInfo->top),    \
                page->charColor[i], _opacity, 4);                              \
          page->field_20 = (displayStartX + glyphInfo->advance) / 1.5f;        \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

DEF_DRAW_DIALOGUE_HOOK(drawDialogueHook, DialoguePage_t);
DEF_DRAW_DIALOGUE_HOOK(ccDrawDialogueHook, CCDialoguePage_t);
DEF_RNDRAW_DIALOGUE_HOOK(rnDrawDialogueHook, RNEDialoguePage_t);
DEF_RNDRAW_DIALOGUE_HOOK(rnDDrawDialogueHook, RNDDialoguePage_t);

void __cdecl drawDialogue2Hook(int fontNumber, int pageNumber,
                               uint32_t opacity) {
  // dunno if this is ever actually called but might as well
  drawDialogueHook(fontNumber, pageNumber, opacity, 0, 0);
}
void __cdecl ccDrawDialogue2Hook(int fontNumber, int pageNumber,
                                 uint32_t opacity) {
  ccDrawDialogueHook(fontNumber, pageNumber, opacity, 0, 0);
}
void __cdecl rnDrawDialogue2Hook(int fontNumber, int pageNumber,
                                 uint32_t opacity) {
  rnDrawDialogueHook(fontNumber, pageNumber, opacity, 0, 0);
}
void __cdecl rnDDrawDialogue2Hook(int fontNumber, int pageNumber,
                                  uint32_t opacity) {
  rnDDrawDialogueHook(fontNumber, pageNumber, opacity, 0, 0);
}

void semiTokeniseSc3String(char* sc3string, std::list<StringWord_t>& words,
                           int baseGlyphSize, int lineLength) {
  if (HAS_SGHD_PHONE) {
    lineLength -= 2 * SGHD_PHONE_X_PADDING;
  }

  ScriptThreadState sc3;
  int sc3evalResult;
  StringWord_t word = {sc3string, NULL, 0, false, false};
  const auto& fontData = TextRendering::Get().getFont(baseGlyphSize, true);

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
        sc3string = (char*)sc3.pc;
        break;
      case 9:
      case 0xB:
      case 0x1E:
        sc3string++;
        break;
      default:
        int glyphId = (uint8_t)sc3string[1] + ((c & 0x7f) << 8);
        uint16_t glyphWidth = 0;
        if (!TextRendering::Get().enabled) {
          glyphWidth = (baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
        } else {
          glyphWidth = fontData->glyphData
                           .glyphMap[TextRendering::Get().fullCharMap[glyphId]]
                           .advance;
        }
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

struct CColor {
  float r;
  float g;
  float b;
  float a;
};

void __cdecl initColor(CColor* a1, int a2) {
  if (a2 & 0xFFFFFF) {
    if ((a2 & 0xFFFFFF) == 0xFFFFFF) {
      a1->b = 1.0;
      a1->g = 1.0;
      a1->r = 1.0;
      a1->a = 1.0;
    } else {
      a1->a = 1.0;
      a1->r = (float)(a2 >> 16 & 0xFF) / 255.0;
      a1->g = (float)(a2 >> 8 & 0xFF) / 255.0;
      a1->b = (float)(unsigned __int8)(a2 & 0xFF) / 255.0;
    }
  } else {
    a1->b = 0.0;
    a1->g = 0.0;
    a1->r = 0.0;
    a1->a = 1.0;
  }
}

unsigned int __cdecl sub_4BB760(int textureId, int maskTextureId,
                                int textureStartX, int textureStartY,
                                float textureSizeX, float textureSizeY,
                                float startPosX, float startPosY, float EndPosX,
                                float EndPosY, float color, float opacity) {
  float a3[4];         // [esp+14h] [ebp-7Ch]
  void* a1[2];         // [esp+24h] [ebp-6Ch]
  CColor colorStruct;  // [esp+2Ch] [ebp-64h]
  float a4[8];         // [esp+6Ch] [ebp-24h]

  a3[0] = startPosX * 2;
  a3[1] = startPosY * 2;
  a3[2] = EndPosX * 2 - startPosX * 2;
  a3[3] = EndPosY * 2 - startPosY * 2;
  a4[0] = textureStartX;
  a4[2] = (textureSizeX);
  a4[4] = startPosX * 2;
  a4[6] = EndPosX * 2 - startPosX * 2;
  a4[5] = startPosY * 2;
  a4[7] = EndPosY * 2 - (startPosY * 2);
  a4[1] = textureStartY;
  a4[3] = (textureSizeY);
  initColor(&colorStruct, color);
  GameExeGetShader(40);
  a1[0] = lb::SurfaceWrapper::ptr(surfaceArray, textureId);
  a1[1] = lb::SurfaceWrapper::ptr(surfaceArray, maskTextureId);

  int shaderPtr = *(int*)gameExeShaderPtr;
  int blendMode = *(int*)gameExeBlendMode;
  int renderMode = *(int*)gameExeRenderMode;

  return GameExeDrawSpriteMaskInternal(
      (float*)&a1, 2, a3, a4, &colorStruct, (float)opacity / 255.0, renderMode,
      shaderPtr, (unsigned __int16)blendMode, 0);
}

#pragma comment(lib, "dxguid.lib")

void __cdecl DrawBacklogContentHookRNE(int textureId, int maskTextureId,
                                       int startX, int startY,
                                       unsigned int maskY, int maskHeight,
                                       int opacity, int index) {
  unsigned int v8;         // edi
  unsigned int v9;         // ecx
  unsigned int v10;        // esi
  int v11;                 // edx
  unsigned int v12;        // ebx
  int linePos;             // eax
  int v14;                 // ebx
  int strIndex;            // esi
  unsigned int charIndex;  // edx
  __int16 v17;             // dx
  int v18;                 // eax
  int colorIndex;          // ecx
  unsigned int v20;        // edx
  float xPosition;         // edi
  int v22;                 // eax
  int v23;                 // eax
  unsigned int v24;        // edx
  unsigned int v25;        // ebx
  unsigned int v26;        // ecx
  unsigned int v29;        // ecx
  int v30;                 // ebx
  unsigned int v31;        // ecx
  int color;               // edi
  int v33;                 // eax
  int v34;                 // eax
  int v35;                 // [esp+0h] [ebp-34h]
  int yPosition;           // [esp+14h] [ebp-20h]
  int v38;                 // [esp+18h] [ebp-1Ch]
  int v39;                 // [esp+18h] [ebp-1Ch]
  int v40;                 // [esp+1Ch] [ebp-18h]
  int endP;                // [esp+20h] [ebp-14h]
  int v42;                 // [esp+24h] [ebp-10h]
  int v43;                 // [esp+24h] [ebp-10h]
  int startPosY;           // [esp+28h] [ebp-Ch]
  int v45;                 // [esp+2Ch] [ebp-8h]
  unsigned int v46;        // [esp+2Ch] [ebp-8h]
  int v47;                 // [esp+30h] [ebp-4h]

  if (!TextRendering::Get().enabled) {
    return gameExeDrawBacklogContentReal(textureId, maskTextureId, startX,
                                         startY, maskY, maskHeight, opacity,
                                         index);
  }

  v40 = 0;
  v47 = 0;
  if (*BacklogLineBufUse) {
    v8 = *BacklogLineBufUse;
    v9 = 0;
    startPosY = 0;
    if (*BacklogLineBufUse) {
      v10 = maskY;
      do {
        v11 = 0xFFFF;
        v12 = startY + BacklogDispLinePosY[v9] - *BacklogDispPos;
        v45 = 0xFFFF;
        v42 = 0xFFFF;
        yPosition = startY + BacklogDispLinePosY[v9] - *BacklogDispPos;
        v38 = 0;
        if (v12 + BacklogDispLineSize[v9] > v10 && v12 < v10 + maskHeight) {
          linePos = BacklogDispLinePos[v9];
          v14 = 0;
          strIndex = BacklogLineBufSize[linePos];
          endP = BacklogLineBufEndp[linePos];
          int xOffset = BacklogTextPos[2 * strIndex];
          if (endP) {
            xPosition = startX;

            do {
              charIndex = BacklogText[strIndex];
              int xOffset = 0;
              bool newline = true;
              if ((charIndex & 0x8000u) == 0) {
                colorIndex = BacklogTextCo[strIndex];
                color = MesFontColor[2 * colorIndex + 1];
                auto glyphSize = BacklogTextSize[4 * strIndex + 3] * 1.5f;

                if (strIndex == 0 ||
                    strIndex > 0 &&
                        BacklogTextPos[2 * (strIndex) + 1] !=
                            BacklogTextPos[2 * (strIndex - 1) + 1]) {
                  newline = true;
                } else
                  newline = false;

                if (newline == false &&
                    (BacklogText[strIndex - 1] & 0x8000) == 0) {
                  auto glyphInfo =
                      TextRendering::Get()
                          .getFont(glyphSize, true)
                          ->getGlyphInfo(BacklogText[strIndex - 1], Regular);
                  xPosition += glyphInfo->advance / 2.0f;
                } else {
                  xPosition = startX + BacklogTextPos[2 * strIndex];
                }
                v47 = yPosition + BacklogTextPos[2 * strIndex + 1];
                v43 = BacklogTextSize[4 * strIndex + 2];
                v22 = yPosition + BacklogTextPos[2 * strIndex + 1];
                if (MesFontColor[2 * colorIndex] != 0xFFFFFF ||
                    (v9 = startPosY, startPosY != index)) {
                  TextRendering::Get().replaceFontSurface(glyphSize);
                  auto glyphInfo =
                      TextRendering::Get()
                          .getFont(glyphSize, false)
                          ->getGlyphInfo(BacklogText[strIndex], Regular);
                  sub_4BB760(
                      400, maskTextureId, glyphInfo->x, glyphInfo->y,
                      glyphInfo->width, glyphInfo->rows,
                      round(xPosition + glyphInfo->left / 2.0f + 1),
                      round(v47 + glyphSize / 2.0f - glyphInfo->top / 2.0f + 1),
                      round(xPosition + glyphInfo->left / 2.0f +
                            glyphInfo->width / 2.0f + 1),
                      round(glyphInfo->rows / 2.0f + glyphSize / 2.0f -
                            glyphInfo->top / 2.0f + 1 + v47),
                      color, opacity);
                  v9 = startPosY;
                  v22 = v47;
                }
                if (!v14 && v45 == 0xFFFF) {
                  v45 = xPosition;
                  v40 = v22;
                }
                v42 = xPosition + v43;
              } else {
                v17 = charIndex & 0x7FFF;
                if (v17 == 1) {
                  v14 = 1;
                  v38 = 1;
                }
                v18 = 0;
                if (v17 != 2) v18 = v14;
                v14 = v18;
              }
              v23 = strIndex + 1;
              strIndex = 0;
              if (v23 != 50000) strIndex = v23;
              --endP;
            } while (endP);
            v8 = *BacklogLineBufUse;
            v11 = v45;
          }
          v10 = maskY;
        }
        BacklogDispCurPosSX[v9] = v40;
        BacklogDispCurPosSY[v9] = v42;
        BacklogDispCurPosEX[v9] = v47;
        BacklogDispCurPosEY[v9] = v11;
        dword_948628[v9++] = v38;
        startPosY = v9;
      } while (v9 < v8);
    }
    v24 = 0;
    v46 = 0;
    if (v8) {
      v25 = maskY;
      do {
        v26 = startY + BacklogDispLinePosY[v24] - *BacklogDispPos;
        v35 = startY + BacklogDispLinePosY[v24] - *BacklogDispPos;
        if (v26 + BacklogDispLineSize[v24] > v25 && v26 < v25 + maskHeight) {
          linePos = BacklogDispLinePos[v24];
          strIndex = BacklogLineBufSize[linePos];
          v39 = BacklogLineBufEndp[linePos];
          if (v39) {
            xPosition = startX;
            bool newline = true;
            do {
              v29 = BacklogText[strIndex];
              if ((v29 & 0x8000u) == 0) {
                colorIndex = BacklogTextCo[strIndex];
                color = MesFontColor[2 * colorIndex + 1];
                auto glyphSize = BacklogTextSize[4 * strIndex + 3] * 1.5f;

                if (strIndex == 0 ||
                    strIndex > 0 &&
                        BacklogTextPos[2 * (strIndex) + 1] !=
                            BacklogTextPos[2 * (strIndex - 1) + 1]) {
                  newline = true;
                } else
                  newline = false;

                if (newline == false &&
                    (BacklogText[strIndex - 1] & 0x8000) == 0) {
                  auto glyphInfo =
                      TextRendering::Get()
                          .getFont(glyphSize, false)
                          ->getGlyphInfo(BacklogText[strIndex - 1], Regular);
                  xPosition += glyphInfo->advance / 2.0f;
                } else {
                  xPosition = startX + BacklogTextPos[2 * strIndex];
                }

                color = MesFontColor[2 * BacklogTextCo[strIndex]];
                v33 = v35 + BacklogTextPos[2 * strIndex + 1];
                if (color == 0xFFFFFF) {
                  if (v46 == index) color = 0x323232;
                  v33 = v35 + BacklogTextPos[2 * strIndex + 1];
                }
                glyphSize = BacklogTextSize[4 * strIndex + 3] * 1.5f;

                TextRendering::Get().replaceFontSurface(glyphSize);
                auto glyphInfo =
                    TextRendering::Get()
                        .getFont(glyphSize, false)
                        ->getGlyphInfo(BacklogText[strIndex], Regular);

                sub_4BB760(400, maskTextureId, glyphInfo->x, glyphInfo->y,
                           glyphInfo->width, glyphInfo->rows,
                           round(xPosition + glyphInfo->left / 2.0f),
                           round(BacklogTextPos[2 * strIndex + 1] + v35 +
                                 glyphSize / 2.0f - glyphInfo->top / 2.0f),
                           round(xPosition + glyphInfo->left / 2.0f +
                                 glyphInfo->width / 2.0f),
                           round(BacklogTextPos[2 * strIndex + 1] +
                                 glyphInfo->rows / 2.0f + glyphSize / 2.0f -
                                 glyphInfo->top / 2.0f + v35),
                           color, opacity);
              }
              v34 = strIndex + 1;
              strIndex = 0;
              if (v34 != 0xC350) strIndex = v34;
              --v39;
            } while (v39);
            v8 = *BacklogLineBufUse;
            v24 = v46;
            v25 = maskY;
          }
        }
        v46 = ++v24;
      } while (v24 < v8);
    }
  }
}

struct BacklogSize {
  uint8_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;
};

void __cdecl DrawBacklogContentHookRND(int textureId, int maskTextureId,
                                       int startX, int startY,
                                       unsigned int maskY, int maskHeight,
                                       int opacity, int index) {
  bool newline = true;
  float xPosition, yPosition;
  // if (GetAsyncKeyState(VK_RBUTTON)) {
  //	TextRendering::Get().enableReplacement();
  //}
  // if (GetAsyncKeyState(VK_LBUTTON)) {
  //	TextRendering::Get().disableReplacement();

  //}

  if (!TextRendering::Get().enabled)

    return gameExeDrawBacklogContentReal(78, maskTextureId, startX, startY,
                                         maskY, maskHeight, opacity, index);

  unsigned int v8;   // esi
  unsigned int v9;   // edx
  int v10;           // ebx
  unsigned int v11;  // ecx
  int v12;           // ecx
  int v13;           // edx
  int strIndex;      // edi
  unsigned int v15;  // ecx
  __int16 v16;       // cx
  int v17;           // eax
  int v18;           // eax
  int v19;           // edx
  int color;         // eax
  int v21;           // eax
  unsigned int v22;  // [esp+40h] [ebp-30h]
  unsigned int v23;  // [esp+44h] [ebp-2Ch]
  int v24;           // [esp+48h] [ebp-28h]
  int v25;           // [esp+4Ch] [ebp-24h]
  int v26;           // [esp+50h] [ebp-20h]
  int v27;           // [esp+54h] [ebp-1Ch]
  int a10;           // [esp+58h] [ebp-18h]
  int a12;           // [esp+5Ch] [ebp-14h]
  int v30;           // [esp+60h] [ebp-10h]
  int v31;           // [esp+64h] [ebp-Ch]
  int v32;           // [esp+68h] [ebp-8h]
  int v33;           // [esp+6Ch] [ebp-4h]
  int maxXX = 0;
  startX += 80;
  if (*BacklogLineBufUse) {
    v8 = 0;
    v23 = 0;
    if (*BacklogLineBufUse) {
      v9 = maskY;
      do {
        v10 = 0xFFFF;
        v11 = startY + BacklogDispLinePosY[v8] - *BacklogDispPos;
        v31 = 0xFFFF;
        v30 = 0xFFFF;
        v25 = 0;
        v32 = 0;
        v22 = v11;
        v24 = 0;
        if (v11 + BacklogDispLineSize[v8] > v9 && v11 < v9 + maskHeight) {
          v12 = BacklogDispLinePos[v8];
          v13 = 0;
          v27 = 0;
          strIndex = BacklogLineBufSize[v12];
          v26 = BacklogLineBufEndp[v12];
          if (v26) {
            auto ws = std::wstring_view((wchar_t*)BacklogText);
            auto nameStart = ws.find(0x8001, strIndex);
            auto nameEnd = ws.find(0x8002, strIndex);
            auto c = nameEnd - nameStart;
            short lastNameX = BacklogTextPos[2 * nameEnd - 2];
            short maxX = -40;
            short diff = lastNameX - maxX;
            auto glyphSize = BacklogTextSize[4 * (strIndex + 1) + 3] * 1.5f;
            int length = 0;

            if (diff != 0 && nameStart < nameEnd &&
                nameStart < strIndex + v26 && (nameEnd - nameStart) < v26) {
              for (int i = nameStart + 1; i < nameEnd; i++) {
                length += TextRendering::Get()
                              .getFont(glyphSize, false)
                              ->getGlyphInfo(BacklogText[i], Regular)
                              ->advance;
              }
              int initialX = maxX - length;

              for (int i = nameStart + 1; i < nameEnd; i++) {
                BacklogTextPos[2 * i] = initialX;
                initialX += TextRendering::Get()
                                .getFont(glyphSize, false)
                                ->getGlyphInfo(BacklogText[i], Regular)
                                ->advance;
              }
            }
            maxXX = 0;
            v32 = 0;
            v25 = 10000;
            do {
              v15 = BacklogText[strIndex];
              if ((v15 & 0x8000u) == 0) {
                v18 = BacklogTextCo[strIndex];
                v19 = MesFontColor[2 * v18];
                color = MesFontColor[2 * v18 + 1];
                a10 = v19;
                if (v19 == 0xFFFFFF) {
                  a12 = 0xFFFFFF;
                  a10 = color;
                } else {
                  a12 = color;
                }
                v32 = v22 + BacklogTextPos[2 * strIndex + 1];
                v33 = startX + BacklogTextPos[2 * strIndex] / 1.5f;
                v30 = v33 + BacklogTextSize[4 * strIndex + 2];

                int colorIndex = BacklogTextCo[strIndex];
                BacklogSize* size =
                    (BacklogSize*)&BacklogTextSize[4 * strIndex];
                auto glyphSize = BacklogTextSize[4 * strIndex + 3] * 1.5f;

                if (strIndex == 0 ||
                    strIndex > 0 &&
                        BacklogTextPos[2 * (strIndex) + 1] !=
                            BacklogTextPos[2 * (strIndex - 1) + 1]) {
                  newline = true;
                }

                if (newline == false &&
                    (BacklogText[strIndex - 1] & 0x8000) == 0) {
                  auto glyphInfo =
                      TextRendering::Get()
                          .getFont(glyphSize, false)
                          ->getGlyphInfo(BacklogText[strIndex - 1], Regular);
                  xPosition += glyphInfo->advance;
                } else {
                  xPosition = (startX * 1.5f + BacklogTextPos[2 * strIndex]);
                  newline = false;
                }

                TextRendering::Get().replaceFontSurface(glyphSize);
                auto glyphInfo =
                    TextRendering::Get()
                        .getFont(glyphSize, false)
                        ->getGlyphInfo(BacklogText[strIndex], Regular);
                int dummy1, dummy2;
                int v35 = startY + BacklogDispLinePosY[v8] - *BacklogDispPos;
                v35 *= 1.5f;
                v35 += 24 * glyphSize / 48.0;
                maxXX = max(maxXX, max(v30, xPosition + glyphInfo->left +
                                                glyphInfo->width));

                gameExeSg0DrawGlyph2(
                    TextRendering::Get().FONT_TEXTURE_ID, maskTextureId,
                    glyphInfo->x, glyphInfo->y, glyphInfo->width,
                    glyphInfo->rows, 32 * 2,
                    round(BacklogTextPos[2 * strIndex + 1] * 1.5f + v35 +
                          glyphSize / 2.0f - glyphInfo->top) -
                        13,
                    round(xPosition + glyphInfo->left),
                    round(BacklogTextPos[2 * strIndex + 1] * 1.5f + v35 +
                          glyphSize / 2.0f - glyphInfo->top),
                    round(xPosition + glyphInfo->left + glyphInfo->width),
                    round(BacklogTextPos[2 * strIndex + 1] * 1.5f +
                          glyphInfo->rows + glyphSize / 2.0f - glyphInfo->top +
                          v35),

                    a10, opacity, &dummy1, &dummy2);

                /*

                                                                                        sub_4D5B30(
                                                                                                textureId,
                                                                                                maskTextureId,
                                                                                                (float)(int)(32 * (v15 - (v15 >> 6 << 6)) + 1),
                                                                                                (float)(int)(32 * (v15 >> 6) + 1),
                                                                                                (float)(BacklogTextSize[4 * strIndex] - 2),
                                                                                                (float)(BacklogTextSize[4 * strIndex + 1] - 2),
                                                                                                (float)(v33 + 1) * 1.5,
                                                                                                (float)(v32 + 1) * 1.5,
                                                                                                (float)(v30 + 1) * 1.5,
                                                                                                (float)(v32 + 1 + BacklogTextSize[4 * strIndex + 3]) * 1.5,
                                                                                                a10,
                                                                                                opacity,
                                                                                                a12,
                                                                                                0.44999999,
                                                                                                0.30000001);*/

                v13 = v27;
                if (!v27 && v31 == 0xFFFF) {
                  v31 = v33;
                  v25 = v32;
                  //	v25 = min(v25, BacklogTextPos[2 * strIndex + 1] * 1.5f +
                  // glyphInfo->rows + glyphSize / 2.0f - glyphInfo->top);

                  v32 = max(v32, BacklogTextPos[2 * strIndex + 1] * 1.5f +
                                     glyphInfo->rows + glyphSize / 2.0f -
                                     glyphInfo->top);
                }
                if (v27 == 1 && v10 == 0xFFFF) v10 = v33;
              } else {
                v16 = v15 & 0x7FFF;
                if (v16 == 1) {
                  v13 = 1;
                  v24 = 1;
                }
                v17 = 0;
                if (v16 != 2) v17 = v13;
                v13 = v17;
                v27 = v17;
              }
              v21 = strIndex + 1;
              strIndex = 0;
              if (v21 != 50000) strIndex = v21;
              --v26;
            } while (v26);
            v8 = v23;
          }
          v9 = maskY;
        }
        dword_AEDDB0[v8] = v31;
        newline = true;
        BacklogDispCurPosSX[v8] = v25;
        BacklogDispCurPosSY[v8] = maxXX / 1.5f;
        BacklogDispCurPosEX[v8] = v32 - 8;
        BacklogDispCurPosEY[v8] = v24;
        dword_948628[v8++] = v10;
        v23 = v8;
      } while (v8 < *BacklogLineBufUse);
    }
  }
}

int __cdecl SetDialoguePageValuesHook(int page, uint8_t* data) {
  int ret = gameExeSetDialoguePageValuesReal(page, data);
  const uint16_t fontSize =
      lb::config["patch"]["dialogueFontSize"].get<uint16_t>();
  const uint16_t backlogFontSize =
      lb::config["patch"]["backlogFontSize"].get<uint16_t>();

  uint32_t* useOfStringWidthCalcMagic =
      (uint32_t*)sigScan("game", "useOfStringWidthCalcMagic");

  if (useOfStringWidthCalcMagic) {
    DWORD oldProtect;
    VirtualProtect(useOfStringWidthCalcMagic, sizeof(uint32_t), PAGE_READWRITE,
                   &oldProtect);
    *useOfStringWidthCalcMagic =
        lb::config["patch"]["dialogueFontSizeMagic"].get<uint32_t>();
    VirtualProtect(useOfStringWidthCalcMagic, sizeof(uint32_t), oldProtect,
                   &oldProtect);
  }

  if (page == 0) {
    TextRendering::Get().dialogueSettings[10] =
        lb::config["patch"]["dialogueWidth"].get<uint16_t>();
    TextRendering::Get().dialogueSettings[14] = fontSize;
    TextRendering::Get().dialogueSettings[15] = fontSize;
  }
  if (page == 9) {
    TextRendering::Get().dialogueSettings[page * 24 + 14] = backlogFontSize;
    TextRendering::Get().dialogueSettings[page * 24 + 15] = backlogFontSize;
  }
  return ret;
}

int __cdecl drawTipMessageHook(int textureId, int a2, int a3, char* sc3String,
                               unsigned int a5, int color,
                               unsigned int glyphSize, uint32_t opacity) {
  if (!TextRendering::Get().enabled) {
    return rnDrawTipMessageReal(textureId, a2, a3, sc3String, a5, color,
                                glyphSize, opacity);
    ;
  } else {
    rnDrawTextHook(TextRendering::Get().FONT_TEXTURE_ID, a2, a3, 0,
                   (uint8_t*)sc3String, a5, color, glyphSize, opacity);
  }
}

int __cdecl drawChatMessageHook(int a2, float a3, float a4, float a5, char* sc3,
                                float a7, int color, float a9,
                                uint32_t opacity) {
  int lineLength = a5;
  std::list<StringWord_t> words;

  // if (GetAsyncKeyState(VK_RBUTTON)) {
  //	TextRendering::Get().enableReplacement();
  //}
  // if (GetAsyncKeyState(VK_LBUTTON)) {
  //	TextRendering::Get().disableReplacement();

  //}

  if (!TextRendering::Get().enabled) {
    return rnDrawChatMessageReal(a2, a3, a4, a5, sc3, a7, color, a9, opacity);
  } else {
    semiTokeniseSc3String(sc3, words, a9, lineLength);
    int xOffset, yOffset;
    xOffset = 0;
    yOffset = 0;
    int lineSkipCount = 1;
    int lineDisplayCount = 0;
    lineLength = a5 * 1.5;
    const int glyphSize = a9 * 1.5;

    ProcessedSc3String_t str;
    MultiplierData mData;
    mData.xOffset = 1.5f;
    mData.yOffset = 1.5f;

    processSc3TokenList(a3, a4, lineLength, words, a5, color, glyphSize, &str,
                        false, COORDS_MULTIPLIER, 0, 0, color, glyphSize,
                        &mData);

    TextRendering::Get().replaceFontSurface(glyphSize);

    for (int i = 0; i < str.length; i++) {
      int curColor = str.color[i];
      auto glyphInfo = TextRendering::Get()
                           .getFont(glyphSize, false)
                           ->getGlyphInfo(str.glyph[i], Regular);

      if (str.textureWidth[i] > 0 && str.textureHeight[i] > 0)

        drawSpriteHook(TextRendering::Get().FONT_TEXTURE_ID,
                       str.textureStartX[i], str.textureStartY[i],
                       str.textureWidth[i], str.textureHeight[i],
                       str.displayStartX[i],
                       str.displayStartY[i] + glyphSize - glyphInfo->top,
                       curColor, opacity, 4);
    }
  }
}

int __cdecl drawTwipoContentHook(int textureId, int startX, int startY,
                                 unsigned int maxLineLength, int a5,
                                 unsigned int a6, char* sc3, int color,
                                 int glyphSize, uint32_t opacity, int linkColor,
                                 int a12, int a13, int a14) {
  // if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;
  std::list<StringWord_t> words;

  if (!TextRendering::Get().enabled) {
    return rnDrawTwipoContentReal(textureId, startX, startY, maxLineLength, a5,
                                  a6, sc3, color, glyphSize, opacity, linkColor,
                                  a12, a13, a14);
    ;
  }
  int xOffset, yOffset;
  xOffset = 0;
  yOffset = 0;

  int lineSkipCount = 1;
  int lineDisplayCount = 0;
  glyphSize *= 1.5;

  ProcessedSc3String_t str;
  MultiplierData mData;
  mData.xOffset = 1.5f;
  mData.yOffset = 1.5f;
  mData.displayYOffset = -6.0f * glyphSize / 48.0f;
  str.curLinkNumber = 0xFF;
  int lineHeight = glyphSize;

  if (config["gamedef"]["dialoguePageVersion"].get<std::string>() == "rn") {
    // Twipo regular text
    if (glyphSize == 55 && maxLineLength == 645) {
      glyphSize = 48;
      lineHeight = glyphSize * 1.25;
      a5 = ceil(a5 / 1.25) + 1;
    }
    // Twipo reply
    if (glyphSize == 63 && maxLineLength == 613) {
      glyphSize = 55;
      lineHeight = glyphSize * 1.25;
      a5 = ceil(a5 / 1.25);
    }

    // Twipo header
    if (glyphSize == 55 && maxLineLength == 461) {
      maxLineLength = 600;
    }

    if (currentGame == RNE) {
      if (glyphSize == 63 && maxLineLength == 421) {
        maxLineLength = 530;
        int length = getSc3StringDisplayWidthHook(sc3, 0xFF, glyphSize);
        while (getSc3StringDisplayWidthHook(sc3, 0xFF, glyphSize) >=
               maxLineLength)
          glyphSize--;
      }
    }

    // Kimijima report list

    if (glyphSize == 33 && maxLineLength == 138) {
      glyphSize = 29;
      mData.displayYOffset = -4.0f * glyphSize / 48.0f;
    }
    if (glyphSize == 63 && maxLineLength == 0x1CD) {
      maxLineLength = 600;
    }

    if (glyphSize == 0x27 && maxLineLength == 0xD5) {
      maxLineLength = 0x10f;
    }
  } else if (config["gamedef"]["dialoguePageVersion"].get<std::string>() ==
             "rnd") {
    if (glyphSize == 55 && maxLineLength == 645) {
      glyphSize = 48;
      lineHeight = glyphSize * 1.25;
      a5 = ceil(a5 / 1.25) + 1;
    }

    if (glyphSize == 67) {
      glyphSize = 55;
    }

    if (glyphSize == 63 && maxLineLength == 400) {
      glyphSize = 52;
      lineHeight = glyphSize * 1.25;
      a5 = ceil(a5 / 1.25);
      maxLineLength = 600;
    }

    // Twipo header
    if ((glyphSize == 63 || glyphSize == 55) && maxLineLength == 493) {
      glyphSize = 55;
      maxLineLength = 800;
    }

    // Kimijima report list

    if (glyphSize == 33 && maxLineLength == 138) {
      glyphSize = 29;
      mData.displayYOffset = -4.0f * glyphSize / 48.0f;
    }
    if (glyphSize == 63 && maxLineLength == 0x1CD) {
      maxLineLength = 600;
      int length = getSc3StringDisplayWidthHook(sc3, 0xFF, glyphSize);
      while (getSc3StringDisplayWidthHook(sc3, 0xFF, glyphSize) >=
             maxLineLength)
        glyphSize--;
    }

    if (glyphSize == 0x27 && maxLineLength == 0xD5) {
      maxLineLength = 0x10f;
    }
  }

  int lineLength = maxLineLength * 1.5f;
  if (currentGame == RNE) {
    semiTokeniseSc3String(sc3, words, glyphSize, lineLength);
  } else {
    semiTokeniseSc3String(sc3, words, glyphSize * 1.5, lineLength);
  }

  processSc3TokenList(startX, startY, lineLength, words, a5, color, glyphSize,
                      &str, true, COORDS_MULTIPLIER, -1, NOT_A_LINK, color,
                      lineHeight, &mData);

  processSc3TokenList(startX, startY, lineLength, words, a5, color, glyphSize,
                      &str, false, COORDS_MULTIPLIER, str.linkCount - 1,
                      str.curLinkNumber, str.curColor, lineHeight, &mData);

  TextRendering::Get().replaceFontSurface(glyphSize);

  for (int i = 0; i < str.length; i++) {
    int curColor = str.color[i];
    auto glyphInfo = TextRendering::Get()
                         .getFont(glyphSize, false)
                         ->getGlyphInfo(str.glyph[i], Regular);

    if (str.linkNumber[i] != NOT_A_LINK) {
      auto linkGlyphInfo = TextRendering::Get()
                               .getFont(glyphSize, false)
                               ->getGlyphInfoByChar('_', Regular);

      if (str.linkNumber[i] == a12)
        curColor = color;
      else
        curColor = linkColor;

      float endUnderScoreX = str.displayStartX[i] + glyphInfo->advance;

      if (i + 1 < str.length &&
          str.displayStartY[i] == str.displayStartY[i + 1]) {
        endUnderScoreX = str.displayStartX[i + 1];
      }

      int remaining = endUnderScoreX - str.displayStartX[i];
      int offset = 0;
      /*	while (remaining > 0) {
              if (glyphInfo->width > 0 && str.textureHeight[i] > 0)

                      /*
         drawSpriteHook(TextRendering::Get().FONT_TEXTURE_ID,
         linkGlyphInfo->x+1, linkGlyphInfo->y, min(linkGlyphInfo->width-2,
         remaining), linkGlyphInfo->rows, round(str.displayStartX[i] + offset ),
                                      round(str.displayStartY[i] + glyphSize+2 -
         linkGlyphInfo->top), curColor, opacity, 4);

                      remaining -= linkGlyphInfo->width-2;
                      offset += linkGlyphInfo->width-2;
              }*/
    }

    if (str.textureWidth[i] > 0 && str.textureHeight[i] > 0)

      drawSpriteHook(TextRendering::Get().FONT_TEXTURE_ID, str.textureStartX[i],
                     str.textureStartY[i], str.textureWidth[i],
                     str.textureHeight[i], round(str.displayStartX[i]),
                     round(str.displayStartY[i] + glyphSize - glyphInfo->top),
                     curColor, opacity, 4);
  }
  return 1;
}

float addCharacter(ProcessedSc3String_t* result, int baseGlyphSize, int glyphId,
                   int lineCount, int curLinkNumber, bool measureOnly,
                   float multiplier, int xOffset, int& curLineLength,
                   int yOffset, int currentColor, int lineHeight,
                   const MultiplierData* mData) {
  int i = result->length;
  const auto& fontData = TextRendering::Get().getFont(baseGlyphSize, false);
  char character = TextRendering::Get().fullCharMap[glyphId];
  result->text[i] = character;
  if (curLinkNumber != NOT_A_LINK) {
    result->linkCharCount++;
  }
  uint16_t glyphWidth = 0;
  if (!TextRendering::Get().enabled) {
    glyphWidth = (baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
  } else {
    glyphWidth = fontData->getGlyphInfo(glyphId, FontType::Regular)->advance;
  }
  if (!measureOnly) {
    // anything that's part of an array needs to go here, otherwise we
    // get buffer overflows with long mails
    if (!TextRendering::Get().enabled) {
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
    } else {
      auto glyphInfo = fontData->getGlyphInfo(glyphId, FontType::Regular);
      multiplier = 1.0f;
      result->linkNumber[i] = curLinkNumber;
      result->glyph[i] = glyphId;
      result->textureStartX[i] = multiplier * glyphInfo->x;
      result->textureStartY[i] = multiplier * glyphInfo->y;
      result->textureWidth[i] = glyphInfo->width * multiplier;
      result->textureHeight[i] = baseGlyphSize * multiplier;
      result->displayStartX[i] =
          (xOffset * mData->xOffset + (curLineLength) + glyphInfo->left) *
          multiplier;
      result->displayStartY[i] =
          (yOffset * mData->yOffset + (result->lines * lineHeight)) *
              multiplier +
          mData->displayYOffset;
      result->displayEndX[i] = (xOffset * mData->xOffset + curLineLength +
                                glyphInfo->width + glyphInfo->left) *
                               multiplier;
      result->displayEndY[i] = (yOffset * mData->yOffset +
                                (result->lines) * lineHeight + baseGlyphSize) *
                                   multiplier +
                               mData->displayYOffset;
      result->color[i] = currentColor;
    }
  }
  curLineLength += glyphWidth;

  result->length++;
  return glyphWidth;
}

void processSc3TokenList(int xOffset, int yOffset, int lineLength,
                         std::list<StringWord_t>& words, int lineCount,
                         int color, int baseGlyphSize,
                         ProcessedSc3String_t* result, bool measureOnly,
                         float multiplier, int lastLinkNumber,
                         int curLinkNumber, int currentColor, int lineHeight,
                         const MultiplierData* mData) {
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

  if (lineHeight == -1) lineHeight = baseGlyphSize;

  memset(result, 0, sizeof(ProcessedSc3String_t));

  int curProcessedStringLength = 0;
  int curLineLength = 0;
  int prevLineLength = 0;

  int spaceCost = TextRendering::Get()
                      .getFont(baseGlyphSize * 1.5f, true)
                      ->getGlyphInfo(GLYPH_ID_FULLWIDTH_SPACE, Regular)
                      ->advance;
  int ellipsisCost = TextRendering::Get()
                             .getFont(baseGlyphSize * 1.5f, true)
                             ->getGlyphInfoByChar('.', Regular)
                             ->advance *
                         3 +
                     4;

  MultiplierData multiplierData;
  if (mData != NULL) {
    multiplierData = *mData;
  }

  for (auto it = words.begin(); it != words.end(); it++) {
    if (result->lines >= lineCount) {
      words.erase(it, words.end());
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
      result->lines--;
      curLineLength = prevLineLength;
      curLinkNumber = NOT_A_LINK;

      for (int i = 0; i < 3; i++) {
        addCharacter(result, baseGlyphSize,
                     TextRendering::Get().fullCharMap.find('.'), lineCount - 1,
                     curLinkNumber, false, 1.0, xOffset, curLineLength, yOffset,
                     currentColor, lineHeight, mData);
      }
      words.erase(++it, words.end());
      break;
    };

    char c;
    char* sc3string = (curLineLength == 0 && it->startsWithSpace == true)
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
          sc3string = (char*)sc3.pc;
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
          if (result->lines < lineCount - 1 ||
              (result->lines == lineCount - 1 &&
               curLineLength + ellipsisCost < lineLength)) {
            auto glyphWidth = addCharacter(
                result, baseGlyphSize, glyphId, lineCount, curLinkNumber,
                measureOnly, multiplier, xOffset, curLineLength, yOffset,
                currentColor, lineHeight, mData);
          }

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
                              int lineLength, char* sc3string,
                              int lineSkipCount, int lineDisplayCount,
                              int color, int baseGlyphSize, int opacity) {
  ProcessedSc3String_t str;

  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(sc3string, words, baseGlyphSize, lineLength);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineSkipCount, color,
                      baseGlyphSize, &str, true, COORDS_MULTIPLIER, -1,
                      NOT_A_LINK, color, baseGlyphSize, nullptr);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineDisplayCount,
                      color, baseGlyphSize, &str, false, COORDS_MULTIPLIER,
                      str.linkCount - 1, str.curLinkNumber, str.curColor,
                      baseGlyphSize, nullptr);

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
                                  unsigned int a4, char* string,
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
    if (fixIter->second.fontSize) glyphSize = fixIter->second.fontSize;
  }
  return gameExeDrawSingleTextLineReal(textureId, startX, startY, a4, string,
                                       maxLength, color, glyphSize, opacity);
}

int __cdecl getSc3StringDisplayWidthHook(char* sc3string,
                                         unsigned int maxCharacters,
                                         int baseGlyphSize) {
  if (!maxCharacters) maxCharacters = DEFAULT_MAX_CHARACTERS;
  ScriptThreadState sc3;
  int sc3evalResult;
  int result = 0;
  int i = 0;
  signed char c;
  FontData* fontData;
  if (UseNewTextSystem)
    fontData = TextRendering::Get().getFont(baseGlyphSize, true);
  while (i <= maxCharacters && (c = *sc3string) != -1) {
    if (c == 4) {
      sc3.pc = sc3string + 1;
      gameExeSc3Eval(&sc3, &sc3evalResult);
      sc3string = (char*)sc3.pc;
    } else if (c < 0) {
      int glyphId = (uint8_t)sc3string[1] + ((c & 0x7f) << 8);
      if (UseNewTextSystem) {
        if (TextRendering::Get().enabled) {
          int adv = fontData->getGlyphInfo(glyphId, Regular)->advance;
          result += adv;
        } else {
          result += TextRendering::Get().originalWidth[glyphId];
        }
      } else {
        result += (baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
      }
      i++;
      sc3string += 2;
    }
  }
  return result;
}

int __cdecl sghdGetLinksFromSc3StringHook(int xOffset, int yOffset,
                                          int lineLength, char* sc3string,
                                          int lineSkipCount,
                                          int lineDisplayCount,
                                          int baseGlyphSize,
                                          LinkMetrics_t* result) {
  ProcessedSc3String_t str;

  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(sc3string, words, baseGlyphSize, lineLength);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineSkipCount, 0,
                      baseGlyphSize, &str, true, 1.0f, -1, NOT_A_LINK, 0,
                      baseGlyphSize, NULL);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineDisplayCount, 0,
                      baseGlyphSize, &str, false, 1.0f, str.linkCount - 1,
                      str.curLinkNumber, str.curColor, baseGlyphSize, NULL);

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
    char* sc3string, unsigned int lineSkipCount, unsigned int lineDisplayCount,
    int color, unsigned int baseGlyphSize, int opacity, int unselectedLinkColor,
    int selectedLinkColor, int selectedLink) {
  ProcessedSc3String_t str;

  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(sc3string, words, baseGlyphSize, lineLength);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineSkipCount, color,
                      baseGlyphSize, &str, true, COORDS_MULTIPLIER, -1,
                      NOT_A_LINK, color, baseGlyphSize, nullptr);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineDisplayCount,
                      color, baseGlyphSize, &str, false, COORDS_MULTIPLIER,
                      str.linkCount - 1, str.curLinkNumber, str.curColor,
                      baseGlyphSize, NULL);

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
                                      char* sc3string,
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
                      NOT_A_LINK, color, baseGlyphSize, NULL);
  processSc3TokenList(xOffset, yOffset, lineLength, words, lineDisplayCount,
                      color, baseGlyphSize, &str, false, COORDS_MULTIPLIER,
                      str.linkCount - 1, str.curLinkNumber, str.curColor,
                      baseGlyphSize, NULL);

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
int __cdecl getSc3StringLineCountHook(int lineLength, char* sc3string,
                                      unsigned int baseGlyphSize) {
  ProcessedSc3String_t str;
  if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;

  std::list<StringWord_t> words;
  semiTokeniseSc3String(sc3string, words, baseGlyphSize, lineLength);
  processSc3TokenList(0, 0, lineLength, words, LINECOUNT_DISABLE_OR_ERROR, 0,
                      baseGlyphSize, &str, true, 1.0f, -1, NOT_A_LINK, 0,
                      baseGlyphSize, NULL);
  return str.lines + 1;
}
int __cdecl getRineInputRectangleHook(int* lineLength, char* text,
                                      unsigned int baseGlyphSize) {
  ProcessedSc3String_t str;
  int maxLineLength =
      (lineLength && *lineLength ? *lineLength : DEFAULT_LINE_LENGTH);

  std::list<StringWord_t> words;
  semiTokeniseSc3String(text, words, baseGlyphSize, maxLineLength);
  processSc3TokenList(0, 0, maxLineLength, words, LINECOUNT_DISABLE_OR_ERROR, 0,
                      baseGlyphSize, &str, true, 1.0f, -1, NOT_A_LINK, 0,
                      baseGlyphSize, NULL);
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

int rnDrawGlyphHook(int textureId, float glyphInTextureStartX,
                    float glyphInTextureStartY, float glyphInTextureWidth,
                    float glyphInTextureHeight, float displayStartX,
                    float displayStartY, float displayEndX, float displayEndY,
                    int color, uint32_t opacity) {
  if (TextRendering::Get().enabled) {
    //	if (textureId == FIRST_FONT_ID)
    //		textureId = TextRendering::Get().FONT_TEXTURE_ID;
    //	if (textureId == OUTLINE_TEXTURE_ID)
    //		textureId = TextRendering::Get().OUTLINE_TEXTURE_ID;
    //	TextRendering::Instance().replaceFontSurface(glyphInTextureHeight);
  }

  return gameExeDrawGlyphReal(
      textureId, glyphInTextureStartX, glyphInTextureStartY,
      glyphInTextureWidth, glyphInTextureHeight, displayStartX, displayStartY,
      displayEndX, displayEndY, color, opacity);
}

int __cdecl rnDrawTextHook(signed int textureId, int a2, signed int startY,
                           unsigned int a4, uint8_t* sc3, signed int startX,
                           int color, int height, int opacity) {
  int length = 0;

  bool finish = false;
  int sc3Index = 0;
  std::vector<uint16_t> v;
  std::vector<wchar_t> v2;

  if (TextRendering::Get().enabled) {
    if (a4 == 0x104 && height == 0x18) {
      a4 *= 1.33;
      height *= 1.33;
    }
    int width = a4 * 1.5 + 1;
    while (a4 != 0 && width > a4 * 1.5 && height > 0) {
      width = getSc3StringDisplayWidthHook((char*)sc3, 0, height * 1.5);
      height--;
    }
    std::list<StringWord_t> words;

    semiTokeniseSc3String((char*)sc3, words, height * 1.5, a4 * 1.5);
    int xOffset, yOffset;
    xOffset = 0;
    yOffset = 0;
    int lineSkipCount = 1;
    int lineDisplayCount = 0;
    int lineLength = a4 * 1.5;
    const int glyphSize = height * 1.5;

    ProcessedSc3String_t str;
    MultiplierData mData;
    mData.xOffset = 1.5f;
    mData.yOffset = 1.5f;
    mData.displayYOffset = -6.0f * glyphSize / 48.0f;
    if (a4 == 0) a4 = 10000;

    processSc3TokenList(a2, startY, a4 * 2.5f, words, 1, color, glyphSize, &str,
                        false, COORDS_MULTIPLIER, 0, 0, color, glyphSize,
                        &mData);

    TextRendering::Get().replaceFontSurface(glyphSize);

    for (int i = 0; i < str.length; i++) {
      int curColor = str.color[i];
      auto glyphInfo = TextRendering::Get()
                           .getFont(glyphSize, false)
                           ->getGlyphInfo(str.glyph[i], Regular);

      if (str.textureWidth[i] > 0 && str.textureHeight[i] > 0)

        drawSpriteHook(TextRendering::Get().FONT_TEXTURE_ID,
                       str.textureStartX[i], str.textureStartY[i],
                       str.textureWidth[i], str.textureHeight[i],
                       str.displayStartX[i],
                       str.displayStartY[i] + glyphSize - glyphInfo->top,
                       curColor, opacity, 4);
    }

    /*	while (!finish) {

                    if (sc3[sc3Index] == 0xFF) {
                            finish = true;
                    }
                    else
                            if (sc3[sc3Index] >= 0x80) {

                                    uint16_t value = (sc3[sc3Index] >> 8 |
       sc3[sc3Index + 1]) & 0x7FFF; v.push_back(value);
                                    v2.push_back(TextRendering::Get().fullCharMap[value]);
                                    sc3Index += 2;
                            }
                            else {
                                    sc3Index++;
                            }

            }
            int displayStartX = a2 * 1.5f;

            for (int i = 0; i < v.size(); i++) {
                    uint32_t currentChar = v[i];
                    auto fontData = TextRendering::Get().getFont(height * 1.5f,
       false);

                    auto glyphInfo = fontData->getGlyphInfo(currentChar,
       FontType::Regular);

                    int column = currentChar %
       TextRendering::Get().GLYPHS_PER_ROW; int row = currentChar /
       TextRendering::Get().GLYPHS_PER_ROW; int displayStartY = startY * 1.5f;
                    TextRendering::Get().replaceFontSurface(height * 1.5f);
                    if (glyphInfo->width && glyphInfo->rows) {
                            gameExeDrawSpriteReal(
                                    TextRendering::Get().FONT_TEXTURE_ID,
       glyphInfo->x, glyphInfo->y, glyphInfo->width, glyphInfo->rows,
       (displayStartX + glyphInfo->left), (displayStartY + height -
       glyphInfo->top) , color, opacity, 4);
                    }

                    displayStartX += glyphInfo->advance;

            }*/
  } else {
    rnDrawTextReal(textureId, a2, startY, a4, sc3, startX, color, height,
                   opacity);
  }
  return 1;
}

unsigned int sg0DrawGlyph2Hook(int textureId, int a2,
                               float glyphInTextureStartX,
                               float glyphInTextureStartY,
                               float glyphInTextureWidth,
                               float glyphInTextureHeight, float a7, float a8,
                               float a9, float a10, float a11, float a12,
                               signed int inColor, signed int opacity, int* a15,
                               int* a16) {
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

  if (TextRendering::Get().enabled)
    if (currentGame == RNE) {
      return gameExeSg0DrawGlyph2Real(
          TextRendering::Get().FONT_TEXTURE_ID, a2, glyphInTextureStartX / 1.5f,
          glyphInTextureStartY / 1.5f, glyphInTextureWidth / 1.5f,
          glyphInTextureHeight / 1.5f, a7 / 2.0f, a8 / 2.0f, a9 / 2.0f,
          a10 / 2.0f, a11 / 2.0f, a12 / 2.0f, inColor, opacity, a15, a16);
    } else {
      return gameExeSg0DrawGlyph2Real(
          TextRendering::Get().FONT_TEXTURE_ID, a2, glyphInTextureStartX,
          glyphInTextureStartY, glyphInTextureWidth, glyphInTextureHeight, a7,
          a8, a9, a10, a11, a12, inColor, opacity, a15, a16);
    }

  else {
    return gameExeSg0DrawGlyph2Real(textureId, a2, glyphInTextureStartX,
                                    glyphInTextureStartY, glyphInTextureWidth,
                                    glyphInTextureHeight, a7, a8, a9 + 0, a10,
                                    a11, a12, inColor, opacity, a15, a16);
  }
}

unsigned int sg0DrawGlyph3Hook(int textureId, int maskTextureId,
                               int textureStartX, int textureStartY,
                               int textureSizeX, int textureSizeY,
                               int startPosX, int startPosY, int EndPosX,
                               int EndPosY, int color, int opacity) {
  return gameExeSg0DrawGlyph3Real(
      textureId, maskTextureId, textureStartX, textureStartY, textureSizeX,
      textureSizeY, startPosX, startPosY, EndPosX, EndPosY, color, opacity);
}

int setTipContentHook(char* sc3string) {
  if (!TextRendering::Get().enabled) return gameExeSetTipContentReal(sc3string);
  tipContent = sc3string;
  ProcessedSc3String_t str;

  std::list<StringWord_t> words;
  MultiplierData mData;
  if (currentGame == RNE) {
    mData.xOffset = 2.0f;
    mData.yOffset = 2.0f;
  } else {
    mData.xOffset = 1.5f;
    mData.yOffset = 1.5f;
  }

  semiTokeniseSc3String(tipContent, words, TIP_REIMPL_GLYPH_SIZE,
                        TIP_REIMPL_LINE_LENGTH);
  processSc3TokenList(0, 0, TIP_REIMPL_LINE_LENGTH, words, 255, 0,
                      TIP_REIMPL_GLYPH_SIZE, &str, false, COORDS_MULTIPLIER, -1,
                      NOT_A_LINK, 0, TIP_REIMPL_GLYPH_SIZE * 1.5f, &mData);

  return str.displayEndY[str.length - 1];  // scroll height
}

void drawReportContentHook(int textureId, int maskId, int a3, int a4,
                           int startX, int startY, unsigned int maskWidth,
                           unsigned int a8, unsigned int a9, char* a10,
                           unsigned int a11, unsigned int a12, int opacity,
                           int a14, int a15, float a16) {
  ProcessedSc3String_t str;

  int dummy1;
  int dummy2;
  char name[256];
  std::list<StringWord_t> words;
  MultiplierData mData;
  if (!TextRendering::Get().enabled) {
    gameExeDrawReportContentReal(textureId, maskId, a3, a4, startX, startY,
                                 maskWidth, a8, a9, a10, a11, a12, opacity, a14,
                                 a15, a16);
    return;
  }
  if (currentGame == RNE) {
    mData.xOffset = 2.0f;
    mData.yOffset = 2.0f;
  } else {
    mData.xOffset = 1.5f;
    mData.yOffset = 1.5f;
  }

  semiTokeniseSc3String(a10, words, 64, TIP_REIMPL_LINE_LENGTH);
  processSc3TokenList(startX, startY, a3, words, 255, a11, 64, &str, false,
                      COORDS_MULTIPLIER, -1, NOT_A_LINK, a11, 64 * 1.25f,
                      &mData);

  if (a8 <= startY + a12 && a9 >= startY) {
    for (int i = 0; i < str.length; i++) {
      if (str.displayStartY[i] / COORDS_MULTIPLIER > a8 / 1.5f &&
          str.displayEndY[i] / COORDS_MULTIPLIER < (a8 + a9 * 1.5f) * 1.0f) {
        TextRendering::Get().replaceFontSurface(64);
        auto fontData = TextRendering::Get().getFont(64, false);
        auto glyphInfo =
            fontData->getGlyphInfo(str.glyph[i], FontType::Regular);

        int maskY;
        int maskYf;

        maskY = a4 + 44;
        if (a8 + 12 <= startY) {
          if (a9 - 12 >= startY + a12) {
            goto LABEL_35;
          }

          else {
            maskYf = a4 - a9 + 88;
          }
        } else {
          maskYf = a4 - a8 + 32;
        }
        maskY = startY + maskYf;
      LABEL_35:

        if (str.displayStartY[i] > 120 && str.displayStartY[i] < 900) {
          maskY = 38;
        }

        gameExeSg0DrawGlyph2(
            TextRendering::Get().FONT_TEXTURE_ID, maskId, str.textureStartX[i],
            str.textureStartY[i], str.textureWidth[i], str.textureHeight[i],
            a3 * 2, (maskY)*2 + 64 - glyphInfo->top,
            ((float)str.displayStartX[i] + (1.0f * COORDS_MULTIPLIER)),
            ((float)str.displayStartY[i] - glyphInfo->top + 64 +
             ((1.0f + (float)0) * COORDS_MULTIPLIER)),
            ((float)str.displayEndX[i] + (1.0f * COORDS_MULTIPLIER)),
            ((float)str.displayEndY[i] - glyphInfo->top + 64 +
             ((1.0f + (float)0) * COORDS_MULTIPLIER)),
            str.color[i], opacity, &dummy1, &dummy2);

        gameExeSg0DrawGlyph2(TextRendering::Get().FONT_TEXTURE_ID, maskId,
                             str.textureStartX[i], str.textureStartY[i],
                             str.textureWidth[i], str.textureHeight[i], a3 * 2,
                             (maskY)*2 + 64 - glyphInfo->top,
                             (float)str.displayStartX[i],
                             (float)str.displayStartY[i] - glyphInfo->top + 64,
                             (float)str.displayEndX[i],
                             (float)str.displayEndY[i] - glyphInfo->top + 64,
                             str.color[i], opacity, &dummy1, &dummy2);
      }
    }
  }
}

void drawPhoneCallNameHook(int textureId, int maskId, int a3, int a4, int a5,
                           int a6, unsigned int a7, char* a8, int a9, int color,
                           unsigned int a11, signed int opacity) {
  ProcessedSc3String_t str;

  int dummy1;
  int dummy2;
  char name[256];
  std::list<StringWord_t> words;
  MultiplierData mData;

  if (!TextRendering::Get().enabled) {
    rnDrawPhoneCallNameReal(textureId, maskId, a3, a4, a5, a6, a7, a8, a9,
                            color, a11, opacity);
    return;
  }
  a11 *= 1.75;
  if (currentGame == RNE) {
    mData.xOffset = 2.0f;
    mData.yOffset = 2.0f;
  } else {
    mData.xOffset = 1.5f;
    mData.yOffset = 1.5f;
  }
  semiTokeniseSc3String(a8, words, a11, TIP_REIMPL_LINE_LENGTH);
  processSc3TokenList(a3, a4, a7, words, 255, color, a11, &str, false,
                      COORDS_MULTIPLIER, -1, NOT_A_LINK, color, a11 * 1.25f,
                      &mData);
  for (int i = 0; i < str.length; i++) {
    /*	if (str.displayStartY[i] / COORDS_MULTIPLIER > a8 / 1.5f &&
                    str.displayEndY[i] / COORDS_MULTIPLIER < (a8 + a9 * 1.5f)
       * 1.0f)*/
    {
      TextRendering::Get().replaceFontSurface(a11);
      auto fontData = TextRendering::Get().getFont(a11, false);
      auto glyphInfo = fontData->getGlyphInfo(str.glyph[i], FontType::Regular);

      gameExeSg0DrawGlyph2(
          TextRendering::Get().FONT_TEXTURE_ID, maskId, str.textureStartX[i],
          str.textureStartY[i], str.textureWidth[i], str.textureHeight[i],
          str.displayStartX[i] + a5 * 2, (a4 + a6) * 1.5 + 64 - glyphInfo->top,
          ((float)str.displayStartX[i] + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayStartY[i] - glyphInfo->top + 64 +
           ((1.0f + (float)0) * COORDS_MULTIPLIER)),
          ((float)str.displayEndX[i] + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayEndY[i] - glyphInfo->top + 64 +
           ((1.0f + (float)0) * COORDS_MULTIPLIER)),
          str.color[i], opacity, &dummy1, &dummy2);
    }
  }
}

void drawTipContentHook(int textureId, int maskId, int startX, int startY,
                        int maskStartY, int maskHeight, int a7, int color,
                        int shadowColor, int opacity) {
  ProcessedSc3String_t str;

  int dummy1;
  int dummy2;
  char name[256];
  /*	if (GetAsyncKeyState(VK_RBUTTON)) {
                  TextRendering::Get().enableReplacement();
          }
          if (GetAsyncKeyState(VK_LBUTTON)) {
                  TextRendering::Get().disableReplacement();

          }
          */

  if (!TextRendering::Get().enabled) {
    gameExeDrawTipContentReal(textureId, maskId, startX, startY, maskStartY,
                              maskHeight, a7, color, shadowColor, opacity);
    return;
  }
  std::list<StringWord_t> words;
  MultiplierData mData;
  if (currentGame == RNE) {
    mData.xOffset = 2.0f;
    mData.yOffset = 2.0f;
  } else {
    mData.xOffset = 1.5f;
    mData.yOffset = 1.5f;
  }

  semiTokeniseSc3String(tipContent, words, TIP_REIMPL_GLYPH_SIZE,
                        TIP_REIMPL_LINE_LENGTH);
  processSc3TokenList(startX, startY, TIP_REIMPL_LINE_LENGTH, words, 255, color,
                      TIP_REIMPL_GLYPH_SIZE, &str, false, COORDS_MULTIPLIER, -1,
                      NOT_A_LINK, color, TIP_REIMPL_GLYPH_SIZE * 1.5f, &mData);
  TextRendering::Get().replaceFontSurface(TIP_REIMPL_GLYPH_SIZE);
  auto fontData = TextRendering::Get().getFont(TIP_REIMPL_GLYPH_SIZE, false);
  maskHeight *= 1.5f;
  for (int i = 0; i < str.length; i++) {
    if (str.displayStartY[i] / COORDS_MULTIPLIER > maskStartY &&
        str.displayEndY[i] / COORDS_MULTIPLIER <
            (maskStartY + maskHeight) * 1.0f) {
      auto glyphInfo = fontData->getGlyphInfo(str.glyph[i], FontType::Regular);

      gameExeSg0DrawGlyph2(
          TextRendering::Get().FONT_TEXTURE_ID, maskId, str.textureStartX[i],
          str.textureStartY[i], str.textureWidth[i], str.textureHeight[i],
          ((float)str.displayStartX[i] + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayStartY[i] + TIP_REIMPL_GLYPH_SIZE -
           glyphInfo->top + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayStartX[i] + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayStartY[i] + TIP_REIMPL_GLYPH_SIZE -
           glyphInfo->top + ((1.0f + (float)a7) * COORDS_MULTIPLIER)),
          ((float)str.displayEndX[i] + (1.0f * COORDS_MULTIPLIER)),
          ((float)str.displayEndY[i] + TIP_REIMPL_GLYPH_SIZE - glyphInfo->top +
           ((1.0f + (float)a7) * COORDS_MULTIPLIER)),
          shadowColor, opacity, &dummy1, &dummy2);

      gameExeSg0DrawGlyph2(
          TextRendering::Get().FONT_TEXTURE_ID, maskId, str.textureStartX[i],
          str.textureStartY[i], str.textureWidth[i], str.textureHeight[i],
          str.displayStartX[i],
          str.displayStartY[i] + TIP_REIMPL_GLYPH_SIZE - glyphInfo->top,
          str.displayStartX[i],
          ((float)str.displayStartY[i] + TIP_REIMPL_GLYPH_SIZE -
           glyphInfo->top + ((float)a7 * COORDS_MULTIPLIER)),
          str.displayEndX[i],
          ((float)str.displayEndY[i] + TIP_REIMPL_GLYPH_SIZE - glyphInfo->top +
           ((float)a7 * COORDS_MULTIPLIER)),
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
    if (it->second.width) spriteWidth = it->second.width;
    if (it->second.height) spriteHeight = it->second.height;
    displayX += it->second.dx;
    displayY += it->second.dy;
  }

  if (textureId == 80 && (spriteY == 1640 || spriteY == 1936)) {
    gameExeDrawSpriteReal(textureId, spriteX, spriteY + 4, spriteWidth, 9,
                          displayX, displayY, color, opacity, shaderId);
    return gameExeDrawSpriteReal(textureId, spriteX, spriteY + 4, spriteWidth,
                                 spriteHeight - 9, displayX, displayY + 9,
                                 color, opacity, shaderId);
  }
  return gameExeDrawSpriteReal(textureId, spriteX, spriteY, spriteWidth,
                               spriteHeight, displayX, displayY, color, opacity,
                               shaderId);
}
}  // namespace lb
