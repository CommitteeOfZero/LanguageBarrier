#include "TextRendering.h"
#include <string>
#include <freetype/ftstroke.h>
#include "Game.h"
#include <d3d11.h>
#include <directxtex.h>
void TextRendering::disableReplacement()
{
	this->enabled = false;
	memcpy(widthData, originalWidth, NUM_GLYPHS);
	memcpy(widthData2, originalWidth2, NUM_GLYPHS);
}


void TextRendering::enableReplacement()
{
	this->enabled = true;
	this->getFont(32);
	for (int i = 0; i < NUM_GLYPHS; i++) {
		widthData[i] = (uint8_t)fontData[32].getGlyphInfo(i, FontType::Regular)->advance;
	}
}

TextRendering::TextRendering()
{
	FILE* f = fopen("charset.utf8", "rb");
	char charset[16000];
	fread(charset, 8895, 1, f);
	wchar_t charset16[8895];
	auto b = MultiByteToWideChar(CP_UTF8, 0, charset, -1, charset16, 8895);
	charMap = std::wstring(charset16);
}

void TextRendering::Init(void* widthData, void* widthData2)
{
	memcpy(originalWidth, widthData, NUM_GLYPHS);
	memcpy(originalWidth2, widthData2, NUM_GLYPHS);
	this->widthData = (uint8_t*)widthData;
	this->widthData2 = (uint8_t*)widthData2;
}

struct TextSize {

