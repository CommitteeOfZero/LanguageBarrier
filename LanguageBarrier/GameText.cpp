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

#define DEF_DIALOGUE_PAGE(name, size, opacityType)  \
  typedef struct {                  \
  int field_0;                  \
  int field_4;                  \
  int drawNextPageNow;              \
  int pageLength;                 \
  int field_10;                   \
  char field_14;                  \
  char field_15;                  \
  char field_16;                  \
  char field_17;                  \
  int field_18;                   \
  int field_1C;                   \
  int field_20;                   \
  int field_24;                   \
  int field_28;                   \
  int field_2C;                   \
  int field_30;                   \
  int field_34;                   \
  int field_38;                   \
  int fontNumber[size];               \
  int charColor[size];              \
  int charOutlineColor[size];           \
  char glyphCol[size];              \
  char glyphRow[size];              \
  char glyphOrigWidth[size];            \
  char glyphOrigHeight[size];           \
  __int16 charDisplayX[size];           \
  __int16 charDisplayY[size];           \
  __int16 glyphDisplayWidth[size];        \
  __int16 glyphDisplayHeight[size];         \
  char field_BBBC[size];              \
  int field_C38C[size];               \
  opacityType charDisplayOpacity[size];       \
  } name;                       \
  static name *gameExeDialoguePages_##name = NULL;
DEF_DIALOGUE_PAGE(DialoguePage_t, 2000, char);
DEF_DIALOGUE_PAGE(CCDialoguePage_t, 600, char);
DEF_DIALOGUE_PAGE(RNDialoguePage_t, 2200, int16_t);

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

typedef int(__cdecl* rnDrawTextHookProc)(signed int textureId, int a2, signed int startY, unsigned int a4, uint8_t* a5, signed int startX, int color, int height, int opacity);
static rnDrawTextHookProc rnDrawText = NULL;
static rnDrawTextHookProc rnDrawTextReal = NULL;

struct MultiplierData {
	float xOffset = 1.0f;
	float yOffset = 1.0f;
	float textureHeight = 1.0f;
	float textureWidth = 1.0f;
};

typedef int(__cdecl* gslFillHookProc)(int id, int a1, int a2, int a3, int a4, int r, int g, int b, int a);
static gslFillHookProc gameExegslFill = NULL;
static gslFillHookProc gameExegslFillReal = NULL;
int __cdecl gslFillHook(int id, int a1, int a2, int a3, int a4, int r, int g, int b, int a);



typedef int(__cdecl* drawTwipoContentHookProc)(int textureId, int a2, int a3, unsigned int a4, int a5, unsigned int a6, char* sc3, int a8, int a9, uint32_t opacity, int a11, int a12, int a13, int a14
	);
static drawTwipoContentHookProc rnDrawTwipoContent = NULL;
static drawTwipoContentHookProc rnDrawTwipoContentReal = NULL;




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
	int textureId, int maskTextureId,
	int textureStartX, int textureStartY, int textureSizeX,
	int textureSizeY, int startPosX,
	int startPosY, int EndPosX, int EndPosY, int color, int opacity);
static Sg0DrawGlyph3Proc gameExeSg0DrawGlyph3 = NULL;
static Sg0DrawGlyph3Proc gameExeSg0DrawGlyph3Real = NULL;

typedef unsigned int(__cdecl* DrawSpriteMaskInternalProc)(float* a1, int a2, float* a3, float* a4, void* a5, float a6, int a62, int a7, int a8, int a9);
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

typedef void(__cdecl* DrawBacklogContentProc)(int textureId, int maskTextureId, int startX, int startY, unsigned int maskY, int maskHeight, int opacity, int index);
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

typedef int(__cdecl* GetRineInputRectangleProc)(int* lineLength, char* text, unsigned int baseGlyphSize);
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

static uintptr_t gameExeCcBacklogNamePosCode = NULL;     // = 0x00454FE9
static uintptr_t gameExeCcBacklogNamePosAdjustRet = NULL;  // = 0x00454FEF

static uint8_t* gameExeGlyphWidthsFont1 = NULL;     // = (uint8_t *)0x52C7F0;
static uint8_t* gameExeGlyphWidthsFont2 = NULL;     // = (uint8_t *)0x52E058;
static int* gameExeColors = NULL;           // = (int *)0x52E1E8;
static int8_t* gameExeBacklogHighlightHeight = NULL;  // = (int8_t *)0x435DD4;

static int* gameExeCcBacklogCurLine =
NULL;  // = (int*)0x017F9EF8; (CHAOS;CHILD)
static int* gameExeCcBacklogLineHeights =
NULL;  // = (int*)0x017FA560; (CHAOS;CHILD)
static void* gameExeCcBacklogHighlightDrawRet = NULL;

static uint8_t widths[lb::TOTAL_NUM_FONT_CELLS];
static float SPLIT_FONT_OUTLINE_A_HEIGHT;

