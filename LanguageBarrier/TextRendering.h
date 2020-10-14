#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype\ftglyph.h>
#include FT_STROKER_H

#include <map>
#include <DirectXTex.h>
#include <string>
#pragma comment(lib,"freetype.lib")
struct GlyphInfo
{
	__int8 bmp_top;
	__int8 bmp_left;
	__int8 rows;
	__int8 width;
};
struct fontOut
{
	void* bmpPtr;
	GlyphInfo* infoPtr;
};


struct FontGlyph {
	uint8_t * data=nullptr;
	float x=-1, y=-1;
	uint16_t advance;
	int32_t top;
	int32_t left;
	uint16_t rows;
	uint16_t width;
};

struct DDS_PIXELFORMAT {
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwFourCC;
	uint32_t dwRGBBitCount;
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;
};

typedef struct {
	uint32_t           dwSize;
	uint32_t           dwFlags;
	uint32_t           dwHeight;
	uint32_t           dwWidth;
	uint32_t           dwPitchOrLinearSize;
	uint32_t           dwDepth;
	uint32_t           dwMipMapCount;
	uint32_t           dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	uint32_t           dwCaps;
	uint32_t           dwCaps2;
	uint32_t           dwCaps3;
	uint32_t           dwCaps4;
	uint32_t           dwReserved2;
} DDS_HEADER;

struct GlyphMap {




	std::map<wchar_t, FontGlyph> glyphMap;
	std::map<wchar_t, FontGlyph> outlineMap;

};

struct  MesModeStruct
{
	__int16 field_0;
	__int16 field_2;
	__int16 field_4;
	__int16 field_6;
	__int16 field_8;
	__int16 field_A;
	__int16 field_C;
	__int16 field_E;
	__int16 field_10;
	__int16 field_12;
	__int16 field_14;
	__int16 field_16;
	__int16 field_18;
	__int16 field_1A;
	__int16 field_1C;
	__int16 field_1E;
	__int16 field_20;
	__int16 field_22;
	__int16 field_24;
	__int16 field_26;
	__int16 field_28;
	__int16 field_2A;
	__int16 field_2C;
	__int16 field_2E;
};

enum FontType {
	Regular,
	Outline,
	Italics
};


struct FontData {
	int size;
	DirectX::Blob texture;
	DirectX::Blob texture2;
	DirectX::ScratchImage fontTexture;
	DirectX::ScratchImage outlineTexture;
	GlyphMap glyphData;
	ID3D11Texture2D* fontTexturePtr;
	ID3D11ShaderResourceView* fontShaderRscView;
	ID3D11Texture2D* outlineTexturePtr;
	ID3D11ShaderResourceView* outlineShaderRscView;

	FontGlyph* getGlyphInfo(int id, FontType type);
	FontGlyph* getGlyphInfoByChar(wchar_t character, FontType type);

};

struct TextRendering
{
	FT_Library ftLibrary;
	FT_Face ftFace;
	FT_Stroker stroker;
    int FONT_CELL_SIZE = 66;
	const int GLYPHS_PER_ROW = 64;
	int NUM_GLYPHS = 351;
	char* fontPath = "languagebarrier/noto.ttc";

	bool enabled = true;

	void disableReplacement();
	void enableReplacement();
	GlyphInfo charInfo[0x40000];
	TextRendering();

	uint8_t originalWidth[32000];
	uint8_t originalWidth2[32000];
	uint8_t* widthData;
	uint8_t* widthData2;
	void Init(void* widthData, void* widthData2);
	void buildFont(int fontSize, bool measure);


	std::map<uint16_t, FontData> fontData;
	std::wstring filteredCharMap;

	std::wstring charMap;
	inline static TextRendering& Get()
	{
		static TextRendering instance;
		return instance;
	}
	void renderGlyph(FontData* fontData, uint16_t n, bool measure);

	void RenderOutline(FontData* fontData, uint16_t n, bool measure);

	FontData* getFont(int height, bool measure);

	void replaceFontSurface(int size);
	int SurfacePointSize[512];

	static const uint16_t FONT_TEXTURE_ID = 400;
	static const uint16_t OUTLINE_TEXTURE_ID = 401;

};