	int w, h, w2, h2;

};
void TextRendering::buildFont(int fontSize)
{
	const char* fontPath = "languagebarrier/noto.ttc";

	if (this->ftLibrary == nullptr) {

		if (FT_Init_FreeType(&this->ftLibrary))
		{
		}
	}
	if (this->ftFace == nullptr) {
		if (FT_New_Face(this->ftLibrary, fontPath, 0, &this->ftFace))
		{
		}
	}
	FT_Select_Charmap(this->ftFace, FT_Encoding::FT_ENCODING_UNICODE);
	FT_Set_Pixel_Sizes(this->ftFace, 0, fontSize);

	FT_Stroker_New(this->ftLibrary, &this->stroker);
	//  2 * 64 result in 2px outline
	FT_Stroker_Set(stroker, 2 * fontSize * FONT_CELL_SIZE / 48, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);


	this->fontData[fontSize] = FontData();
	auto  fontData = &this->fontData[fontSize];

	fontData->fontTexture.Initialize2D(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, FONT_CELL_SIZE * GLYPHS_PER_ROW, GLYPHS_PER_ROW * ceil((float)NUM_GLYPHS / GLYPHS_PER_ROW), 1, 1);
	fontData->outlineTexture.Initialize2D(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, FONT_CELL_SIZE * GLYPHS_PER_ROW, GLYPHS_PER_ROW * ceil((float)NUM_GLYPHS / GLYPHS_PER_ROW), 1, 1);
	int num = 1;
	int mipIndex = 0;
	/*while (fontSize / num > 1) {

		int cellSize = 64 / num;

		FT_Set_Pixel_Sizes(this->ftFace, 0, fontSize / num);
		if (this->fontData.find(fontSize / num) == this->fontData.end()) {
			this->fontData[fontSize / num] = FontData();
			fontData = &this->fontData[fontSize / num];

		}
		else {
			fontData = &this->fontData[fontSize / num];

		}
		const DirectX::Image* outlineMip = fontData->outlineTexture.GetImage(mipIndex, 0, 0);
		const DirectX::Image* fontMip = fontData->fontTexture.GetImage(mipIndex, 0, 0);

		uint32_t* rgbaPixels = (uint32_t*)outlineMip->pixels;
		memset(outlineMip->pixels, 0, outlineMip->rowPitch * outlineMip->height);
		memset(fontMip->pixels, 0, outlineMip->rowPitch * outlineMip->height);

		for (int i = 0; i < NUM_GLYPHS; i++) {
			RenderOutline(fontData, charMap[i]);
			const auto& glyph = fontData->glyphData.outlineMap[charMap[i]];
			auto glyphData = glyph.data;

			if (glyph.rows > cellSize || glyph.width > cellSize) {

			}
			int x = (i * cellSize) % (cellSize * 64) / cellSize;
			int y = i * cellSize / (cellSize * 64);


			for (int k = 0; k < 64 / num; k++) {
				for (int j = 0; j < 64 / num; j++) {
					rgbaPixels[64 * cellSize * (j + y * cellSize) + cellSize * x + k] = 0x0 | glyphData[j][k] << 24;

				}
			}


		}
		rgbaPixels = (uint32_t*)fontMip->pixels;

		for (int i = 0; i < NUM_GLYPHS; i++) {
			//RenderOutline(charMap[i]);
			renderGlyph(fontData, charMap[i]);
			const auto& glyph = fontData->glyphData.glyphMap[charMap[i]];
			auto glyphData = fontData->glyphData.glyphMap[charMap[i]].data;

			if (glyph.rows > cellSize || glyph.width > cellSize) {

			}
			int x = (i * cellSize) % (cellSize * 64) / cellSize;
			int y = i * cellSize / (cellSize * 64);

			for (int k = 0; k < 64 / num; k++) {
				for (int j = 0; j < 64 / num; j++) {
					rgbaPixels[64 * cellSize * (j + y * cellSize) + cellSize * x + k] = 0xFFFFFF | glyphData[j][k] << 24;
					//rgbaPixels2[3072 * (j + y * 64) + 64 * x + k] = 0x0 | glyphData2[j][k] << 24;

				}
			}


		}
		num *= 2;
		mipIndex++;

	}*/

	num = 1;
	mipIndex = 0;

	int cellSize = FONT_CELL_SIZE / num;

	FT_Set_Pixel_Sizes(this->ftFace, 0, fontSize / num);
	if (this->fontData.find(fontSize / num) == this->fontData.end()) {
		this->fontData[fontSize / num] = FontData();
		fontData = &this->fontData[fontSize / num];

	}
	else {
		fontData = &this->fontData[fontSize / num];

	}
	const DirectX::Image* outlineMip = fontData->outlineTexture.GetImage(mipIndex, 0, 0);
	const DirectX::Image* fontMip = fontData->fontTexture.GetImage(mipIndex, 0, 0);

	uint32_t* rgbaPixels = (uint32_t*)outlineMip->pixels;
	memset(outlineMip->pixels, 0, outlineMip->rowPitch * outlineMip->height);
	memset(fontMip->pixels, 0, outlineMip->rowPitch * outlineMip->height);

	for (int i = 0; i < NUM_GLYPHS; i++) {
		RenderOutline(fontData, charMap[i]);
		const auto& glyph = fontData->glyphData.outlineMap[charMap[i]];
		auto glyphData = glyph.data;

		if (glyph.rows > cellSize || glyph.width > cellSize) {

		}
		int x = (i * cellSize) % (cellSize * 64) / cellSize;
		int y = i * cellSize / (cellSize * 64);


		for (int k = 0; k < 64 / num; k++) {
			for (int j = 0; j < 64 / num; j++) {
				rgbaPixels[64 * cellSize * (j + y * cellSize) + cellSize * x + k] = 0x0 | glyphData[j][k] << 24;

			}
		}


	}
	rgbaPixels = (uint32_t*)fontMip->pixels;

	for (int i = 0; i < NUM_GLYPHS; i++) {
		renderGlyph(fontData, charMap[i]);
		const auto& glyph = fontData->getGlyphInfo(i, FontType::Regular);

		if (glyph->rows > cellSize || glyph->width > cellSize) {

		}
		int x = (i * cellSize) % (cellSize * FONT_CELL_SIZE) / cellSize;
		int y = i * cellSize / (cellSize * FONT_CELL_SIZE);

		for (int k = 0; k < FONT_CELL_SIZE / num; k++) {
			for (int j = 0; j < FONT_CELL_SIZE / num; j++) {
				rgbaPixels[FONT_CELL_SIZE * cellSize * (j + y * cellSize) + cellSize * x + k] = 0xFFFFFF | glyph->data[j][k] << 24;
			}
		}


	}


	const DirectX::Image* img = fontData->fontTexture.GetImages();
	const DirectX::Image* img2 = fontData->outlineTexture.GetImages();

	assert(img);
	size_t nimg = fontData->fontTexture.GetImageCount();
	assert(nimg > 0);

	FT_Stroker_Done(stroker);


	D3D11_SHADER_RESOURCE_VIEW_DESC rscViewDesc;



	memset(&rscViewDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	auto result = DirectX::CreateTexture(gameExePMgsD3D11State->pid3d11deviceC, this->fontData[fontSize].fontTexture.GetImages(),
		this->fontData[fontSize].fontTexture.GetImageCount(), this->fontData[fontSize].fontTexture.GetMetadata(), (ID3D11Resource**)&this->fontData[fontSize].fontTexturePtr);
	result = DirectX::CreateTexture(gameExePMgsD3D11State->pid3d11deviceC, this->fontData[fontSize].outlineTexture.GetImages(),
		this->fontData[fontSize].outlineTexture.GetImageCount(), this->fontData[fontSize].outlineTexture.GetMetadata(), (ID3D11Resource**)&this->fontData[fontSize].outlineTexturePtr);
	rscViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rscViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	rscViewDesc.Buffer.NumElements = 1;
	rscViewDesc.Texture2D.MipLevels = this->fontData[fontSize].fontTexture.GetMetadata().mipLevels;
	result = gameExePMgsD3D11State->pid3d11deviceC->CreateShaderResourceView(this->fontData[fontSize].fontTexturePtr, &rscViewDesc, &this->fontData[fontSize].fontShaderRscView);
	result = gameExePMgsD3D11State->pid3d11deviceC->CreateShaderResourceView(this->fontData[fontSize].outlineTexturePtr, &rscViewDesc, &this->fontData[fontSize].outlineShaderRscView);

	SaveToDDSMemory(fontData->fontTexture.GetImages(), fontData->fontTexture.GetImageCount(), fontData->fontTexture.GetMetadata(), DirectX::DDS_FLAGS_NONE, fontData->texture);
	SaveToDDSMemory(fontData->outlineTexture.GetImages(), fontData->outlineTexture.GetImageCount(), fontData->outlineTexture.GetMetadata(), DirectX::DDS_FLAGS_NONE, fontData->texture2);




	return;

}



void TextRendering::renderGlyph(FontData* fontData, uint16_t n)
{
	FT_Long glyph_index;
	FT_Glyph glyph;
	FT_Face face = this->ftFace;

	glyph_index = FT_Get_Char_Index(face, n);
	if (FT_Load_Glyph(face, glyph_index, 0))
	{
		return;

	}
	if (FT_Get_Glyph(face->glyph, &glyph))
	{
		return;

	}
	if (FT_Render_Glyph(face->glyph, FT_Render_Mode::FT_RENDER_MODE_NORMAL))
	{
		return;

	}
	if (FT_Glyph_To_Bitmap(&glyph, FT_Render_Mode::FT_RENDER_MODE_NORMAL, 0, 1))
	{
		return;
	}
	FT_Bitmap* bitmapGlyph = &((FT_BitmapGlyph)glyph)->bitmap;


	fontData->glyphData.glyphMap[n] = FontGlyph();
	memset(fontData->glyphData.glyphMap[n].data, 0, 4096);

	fontData->glyphData.glyphMap[n].advance = face->glyph->advance.x / 64;
	fontData->glyphData.glyphMap[n].rows = face->glyph->bitmap.rows;
	fontData->glyphData.glyphMap[n].width = face->glyph->bitmap.width;
	fontData->glyphData.glyphMap[n].left = face->glyph->bitmap_left;
	fontData->glyphData.glyphMap[n].top = face->glyph->bitmap_top;

	for (int i = 0; i < face->glyph->bitmap.width; i++) {
		for (int j = 0; j < face->glyph->bitmap.rows; j++) {
			fontData->glyphData.glyphMap[n].data[j][i] = (uint8_t)bitmapGlyph->buffer[i + bitmapGlyph->pitch * j];
		}
	}
	FT_Done_Glyph(glyph);

}

void TextRendering::RenderOutline(FontData* fontData, uint16_t n)
{

	FT_Face face = this->ftFace;


	// generation of an outline for single glyph:
	FT_UInt glyphIndex = FT_Get_Char_Index(face, n);
	FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
	FT_Glyph glyph;
	FT_Get_Glyph(face->glyph, &glyph);
	FT_Glyph_StrokeBorder(&glyph, stroker, false, true);
	FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, true);
	FT_BitmapGlyph bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
	fontData->glyphData.outlineMap[n].rows = bitmapGlyph->bitmap.rows;
	fontData->glyphData.outlineMap[n].width = bitmapGlyph->bitmap.width;
	fontData->glyphData.outlineMap[n].left = bitmapGlyph->left;
	fontData->glyphData.outlineMap[n].top = bitmapGlyph->top;
	uint8_t* glyphBuffer = (uint8_t*)fontData->glyphData.outlineMap[n].data;

	for (int i = 0; i < bitmapGlyph->bitmap.width; i++) {
		for (int j = 0; j < bitmapGlyph->bitmap.rows; j++) {
			fontData->glyphData.outlineMap[n].data[j][i] = (uint8_t)bitmapGlyph->bitmap.buffer[i + bitmapGlyph->bitmap.pitch * j];
		}
	}
	FT_Done_Glyph(glyph);
}
FontData* TextRendering::getFont(int height)
{
	if (fontData.find(height) == fontData.end()) {
		fontData[height] = FontData();
		this->buildFont(height);
	}
	return &fontData[height];
}