static std::string* fontBuffers[3] = { 0 };

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
	void __cdecl rnDrawDialogue2Hook(int fontNumber, int pageNumber,
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
		int curLinkNumber, int currentColor, int lineHeight, const MultiplierData* mData);
	int __cdecl getSc3StringDisplayWidthHook(char* sc3string,
		unsigned int maxCharacters,
		int baseGlyphSize);
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
	int __cdecl getRineInputRectangleHook(int* lineLength, char* text, unsigned int baseGlyphSize);
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
	unsigned int sg0DrawGlyph3Hook(int textureId, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12);
	int __cdecl setTipContentHook(char* sc3string);
	void __cdecl drawTipContentHook(int textureId, int maskId, int startX,
		int startY, int maskStartY, int maskHeight,
		int a7, int color, int shadowColor,
		int opacity);
	int __cdecl drawSpriteHook(int textureId, float spriteX, float spriteY,
		float spriteWidth, float spriteHeight,
		float displayX, float displayY, int color,
		int opacity, int shaderId);

	int __cdecl rnDrawTextHook(signed int textureId, int a2, signed int startY, unsigned int a4, uint8_t* a5, signed int startX, int color, int height, int opacity);
	void __cdecl DrawBacklogContentHook(int textureId, int maskTextureId, int startX, int startY, unsigned int maskY, int maskHeight, int opacity, int index);
	int __cdecl drawTwipoContentHook(int textureId, int a2, int a3, unsigned int a4, int a5, unsigned int a6, char* sc3, int a8, int a9, uint32_t opacity, int a11, int a12, int a13, int a14);



	int __cdecl gslFillHook(int id, int a1, int a2, int a3, int a4, int r, int g, int b, int a) {
		return gameExegslFillReal(id, a1, a2, a3, a4, r, g, b, a);
	}

	// There are a bunch more functions like these but I haven't seen them get hit
	// during debugging and the original code *mostly* works okay if it recognises
	// western text as variable-width
	// (which some functions do, and others don't, except for symbols (also used in
	// Western translations) it considers full-width)

	void gameTextInit() {



		/*	if (IMPROVE_DIALOGUE_OUTLINES) {
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
			  gameLoadTexture(OUTLINE_TEXTURE_ID + 1, (void*)(fontBuffers[1]->c_str()),
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
		  }*/
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
		}
		else if (config["gamedef"]["drawGlyphVersion"].get<std::string>() == "rn") {

			scanCreateEnableHook("game", "drawGlyph", (uintptr_t*)&gameExeDrawGlyph,
				(LPVOID)rnDrawGlyphHook,
				(LPVOID*)&gameExeDrawGlyphReal);
			scanCreateEnableHook(
				"game", "sg0DrawGlyph2", (uintptr_t*)&gameExeSg0DrawGlyph2,
				(LPVOID)sg0DrawGlyph2Hook, (LPVOID*)&gameExeSg0DrawGlyph2Real);

			scanCreateEnableHook(
				"game", "sg0DrawGlyph3", (uintptr_t*)&gameExeSg0DrawGlyph3,
				(LPVOID)sg0DrawGlyph3Hook, (LPVOID*)&gameExeSg0DrawGlyph3Real);
			GameExeDrawSpriteMaskInternal = (DrawSpriteMaskInternalProc)sigScan("game", "drawSpriteMaskInternal");
			GameExeGetShader = (GetShaderProc)sigScan("game", "getShader");

			scanCreateEnableHook(
				"game", "rnDrawText", (uintptr_t*)&rnDrawText,
				(LPVOID)rnDrawTextHook, (LPVOID*)&rnDrawTextReal);

		}
		else {
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
			(LPVOID)&gslFillHook,
			(LPVOID*)&gameExegslFillReal);


		scanCreateEnableHook(
			"game", "drawTwipoContent", (uintptr_t*)&rnDrawTwipoContent,
			(LPVOID)drawTwipoContentHook, (LPVOID*)&rnDrawTwipoContentReal);

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
		}
		else if (config["gamedef"].count("dialoguePageVersion") == 1 &&
			config["gamedef"]["dialoguePageVersion"].get<std::string>() == "rn") {
			gameExeDialoguePages_RNDialoguePage_t =
				(RNDialoguePage_t*)sigScan("game", "useOfDialoguePages");
			if (IMPROVE_DIALOGUE_OUTLINES) {
				scanCreateEnableHook(
					"game", "drawDialogue", (uintptr_t*)&gameExeDrawDialogue,
					(LPVOID)rnDrawDialogueHook, (LPVOID*)&gameExeDrawDialogueReal);
				scanCreateEnableHook(
					"game", "drawDialogue2", (uintptr_t*)&gameExeDrawDialogue2,
					(LPVOID)rnDrawDialogue2Hook, (LPVOID*)&gameExeDrawDialogue2Real);
			}
		}
		else {
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

		/*  scanCreateEnableHook("game", "dialogueLayoutRelated",
					 (uintptr_t *)&gameExeDialogueLayoutRelated,
					 (LPVOID)dialogueLayoutRelatedHook,
					 (LPVOID *)&gameExeDialogueLayoutRelatedReal);*/
		if (HAS_DRAW_PHONE_TEXT) {
			scanCreateEnableHook(
				"game", "drawPhoneText", (uintptr_t*)&gameExeDrawPhoneText,
				(LPVOID)drawPhoneTextHook, (LPVOID*)&gameExeDrawPhoneTextReal);
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
				(uintptr_t*)&gameExeDrawSingleTextLine,
				(LPVOID)drawSingleTextLineHook,
				(LPVOID*)&gameExeDrawSingleTextLineReal);
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
			void* target;
			if (config["gamedef"].count("ccBacklogNamePosAdjustVersion") == 1 &&
				config["gamedef"]["ccBacklogNamePosAdjustVersion"].get<std::string>() ==
				"ccsteam") {
				target = ccSteamBacklogNamePosAdjustHook;
			}
			else {
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
			/*scanCreateEnableHook("game", "drawSprite", (uintptr_t*)&gameExeDrawSprite,
				(LPVOID)drawSpriteHook,
				(LPVOID*)&gameExeDrawSpriteReal);*/
		}

		if (true) {
			scanCreateEnableHook("game", "drawBacklogContent", (uintptr_t*)&gameExeDrawBacklogContent,
				(LPVOID)DrawBacklogContentHook,
				(LPVOID*)&gameExeDrawBacklogContentReal);
			gameExeDrawSpriteReal = (DrawSpriteProc)sigScan("game", "drawSprite", false);

		/*	scanCreateEnableHook("game", "drawSprite", (uintptr_t*)&gameExeDrawSprite,
				(LPVOID)drawSpriteHook,
				(LPVOID*)&gameExeDrawSpriteReal);*/
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
		}
		else {
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
			(uint8_t*)gameExeDialogueLayoutWidthLookup1 + lookup1retoffset);
		scanCreateEnableHook("game", "dialogueLayoutWidthLookup2",
			&gameExeDialogueLayoutWidthLookup2,
			dialogueLayoutWidthLookup2Hook, NULL);
		gameExeDialogueLayoutWidthLookup2Return = (uintptr_t)(
			(uint8_t*)gameExeDialogueLayoutWidthLookup2 + lookup2retoffset);
		//scanCreateEnableHook("game", "dialogueLayoutWidthLookup3",
		//  &gameExeDialogueLayoutWidthLookup3,
		//  dialogueLayoutWidthLookup3Hook, NULL);
		//gameExeDialogueLayoutWidthLookup3Return = (uintptr_t)(
		//  (uint8_t*)gameExeDialogueLayoutWidthLookup3 + lookup3retoffset);
		if (signatures.count("tipsListWidthLookup") == 1) {
			configretoffset = signatures["tipsListWidthLookup"].value<int>("return", 0);
			if (configretoffset)
				tipsListWidthRetoffset = configretoffset;
			scanCreateEnableHook("game", "tipsListWidthLookup",
				&gameExeTipsListWidthLookup,
				tipsListWidthLookupHook, NULL);
			gameExeTipsListWidthLookupReturn =
				(uintptr_t)((uint8_t*)gameExeTipsListWidthLookup + tipsListWidthRetoffset);
		}
		if (signatures.count("getRineInputRectangle") == 1) {
			scanCreateEnableHook("game", "getRineInputRectangle",
				(uintptr_t*)&gameExeGetRineInputRectangle,
				(LPVOID)getRineInputRectangleHook,
				(LPVOID*)&gameExeGetRineInputRectangleReal);
		}
		if (signatures.count("calcSpeakerNameLength")) {
			unsigned char* ptr = (unsigned char*)sigScan("game", "calcSpeakerNameLength");
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

		/*FILE* widthsfile = fopen("languagebarrier\\widths.bin", "rb");
		fread(widths, 1, TOTAL_NUM_FONT_CELLS, widthsfile);
		fclose(widthsfile);
		memcpy(gameExeGlyphWidthsFont1, widths, GLYPH_RANGE_FULLWIDTH_START);
		memcpy(gameExeGlyphWidthsFont2, widths, GLYPH_RANGE_FULLWIDTH_START);*/
		TextRendering::Instance().Init(gameExeGlyphWidthsFont1, gameExeGlyphWidthsFont2);

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




#define DEF_DRAW_DIALOGUE_HOOK(funcName, pageType)               \
                                         \
  void __cdecl funcName(int fontNumber, int pageNumber, uint32_t opacity,    \
            int xOffset, int yOffset) {              \
  pageType *page = &gameExeDialoguePages_##pageType[pageNumber];       \
                                         \
  for (int i = 0; i < page->pageLength; i++) {                 \
    if (fontNumber == page->fontNumber[i]) {                 \
    int displayStartX =                          \
      (page->charDisplayX[i] + xOffset) * COORDS_MULTIPLIER;       \
    int displayStartY =                          \
      (page->charDisplayY[i] + yOffset) * COORDS_MULTIPLIER;       \
                                         \
    uint32_t _opacity = (page->charDisplayOpacity[i] * opacity) >> 8;    \
                                         \
    if (page->charOutlineColor[i] != -1) {                 \
      gameExeDrawGlyph(                          \
        OUTLINE_TEXTURE_ID,                        \
        OUTLINE_CELL_WIDTH * page->glyphCol[i] * COORDS_MULTIPLIER,    \
        OUTLINE_CELL_HEIGHT * page->glyphRow[i] * COORDS_MULTIPLIER,   \
        page->glyphOrigWidth[i] * COORDS_MULTIPLIER +          \
          (2 * OUTLINE_PADDING),                     \
        page->glyphOrigHeight[i] * COORDS_MULTIPLIER +           \
          (2 * OUTLINE_PADDING),                     \
        displayStartX - OUTLINE_PADDING,                 \
        displayStartY - OUTLINE_PADDING,                 \
        displayStartX +                          \
          (COORDS_MULTIPLIER * page->glyphDisplayWidth[i]) +       \
          OUTLINE_PADDING,                       \
        displayStartY +                          \
          (COORDS_MULTIPLIER * page->glyphDisplayHeight[i]) +      \
          OUTLINE_PADDING,                       \
        page->charOutlineColor[i], _opacity);              \
    }                                    \
                                         \
    gameExeDrawGlyph(                            \
      FIRST_FONT_ID,                    \
      FONT_CELL_WIDTH * page->glyphCol[i] * COORDS_MULTIPLIER,       \
      FONT_CELL_HEIGHT * page->glyphRow[i] * COORDS_MULTIPLIER,      \
      page->glyphOrigWidth[i] * COORDS_MULTIPLIER,             \
      page->glyphOrigHeight[i] * COORDS_MULTIPLIER, displayStartX,     \
      displayStartY,                           \
      displayStartX + (COORDS_MULTIPLIER * page->glyphDisplayWidth[i]),  \
      displayStartY + (COORDS_MULTIPLIER * page->glyphDisplayHeight[i]), \
      page->charColor[i], _opacity);                   \
    }                                    \
  }                                      \
  }
	DEF_DRAW_DIALOGUE_HOOK(drawDialogueHook, DialoguePage_t);
	DEF_DRAW_DIALOGUE_HOOK(ccDrawDialogueHook, CCDialoguePage_t);
	//DEF_DRAW_DIALOGUE_HOOK(rnDrawDialogueHook, RNDialoguePage_t);



	void __cdecl rnDrawDialogueHook(int fontNumber, int pageNumber, uint32_t opacity,
		int xOffset, int yOffset) {
		RNDialoguePage_t* page = &gameExeDialoguePages_RNDialoguePage_t[pageNumber];
		if (GetAsyncKeyState(VK_RBUTTON)) {
			TextRendering::Instance().enableReplacement();

		}
		else if (GetAsyncKeyState(VK_LBUTTON)) {
			TextRendering::Instance().disableReplacement();
		}

		if (!TextRendering::Instance().enabled)
			return gameExeDrawDialogueReal(fontNumber, pageNumber, opacity, xOffset, yOffset);

		bool newline = true;
		float displayStartX =
			(page->charDisplayX[0] + xOffset) * COORDS_MULTIPLIER;
		float displayStartY =
			(page->charDisplayY[0] + yOffset) * COORDS_MULTIPLIER;
		for (int i = 0; i < page->pageLength; i++) {
			if (fontNumber == page->fontNumber[i]) {

				int glyphSize = page->glyphDisplayHeight[i];
				if (i == 0 || i > 0 && page->charDisplayY[i] != page->charDisplayY[i - 1]) {
					newline = true;
				}
				else newline = false;

				if (newline == false) {
					uint32_t currentChar = page->glyphCol[i - 1] + page->glyphRow[i - 1] * TextRendering::Instance().GLYPHS_PER_ROW;
					auto glyphInfo = TextRendering::Instance().getFont(glyphSize, false)->getGlyphInfo(currentChar, Regular);
					displayStartX += glyphInfo->advance * 1.5f;
				}
				else {
					displayStartX = (page->charDisplayX[i] + xOffset) * COORDS_MULTIPLIER;
				}



				uint32_t _opacity = (page->charDisplayOpacity[i] * opacity) >> 8;

				if (page->charOutlineColor[i] != -1) {

					{
						uint32_t currentChar = page->glyphCol[i] + page->glyphRow[i] * TextRendering::Instance().GLYPHS_PER_ROW;
						wchar_t cChar = TextRendering::Instance().charMap[currentChar];
						const auto glyphInfo = TextRendering::Instance().getFont(page->glyphDisplayHeight[i] * 1.5f,false)->getGlyphInfo(currentChar, FontType::Outline);
						displayStartY =
							(page->charDisplayY[i] + yOffset) * 1.5f;


						__int16 fontSize = page->glyphDisplayHeight[i] * 1.5f;
						TextRendering::Instance().replaceFontSurface(fontSize);
						if (glyphInfo->width && glyphInfo->rows)

						gameExeDrawSpriteReal(
							TextRendering::Instance().OUTLINE_TEXTURE_ID,
							TextRendering::Instance().FONT_CELL_SIZE * page->glyphCol[i],
							TextRendering::Instance().FONT_CELL_SIZE * page->glyphRow[i],
							glyphInfo->width,
							glyphInfo->rows, displayStartX + glyphInfo->left,
							displayStartY + fontSize - glyphInfo->top,
							page->charOutlineColor[i], _opacity,4);
					}
				}



				{
					uint32_t currentChar = page->glyphCol[i] + page->glyphRow[i] * TextRendering::Instance().GLYPHS_PER_ROW;
					auto glyphInfo = TextRendering::Instance().getFont(page->glyphDisplayHeight[i] * 1.5f,false)->getGlyphInfo(currentChar, FontType::Regular);
					//   if (page->charDisplayX[i + 1] > page->charDisplayX[i] && (i + 1) < page->pageLength) {
					 //    page->charDisplayX[i + 1] = page->charDisplayX[i]+ glyphInfo.advance  /3.0f;
					//}

					displayStartY =
						(page->charDisplayY[i] + yOffset) * 1.5f;
					float xRatio = ((float)page->glyphDisplayWidth[i] / (float)page->glyphOrigWidth[i]);

					TextRendering::Instance().replaceFontSurface(page->glyphDisplayHeight[i] * 1.5);
					__int16 fontSize = page->glyphDisplayHeight[i] * 1.5f;
					if(glyphInfo->width && glyphInfo->rows)
					gameExeDrawSpriteReal(
						TextRendering::Instance().FONT_TEXTURE_ID,
						TextRendering::Instance().FONT_CELL_SIZE * page->glyphCol[i],
						TextRendering::Instance().FONT_CELL_SIZE * page->glyphRow[i],
						glyphInfo->width,
						glyphInfo->rows, displayStartX + glyphInfo->left,
						displayStartY + fontSize - glyphInfo->top,
						page->charColor[i], _opacity,4);
				}


			}
		}
	}


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

	void semiTokeniseSc3String(char* sc3string, std::list<StringWord_t>& words,
		int baseGlyphSize, int lineLength) {
		if (HAS_SGHD_PHONE) {
			lineLength -= 2 * SGHD_PHONE_X_PADDING;
		}

		ScriptThreadState sc3;
		int sc3evalResult;
		StringWord_t word = { sc3string, NULL, 0, false, false };
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
				word = { ++sc3string, NULL, 0, false, false };
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
				if (!TextRendering::Instance().enabled) {

					glyphWidth =
						(baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
				}
				else {
					const auto& fontData = TextRendering::Instance().getFont(baseGlyphSize, true);
					glyphWidth =
						fontData->glyphData.glyphMap[TextRendering::Instance().charMap[glyphId]].advance;
				}
				if (glyphId == GLYPH_ID_FULLWIDTH_SPACE ||
					glyphId == GLYPH_ID_HALFWIDTH_SPACE) {
					word.end = sc3string - 1;
					words.push_back(word);
					word = { sc3string, NULL, glyphWidth, true, false };
				}
				else {
					if (word.cost + glyphWidth > lineLength) {
						word.end = sc3string - 1;
						words.push_back(word);
						word = { sc3string, NULL, 0, false, false };
					}
					word.cost += glyphWidth;
				}
				sc3string += 2;
				break;
			}
		}
	}

	struct CColor
	{
		float r;
		float g;
		float b;
		float a;
	};

	void __cdecl initColor(CColor* a1, int a2)
	{
		if (a2 & 0xFFFFFF)
		{
			if ((a2 & 0xFFFFFF) == 0xFFFFFF)
			{
				a1->b = 1.0;
				a1->g = 1.0;
				a1->r = 1.0;
				a1->a = 1.0;
			}
			else
			{
				a1->a = 1.0;
				a1->r = (float)(a2 >> 16 & 0xFF) / 255.0;
				a1->g = (float)(a2 >> 8 & 0xFF) / 255.0;
				a1->b = (float)(unsigned __int8)(a2 & 0xFF) / 255.0;
			}
		}
		else
		{
			a1->b = 0.0;
			a1->g = 0.0;
			a1->r = 0.0;
			a1->a = 1.0;
		}
	}



	unsigned int __cdecl sub_4BB760(int textureId, int maskTextureId, int textureStartX, int textureStartY, float textureSizeX, float textureSizeY, float startPosX, float startPosY, float EndPosX, float EndPosY, float color, float opacity)
	{
		float a3[4]; // [esp+14h] [ebp-7Ch]
		SurfaceStruct* a1[2]; // [esp+24h] [ebp-6Ch]
		CColor colorStruct; // [esp+2Ch] [ebp-64h]
		float a4[8]; // [esp+6Ch] [ebp-24h]



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
		a1[0] = &surfaceArray[textureId];
		a1[1] = &surfaceArray[maskTextureId];

		int shaderPtr = *(int*)0x099DEDC;
		int blendMode = *(int*)0x099DEE8;
		int renderMode = *(int*)0x099DEEC;

		return GameExeDrawSpriteMaskInternal(
			(float*)&a1,
			2,
			a3,
			a4,
			&colorStruct,
			(float)opacity / 255.0,
			renderMode,
			shaderPtr,
			(unsigned __int16)blendMode,
			0);
	}

#pragma comment( lib, "dxguid.lib") 



	void __cdecl DrawBacklogContentHook(int textureId, int maskTextureId, int startX, int startY, unsigned int maskY, int maskHeight, int opacity, int index)
	{
		unsigned int v8; // edi
		unsigned int v9; // ecx
		unsigned int v10; // esi
		int v11; // edx
		unsigned int v12; // ebx
		int linePos; // eax
		int v14; // ebx
		int strIndex; // esi
		unsigned int charIndex; // edx
		__int16 v17; // dx
		int v18; // eax
		int   colorIndex; // ecx
		unsigned int v20; // edx
		float xPosition; // edi
		int v22; // eax
		int v23; // eax
		unsigned int v24; // edx
		unsigned int v25; // ebx
		unsigned int v26; // ecx
		unsigned int v29; // ecx
		int v30; // ebx
		unsigned int v31; // ecx
		int color; // edi
		int v33; // eax
		int v34; // eax
		int v35; // [esp+0h] [ebp-34h]
		int yPosition; // [esp+14h] [ebp-20h]
		int v38; // [esp+18h] [ebp-1Ch]
		int v39; // [esp+18h] [ebp-1Ch]
		int v40; // [esp+1Ch] [ebp-18h]
		int endP; // [esp+20h] [ebp-14h]
		int v42; // [esp+24h] [ebp-10h]
		int v43; // [esp+24h] [ebp-10h]
		int startPosY; // [esp+28h] [ebp-Ch]
		int v45; // [esp+2Ch] [ebp-8h]
		unsigned int v46; // [esp+2Ch] [ebp-8h]
		int v47; // [esp+30h] [ebp-4h]

		uint16_t* MESrevTextFont = (uint16_t*)0x078CBB0;
		int* MESrevDispCurPosEY = (int*)0x079A450;
		int* MESrevDispCurPosSX = (int*)0x079AA90;
		int* MESrevDispCurPosSY = (int*)0x079B0D0;
		int* MESrevDispCurPosEX = (int*)0x079B710;
		int* MESrevBufUse = (int*)0x079BF38;
		int* MESrevBufStartp = (int*)0x079BF3C;
		uint16_t* MesRevText = (uint16_t*)0x079BF40;
		uint16_t* MesRevTextPos = (uint16_t*)0x07B45E0;
		unsigned char* MESrevTextSize = (unsigned char*)0x07E5320;
		uint8_t* MESrevTextCo = (uint8_t*)0x0816060;
		int MESrevLineBufUse = *(int*)0x08223B0;
		int* MESrevLineBufStartp = (int*)0x08223B4;
		int* MESrevLineBufSize = (int*)0x08223C0;
		int* MESrevLineBufEndp = (int*)0x0853100;
		int* MESrevLineVoice = (int*)0x08B4B80;
		int* MESrevLineSave = (int*)0x0916600;
		int MESrevDispPos = *(int*)0x094734C;
		int* MESrevDispLinePos = (int*)0x0947360;
		int* MESrevDispLineSize = (int*)0x09479A8;
		int* MESrevDispLinePosY = (int*)0x0947FE8;
		int* MesFontColor = (int*)0x6DF598;
		int* dword_948628 = (int*)0x948628;

		if (!TextRendering::Instance().enabled)
		{
			return gameExeDrawBacklogContentReal(textureId, maskTextureId, startX, startY, maskY, maskHeight, opacity, index);
		}





		v40 = 0;
		v47 = 0;
		if (MESrevLineBufUse)
		{
			v8 = MESrevLineBufUse;
			v9 = 0;
			startPosY = 0;
			if (MESrevLineBufUse)
			{
				v10 = maskY;
				do
				{
					v11 = 0xFFFF;
					v12 = startY + MESrevDispLinePosY[v9] - MESrevDispPos;
					v45 = 0xFFFF;
					v42 = 0xFFFF;
					yPosition = startY + MESrevDispLinePosY[v9] - MESrevDispPos;
					v38 = 0;
					if (v12 + MESrevDispLineSize[v9] > v10 && v12 < v10 + maskHeight)
					{
						linePos = MESrevDispLinePos[v9];
						v14 = 0;
						strIndex = MESrevLineBufSize[linePos];
						endP = MESrevLineBufEndp[linePos];
						int xOffset = MesRevTextPos[2 * strIndex];
						if (endP)
						{
							xPosition = startX;

							do
							{
								charIndex = MesRevText[strIndex];
								int xOffset = 0;
								bool newline = true;
								if ((charIndex & 0x8000u) == 0)
								{
									colorIndex = MESrevTextCo[strIndex];
									color = MesFontColor[2 * colorIndex + 1];
									auto glyphSize = MESrevTextSize[4 * strIndex + 3] * 1.5f;

									if (strIndex == 0 || strIndex > 0 && MesRevTextPos[2 * (strIndex)+1] != MesRevTextPos[2 * (strIndex - 1) + 1]) {
										newline = true;
									}
									else newline = false;

									if (newline == false) {
										auto glyphInfo = TextRendering::Instance().getFont(glyphSize, true)->getGlyphInfo(MesRevText[strIndex - 1], Regular);
										xPosition += glyphInfo->advance / 2.0f;
									}
									else {
										xPosition = startX + MesRevTextPos[2 * strIndex];
									}
									v47 = yPosition + MesRevTextPos[2 * strIndex + 1];
									v43 = MESrevTextSize[4 * strIndex + 2];
									v22 = yPosition + MesRevTextPos[2 * strIndex + 1];
									if (MesFontColor[2 * colorIndex] != 0xFFFFFF || (v9 = startPosY, startPosY != index))
									{
										TextRendering::Instance().replaceFontSurface(glyphSize);
										auto glyphInfo = TextRendering::Instance().getFont(glyphSize, false)->getGlyphInfo(MesRevText[strIndex], Regular);
										sub_4BB760(
											400,
											maskTextureId,
											TextRendering::Instance().FONT_CELL_SIZE * (MesRevText[strIndex] % TextRendering::Instance().GLYPHS_PER_ROW),
											TextRendering::Instance().FONT_CELL_SIZE * (MesRevText[strIndex] / TextRendering::Instance().GLYPHS_PER_ROW),
											glyphInfo->width * 2,
											glyphInfo->rows * 2,
											xPosition + glyphInfo->left / 2.0f + 1,
											v47 + glyphSize / 2.0f - glyphInfo->top / 2.0f + 1,
											xPosition + glyphInfo->left / 2.0f + glyphInfo->width + 1,
											glyphInfo->rows + glyphSize / 2.0f - glyphInfo->top / 2.0f + 1 + v47,
											color,
											opacity);
										v9 = startPosY;
										v22 = v47;
									}
									if (!v14 && v45 == 0xFFFF)
									{
										v45 = xPosition;
										v40 = v22;
									}
									v42 = xPosition + v43;
								}
								else
								{
									v17 = charIndex & 0x7FFF;
									if (v17 == 1)
									{
										v14 = 1;
										v38 = 1;
									}
									v18 = 0;
									if (v17 != 2)
										v18 = v14;
									v14 = v18;
								}
								v23 = strIndex + 1;
								strIndex = 0;
								if (v23 != 50000)
									strIndex = v23;
								--endP;
							} while (endP);
							v8 = MESrevLineBufUse;
							v11 = v45;
						}
						v10 = maskY;
					}
					MESrevDispCurPosSX[v9] = v40;
					MESrevDispCurPosSY[v9] = v42;
					MESrevDispCurPosEX[v9] = v47;
					MESrevDispCurPosEY[v9] = v11;
					dword_948628[v9++] = v38;
					startPosY = v9;
				} while (v9 < v8);
			}
			v24 = 0;
			v46 = 0;
			if (v8)
			{
				v25 = maskY;
				do
				{
					v26 = startY + MESrevDispLinePosY[v24] - MESrevDispPos;
					v35 = startY + MESrevDispLinePosY[v24] - MESrevDispPos;
					if (v26 + MESrevDispLineSize[v24] > v25 && v26 < v25 + maskHeight)
					{
						linePos = MESrevDispLinePos[v24];
						strIndex = MESrevLineBufSize[linePos];
						v39 = MESrevLineBufEndp[linePos];
						if (v39)
						{
							xPosition = startX;
							bool newline = true;
							do
							{
								v29 = MesRevText[strIndex];
								if ((v29 & 0x8000u) == 0)
								{

									colorIndex = MESrevTextCo[strIndex];
									color = MesFontColor[2 * colorIndex + 1];
									auto glyphSize = MESrevTextSize[4 * strIndex + 3] * 1.5f;

									if (strIndex == 0 || strIndex > 0 && MesRevTextPos[2 * (strIndex)+1] != MesRevTextPos[2 * (strIndex - 1) + 1]) {
										newline = true;
									}
									else newline = false;

									if (newline == false) {
										auto glyphInfo = TextRendering::Instance().getFont(glyphSize, false)->getGlyphInfo(MesRevText[strIndex - 1], Regular);
										xPosition += glyphInfo->advance / 2.0f;
									}
									else {
										xPosition = startX + MesRevTextPos[2 * strIndex];
									}


									color = MesFontColor[2 * MESrevTextCo[strIndex]];
									v33 = v35 + MesRevTextPos[2 * strIndex + 1];
									if (color == 0xFFFFFF)
									{
										if (v46 == index)
											color = 0x323232;
										v33 = v35 + MesRevTextPos[2 * strIndex + 1];
									}
									glyphSize = MESrevTextSize[4 * strIndex + 3] * 1.5f;

									TextRendering::Instance().replaceFontSurface(glyphSize);
									auto glyphInfo = TextRendering::Instance().getFont(glyphSize, false)->getGlyphInfo(MesRevText[strIndex], Regular);
									sub_4BB760(
										400,
										maskTextureId,
										TextRendering::Instance().FONT_CELL_SIZE * (MesRevText[strIndex] % TextRendering::Instance().GLYPHS_PER_ROW),
										TextRendering::Instance().FONT_CELL_SIZE * (MesRevText[strIndex] / TextRendering::Instance().GLYPHS_PER_ROW),
										glyphInfo->width * 2,
										glyphInfo->rows * 2,
										xPosition + glyphInfo->left / 2.0f,
										MesRevTextPos[2 * strIndex + 1] + v35 + glyphSize / 2.0f - glyphInfo->top / 2.0f,
										xPosition + glyphInfo->left / 2.0f + glyphInfo->width,
										MesRevTextPos[2 * strIndex + 1] + glyphInfo->rows + glyphSize / 2.0f - glyphInfo->top / 2.0f + v35,
										color,
										opacity);



								}
								v34 = strIndex + 1;
								strIndex = 0;
								if (v34 != 0xC350)
									strIndex = v34;
								--v39;
							} while (v39);
							v8 = MESrevLineBufUse;
							v24 = v46;
							v25 = maskY;
						}
					}
					v46 = ++v24;
				} while (v24 < v8);
			}
		}
	}



	int __cdecl drawTwipoContentHook(int textureId, int a2, int a3, unsigned int a4, int a5, unsigned int a6, char* sc3, int a8, int a9, uint32_t opacity, int a11, int a12, int a13, int a14) {

		// if (!lineLength) lineLength = DEFAULT_LINE_LENGTH;
		int lineLength = DEFAULT_LINE_LENGTH;
		std::list<StringWord_t> words;


		if (!TextRendering::Instance().enabled) {
			return  rnDrawTwipoContentReal(textureId, a2, a3, a4, a5, a6, sc3, a8, a9, opacity, a11, a12, a13, a14);
			;
		}
		semiTokeniseSc3String(sc3, words, 32, lineLength);
		int xOffset, yOffset;
		xOffset = 0;
		yOffset = 0;
		int color = a8;
		int lineSkipCount = 1;
		int lineDisplayCount = 0;
		lineLength = a4;
		const int glyphSize = a9 * 1.5;

		ProcessedSc3String_t str;
		MultiplierData mData;
		mData.xOffset = 1.5f;
		mData.yOffset = 1.5f;



		processSc3TokenList(a2, a3, lineLength, words, a5, color,
			glyphSize, &str, true, COORDS_MULTIPLIER, -1,
			NOT_A_LINK, color, glyphSize, &mData);

		processSc3TokenList(a2, a3, lineLength, words, a5,
			color, glyphSize, &str, false, COORDS_MULTIPLIER,
			str.linkCount - 1, str.curLinkNumber, str.curColor, glyphSize, &mData);


		TextRendering::Instance().replaceFontSurface(glyphSize);

		for (int i = 0; i < str.length; i++) {
			int curColor = str.color[i];
			auto glyphInfo = TextRendering::Instance().getFont(glyphSize, false)->getGlyphInfo(str.glyph[i], Regular);

			if (str.linkNumber[i] != NOT_A_LINK) {

				auto linkGlyphInfo = TextRendering::Instance().getFont(glyphSize, false)->getGlyphInfo(77, Regular);


				if (str.linkNumber[i] == a12)
					curColor = a8;
				else
					curColor = a11;

				float endUnderScoreX = str.displayStartX[i] + glyphInfo->advance;


				if (i + 1 < str.length && str.displayStartY[i] == str.displayStartY[i + 1]) {
					endUnderScoreX = str.displayStartX[i + 1];
				}

				const int underScoreIndex = 77;
				gameExeDrawGlyph(TextRendering::Instance().FONT_TEXTURE_ID, underScoreIndex % TextRendering::Instance().GLYPHS_PER_ROW * TextRendering::Instance().FONT_CELL_SIZE,
					underScoreIndex / TextRendering::Instance().GLYPHS_PER_ROW * TextRendering::Instance().FONT_CELL_SIZE, linkGlyphInfo->width,
					linkGlyphInfo->rows, str.displayStartX[i],
					str.displayStartY[i] + glyphSize - linkGlyphInfo->top, endUnderScoreX,
					str.displayStartY[i] + glyphSize + linkGlyphInfo->rows * 2 - linkGlyphInfo->top, curColor, opacity);
			}


			gameExeDrawGlyph(TextRendering::Instance().FONT_TEXTURE_ID, str.textureStartX[i], str.textureStartY[i],
				str.textureWidth[i], str.textureHeight[i],
				str.displayStartX[i], str.displayStartY[i] + glyphSize - glyphInfo->top,
				str.displayEndX[i], str.displayEndY[i] + glyphSize - glyphInfo->top, curColor, opacity);
		}
		return 1;
	}

	float addCharacter(ProcessedSc3String_t* result, int baseGlyphSize, int glyphId, int lineCount,
		int curLinkNumber, bool measureOnly, float multiplier, int xOffset, int& curLineLength,
		int yOffset, int currentColor, int lineHeight, const MultiplierData* mData)
	{
		int i = result->length;
		const auto& fontData = TextRendering::Instance().getFont(baseGlyphSize, false);
		char character = TextRendering::Instance().charMap[glyphId];
		result->text[i] = character;
		if (curLinkNumber != NOT_A_LINK) {
			result->linkCharCount++;
		}
		uint16_t glyphWidth = 0;
		if (!TextRendering::Instance().enabled) {

			glyphWidth =
				(baseGlyphSize * widths[glyphId]) / FONT_CELL_WIDTH;
		}
		else {
			glyphWidth =
				fontData->getGlyphInfo(glyphId, FontType::Regular)->advance;
		}
		if (!measureOnly) {
			// anything that's part of an array needs to go here, otherwise we
			// get buffer overflows with long mails
			if (!TextRendering::Instance().enabled) {
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
			else {
				auto glyphInfo = fontData->getGlyphInfo(glyphId, FontType::Regular);
				multiplier = 1.0f;
				result->linkNumber[i] = curLinkNumber;
				result->glyph[i] = glyphId;
				result->textureStartX[i] =
					TextRendering::Instance().FONT_CELL_SIZE * multiplier * (glyphId % FONT_ROW_LENGTH);
				result->textureStartY[i] =
					TextRendering::Instance().FONT_CELL_SIZE * multiplier * (glyphId / FONT_ROW_LENGTH);
				result->textureWidth[i] = glyphInfo->width * multiplier;
				result->textureHeight[i] = baseGlyphSize * multiplier;
				result->displayStartX[i] =
					(xOffset * mData->xOffset + (curLineLength)+glyphInfo->left) * multiplier;
				result->displayStartY[i] =
					(yOffset * mData->yOffset + (result->lines * lineHeight)) * multiplier;
				result->displayEndX[i] = (xOffset * mData->xOffset + curLineLength + glyphInfo->width + glyphInfo->left) * multiplier;
				result->displayEndY[i] =
					(yOffset * mData->yOffset + (result->lines) * lineHeight + baseGlyphSize) * multiplier;
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
		int curLinkNumber, int currentColor, int lineHeight, const MultiplierData* mData) {
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

		int spaceCost =
			(widths[GLYPH_ID_FULLWIDTH_SPACE] * baseGlyphSize) / FONT_CELL_WIDTH;


		MultiplierData multiplierData;
		if (mData != NULL) {
			multiplierData = *mData;
		}



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
				result->lines--;
				curLineLength = prevLineLength;

				for (int i = 0; i < 3; i++) {
					addCharacter(result, baseGlyphSize, TextRendering::Instance().charMap.find('.'), lineCount - 1, curLinkNumber, false, 1.0, xOffset, curLineLength, yOffset, currentColor, lineHeight, mData);
				}
				words.erase(words.begin(), it);
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
					auto glyphWidth = addCharacter(result, baseGlyphSize, glyphId, lineCount, curLinkNumber, measureOnly, multiplier, xOffset, curLineLength, yOffset, currentColor, lineHeight, mData);


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
			str.linkCount - 1, str.curLinkNumber, str.curColor, baseGlyphSize, nullptr);

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
			if (fixIter->second.fontSize)
				glyphSize = fixIter->second.fontSize;
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
		while (i <= maxCharacters && (c = *sc3string) != -1) {
			if (c == 4) {
				sc3.pc = sc3string + 1;
				gameExeSc3Eval(&sc3, &sc3evalResult);
				sc3string = (char*)sc3.pc;
			}
			else if (c < 0) {
				int glyphId = (uint8_t)sc3string[1] + ((c & 0x7f) << 8);
				if(TextRendering::Instance().enabled)
				result += TextRendering::Instance().getFont(baseGlyphSize, true)->getGlyphInfo(glyphId, Regular)->advance;
				else {
					result += TextRendering::Instance().originalWidth[glyphId];

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
			baseGlyphSize, &str, true, 1.0f, -1, NOT_A_LINK, 0, baseGlyphSize, NULL);
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
			str.linkCount - 1, str.curLinkNumber, str.curColor, baseGlyphSize, NULL);

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
			str.linkCount - 1, str.curLinkNumber, str.curColor, baseGlyphSize, NULL);

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
			baseGlyphSize, &str, true, 1.0f, -1, NOT_A_LINK, 0, baseGlyphSize, NULL);
		return str.lines + 1;
	}
	int __cdecl getRineInputRectangleHook(int* lineLength, char* text, unsigned int baseGlyphSize) {
		ProcessedSc3String_t str;
		int maxLineLength = (lineLength && *lineLength ? *lineLength : DEFAULT_LINE_LENGTH);

		std::list<StringWord_t> words;
		semiTokeniseSc3String(text, words, baseGlyphSize, maxLineLength);
		processSc3TokenList(0, 0, maxLineLength, words, LINECOUNT_DISABLE_OR_ERROR, 0,
			baseGlyphSize, &str, true, 1.0f, -1, NOT_A_LINK, 0, baseGlyphSize, NULL);
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

	int rnDrawGlyphHook(int textureId, float glyphInTextureStartX,
		float glyphInTextureStartY, float glyphInTextureWidth,
		float glyphInTextureHeight, float displayStartX,
		float displayStartY, float displayEndX, float displayEndY,
		int color, uint32_t opacity) {


		if (TextRendering::Instance().enabled) {
			if (textureId == FIRST_FONT_ID)
				textureId = TextRendering::Instance().FONT_TEXTURE_ID;
			if (textureId == OUTLINE_TEXTURE_ID)
				textureId = TextRendering::Instance().OUTLINE_TEXTURE_ID;
			//	TextRendering::Instance().replaceFontSurface(glyphInTextureHeight);

		}

		return gameExeDrawGlyphReal(textureId, glyphInTextureStartX,
			glyphInTextureStartY, glyphInTextureWidth,
			glyphInTextureHeight, displayStartX,
			displayStartY, displayEndX, displayEndY,
			color, opacity);

	}

	int __cdecl rnDrawTextHook(signed int textureId, int a2, signed int startY, unsigned int a4, uint8_t* sc3, signed int startX, int color, int height, int opacity)
	{
		// if (!startX) startX = 255;

	
	
		int length = 0;

		bool finish = false;
		int sc3Index = 0;
		std::vector<uint16_t> v;
		std::vector<wchar_t> v2;
		if (GetAsyncKeyState(VK_RBUTTON)) {
			TextRendering::Instance().enableReplacement();
		}
		if (GetAsyncKeyState(VK_LBUTTON)) {
			TextRendering::Instance().disableReplacement();

		}


		if (TextRendering::Instance().enabled) {

			if (a4 == 0x104 && height == 0x18) {
				a4 *= 1.33;
				height *= 1.33;
			}
			int width = a4 * 1.5 + 1;
			while (width > a4 * 1.5) {
				width = getSc3StringDisplayWidthHook((char*)sc3, 0, height * 1.5);
				height--;
			}


			while (!finish) {

				if (sc3[sc3Index] == 0xFF) {
					finish = true;
				}
				else
					if (sc3[sc3Index] >= 0x80) {

						uint16_t value = (sc3[sc3Index] >> 8 | sc3[sc3Index + 1]) & 0x7FFF;
						v.push_back(value);
						v2.push_back(TextRendering::Instance().charMap[value]);
						sc3Index += 2;
					}
					else {
						sc3Index++;
					}

			}
			int displayStartX = a2 * 1.5f;

			for (int i = 0; i < v.size(); i++) {
				uint32_t currentChar = v[i];
				auto fontData = TextRendering::Instance().getFont(height * 1.5f, false);

				auto glyphInfo = fontData->getGlyphInfo(currentChar, FontType::Regular);

				int column = currentChar % TextRendering::Instance().GLYPHS_PER_ROW;
				int row = currentChar / TextRendering::Instance().GLYPHS_PER_ROW;

				int displayStartY = startY * 1.5f;
				TextRendering::Instance().replaceFontSurface(height * 1.5f);
				if (glyphInfo->width && glyphInfo->rows) {
					gameExeDrawSpriteReal(
						400, TextRendering::Instance().FONT_CELL_SIZE * column, TextRendering::Instance().FONT_CELL_SIZE * row,
						glyphInfo->width, glyphInfo->rows, (displayStartX + glyphInfo->left), (displayStartY + height - glyphInfo->top)
						, color, opacity, 4);
				}

				displayStartX += glyphInfo->advance;

			}
		}
		else {
			rnDrawTextReal(textureId, a2, startY, a4, sc3, startX, color, height, opacity);
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

		if (TextRendering::Instance().enabled)
			return gameExeSg0DrawGlyph2Real(TextRendering::Instance().FONT_TEXTURE_ID, a2, glyphInTextureStartX / 1.5f,
				glyphInTextureStartY / 1.5f, glyphInTextureWidth / 1.5f,
				glyphInTextureHeight / 1.5f, a7 / 2.0f, a8 / 2.0f, a9 / 2.0f, a10 / 2.0f, a11 / 2.0f,
				a12 / 2.0f, inColor, opacity, a15, a16);
		else {
			return gameExeSg0DrawGlyph2Real(textureId, a2, glyphInTextureStartX,
				glyphInTextureStartY, glyphInTextureWidth,
				glyphInTextureHeight, a7, a8, a9 + 0, a10, a11,
				a12, inColor, opacity, a15, a16);
		}
	}

	unsigned int sg0DrawGlyph3Hook(int textureId, int maskTextureId, int textureStartX, int textureStartY, int textureSizeX, int textureSizeY, int startPosX, int startPosY, int EndPosX, int EndPosY, int color, int opacity) {
		return gameExeSg0DrawGlyph3Real(textureId, maskTextureId, textureStartX, textureStartY, textureSizeX, textureSizeY, startPosX, startPosY, EndPosX, EndPosY, color, opacity);
	}

	int setTipContentHook(char* sc3string) {
		tipContent = sc3string;

		return gameExeSetTipContentReal(sc3string);
	}
	void drawTipContentHook(int textureId, int maskId, int startX, int startY,
		int maskStartY, int maskHeight, int a7, int color,
		int shadowColor, int opacity) {
		ProcessedSc3String_t str;

		int dummy1;
		int dummy2;
		char name[256];

		for (int i = 0; i < 512; i++) {

			if (surfaceArray[i].texPtr[0] != nullptr) {
				sprintf(name, "Surface texture %i-0", i);
				surfaceArray[i].texPtr[0]->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
			}
		}
		if (!TextRendering::Instance().enabled) {
			gameExeDrawTipContentReal(textureId, maskId, startX, startY, maskStartY, maskHeight, a7, color, shadowColor, opacity);
			return;
		}
		std::list<StringWord_t> words;
		MultiplierData mData;
		mData.xOffset = 2.0f;
		mData.yOffset = 2.0f;
		semiTokeniseSc3String(tipContent, words, TIP_REIMPL_GLYPH_SIZE,
			TIP_REIMPL_LINE_LENGTH);
		processSc3TokenList(startX, startY, TIP_REIMPL_LINE_LENGTH, words, 255, color,
			TIP_REIMPL_GLYPH_SIZE, &str, false, COORDS_MULTIPLIER, -1,
			NOT_A_LINK, color, TIP_REIMPL_GLYPH_SIZE * 1.5f, &mData);
		maskHeight *= 1.5f;
		for (int i = 0; i < str.length; i++) {
			if (str.displayStartY[i] / COORDS_MULTIPLIER > maskStartY &&
				str.displayEndY[i] / COORDS_MULTIPLIER < (maskStartY + maskHeight) * 1.0f) {
				TextRendering::Instance().replaceFontSurface(TIP_REIMPL_GLYPH_SIZE);
				auto fontData = TextRendering::Instance().getFont(TIP_REIMPL_GLYPH_SIZE, false);
				auto glyphInfo = fontData->getGlyphInfo(str.glyph[i], FontType::Regular);

				gameExeSg0DrawGlyph2(
					textureId, maskId, str.textureStartX[i], str.textureStartY[i],
					str.textureWidth[i], str.textureHeight[i],
					((float)str.displayStartX[i] + (1.0f * COORDS_MULTIPLIER)),
					((float)str.displayStartY[i] + TIP_REIMPL_GLYPH_SIZE - glyphInfo->top + (1.0f * COORDS_MULTIPLIER)),
					((float)str.displayStartX[i] + (1.0f * COORDS_MULTIPLIER)),
					((float)str.displayStartY[i] + TIP_REIMPL_GLYPH_SIZE - glyphInfo->top +
						((1.0f + (float)a7) * COORDS_MULTIPLIER)),
					((float)str.displayEndX[i] + (1.0f * COORDS_MULTIPLIER)),
					((float)str.displayEndY[i] + TIP_REIMPL_GLYPH_SIZE - glyphInfo->top +
						((1.0f + (float)a7) * COORDS_MULTIPLIER)),
					shadowColor, opacity, &dummy1, &dummy2);

				gameExeSg0DrawGlyph2(
					textureId, maskId, str.textureStartX[i], str.textureStartY[i],
					str.textureWidth[i], str.textureHeight[i], str.displayStartX[i],
					str.displayStartY[i] + TIP_REIMPL_GLYPH_SIZE - glyphInfo->top, str.displayStartX[i],
					((float)str.displayStartY[i] + TIP_REIMPL_GLYPH_SIZE - glyphInfo->top + ((float)a7 * COORDS_MULTIPLIER)),
					str.displayEndX[i],
					((float)str.displayEndY[i] + TIP_REIMPL_GLYPH_SIZE - glyphInfo->top + ((float)a7 * COORDS_MULTIPLIER)),
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

		if (textureId==80 && (spriteY == 1640 || spriteY==1936)) {
			 gameExeDrawSpriteReal(textureId, spriteX, spriteY+4, spriteWidth,
				9, displayX, displayY, color, opacity,
				shaderId);
			return  gameExeDrawSpriteReal(textureId, spriteX, spriteY + 4, spriteWidth,
				 spriteHeight - 9, displayX, displayY+9, color, opacity,
				 shaderId);
		

		}
		return gameExeDrawSpriteReal(textureId, spriteX, spriteY , spriteWidth,
			spriteHeight,  displayX, displayY , color, opacity,
			shaderId);

	}
}  // namespace lb