void TextRendering::replaceFontSurface(int size)
{

	auto currentFontData = this->getFont(size);

	if (surfaceArray[FONT_TEXTURE_ID].texPtr[0] == nullptr) {
		lb::gameLoadTexture(FONT_TEXTURE_ID, currentFontData->texture.GetBufferPointer(), currentFontData->texture.GetBufferSize());

	}
	if (surfaceArray[OUTLINE_TEXTURE_ID].texPtr[0] == nullptr) {
		lb::gameLoadTexture(OUTLINE_TEXTURE_ID, currentFontData->texture2.GetBufferPointer(), currentFontData->texture2.GetBufferSize());

	}
	surfaceArray[FONT_TEXTURE_ID].texPtr[0] = currentFontData->fontTexturePtr;
	surfaceArray[FONT_TEXTURE_ID].shaderRscView = currentFontData->fontShaderRscView;
	surfaceArray[OUTLINE_TEXTURE_ID].texPtr[0] = currentFontData->outlineTexturePtr;
	surfaceArray[OUTLINE_TEXTURE_ID].shaderRscView = currentFontData->outlineShaderRscView;

}



FontGlyph* FontData::getGlyphInfo(int id, FontType type)
{
	switch (type) {
	case Regular:
		return &this->glyphData.glyphMap[TextRendering::Instance().charMap[id]];
		break;
	case Outline:
		return &this->glyphData.outlineMap[TextRendering::Instance().charMap[id]];
		break;
	case Italics:
		return &this->glyphData.glyphMap[TextRendering::Instance().charMap[id]];
		break;
	default:
		break;

	}
}
