#include "TextRendering.h"
#include <string>
#include <freetype/ftstroke.h>
#include "Game.h"
#include <d3d11.h>
#include <directxtex.h>
#include <nlohmann/json.hpp>
#include <fstream>

void to_json(nlohmann::json& j, const FontGlyph& p) {
	j = nlohmann::json{ {"a", p.advance}, {"x", p.x}, {"y", p.y},
	{"t", p.top}, {"l", p.left}, {"r", p.rows},{"w", p.width} };
}

void from_json(const nlohmann::json& j, FontGlyph& p) {
	j.at("x").get_to(p.x);
	j.at("y").get_to(p.y);
	j.at("t").get_to(p.top);
	j.at("l").get_to(p.left);
	j.at("r").get_to(p.rows);
	j.at("w").get_to(p.width);
	j.at("a").get_to(p.advance);

}



void TextRendering::disableReplacement()
{
	this->enabled = false;

}


void TextRendering::enableReplacement()
{
	this->enabled = true;

}

TextRendering::TextRendering()
{
	FILE* f = fopen("charset.utf8", "rb");
	char charset[16000];
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	fread(charset, fsize, 1, f);
	charset[fsize] = 0;
	charset[fsize + 1] = 0;

	wchar_t charset16[16000];
	auto utf16size = MultiByteToWideChar(CP_UTF8, 0, charset, fsize, charset16, 0);
	MultiByteToWideChar(CP_UTF8, 0, charset, fsize, charset16, utf16size);
	charset16[utf16size] = 0;
	fullCharMap = std::wstring(charset16);

	currentCharMap = &fullCharMap;
	this->buildFont(32, true);

	filteredCharMap.reserve(utf16size);

	for (int i = 0; i < utf16size; i++) {
		if (!(charset16[i] >= 0x4E00 && charset16[i] <= 0x9faf) && filteredCharMap.find(charset16[i]) == currentCharMap->npos)
			filteredCharMap.push_back(charset16[i]);
	}
	currentCharMap = &filteredCharMap;

	this->NUM_GLYPHS = filteredCharMap.length();
}

void TextRendering::Init(void* widthData, void* widthData2)
{
	memcpy(originalWidth, widthData, 351);
	memcpy(originalWidth2, widthData2, 351);
	this->widthData = (uint8_t*)widthData;
	this->widthData2 = (uint8_t*)widthData2;
	this->getFont(32, true);
	for (int i = 0; i < 351; i++) {
		this->widthData[i] = (uint8_t)fontData[32].getGlyphInfo(i, FontType::Regular)->advance;
	}
	this->fontData.erase(32);
}

struct TextSize {

	int w, h, w2, h2;

};
void TextRendering::buildFont(int fontSize, bool measure)
{
	this->FONT_CELL_SIZE = fontSize * 1.33;

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
	NUM_GLYPHS = currentCharMap->length();
	if (!measure) {
		fontData->fontTexture.Initialize2D(DXGI_FORMAT::DXGI_FORMAT_A8_UNORM, FONT_CELL_SIZE * GLYPHS_PER_ROW, FONT_CELL_SIZE * ceil((float)NUM_GLYPHS / GLYPHS_PER_ROW), 1, 1);
		fontData->outlineTexture.Initialize2D(DXGI_FORMAT::DXGI_FORMAT_A8_UNORM, FONT_CELL_SIZE * GLYPHS_PER_ROW, FONT_CELL_SIZE * ceil((float)NUM_GLYPHS / GLYPHS_PER_ROW), 1, 1);
	}
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
	uint8_t* rgbaPixels = nullptr;
	DirectX::Image* outlineMip = nullptr;
	DirectX::Image* fontMip = nullptr;
	if (!measure) {
		outlineMip = (DirectX::Image*) fontData->outlineTexture.GetImage(mipIndex, 0, 0);
		fontMip = (DirectX::Image*)fontData->fontTexture.GetImage(mipIndex, 0, 0);

		rgbaPixels = (uint8_t*)outlineMip->pixels;
		memset(outlineMip->pixels, 0, outlineMip->rowPitch * outlineMip->height);
		memset(fontMip->pixels, 0, outlineMip->rowPitch * outlineMip->height);
	}
	int characterCount = 0;
	for (int i = 0; i < currentCharMap->length(); i++) {


		const auto& glyph = fontData->getGlyphInfoByChar(currentCharMap->at(i), FontType::Outline);
		if (glyph->x == -1 && glyph->y == -1) {
			RenderOutline(fontData, currentCharMap->at(i), measure);

			auto glyphData = glyph->data;

			if (glyph->rows > cellSize || glyph->width > cellSize) {

			}
			int x = (characterCount * cellSize) % (cellSize * GLYPHS_PER_ROW) / cellSize;
			int y = characterCount * cellSize / (cellSize * GLYPHS_PER_ROW);

			if (!measure && glyph->x == -1 && glyph->y == -1) {
				glyph->x = x * FONT_CELL_SIZE;
				glyph->y = y * FONT_CELL_SIZE;
				for (int j = 0; j < FONT_CELL_SIZE / num; j++) {
					memcpy(&rgbaPixels[GLYPHS_PER_ROW * cellSize * (j + y * cellSize) + cellSize * x], &glyphData[j * FONT_CELL_SIZE], glyph->width);


				}
				characterCount++;
				if (glyph->data != nullptr)
					delete  glyph->data;
				glyph->data = nullptr;
			}
		}

	}
	if (!measure) {
		rgbaPixels = (uint8_t*)fontMip->pixels;
	}
	characterCount = 0;
	for (int i = 0; i < currentCharMap->length(); i++) {
		const auto& glyph = fontData->getGlyphInfoByChar(currentCharMap->at(i), FontType::Regular);
		if (glyph->x == -1 && glyph->y == -1) {

			renderGlyph(fontData, currentCharMap->at(i), measure);

			if (glyph->rows > cellSize || glyph->width > cellSize) {

			}
			int x = (characterCount * cellSize) % (cellSize * GLYPHS_PER_ROW) / cellSize;
			int y = characterCount * cellSize / (cellSize * GLYPHS_PER_ROW);
			if (!measure && glyph->x == -1 && glyph->y == -1) {
				glyph->x = x * FONT_CELL_SIZE;
				glyph->y = y * FONT_CELL_SIZE;
				for (int j = 0; j < FONT_CELL_SIZE / num; j++) {
					memcpy(&rgbaPixels[GLYPHS_PER_ROW * cellSize * (j + y * cellSize) + cellSize * x], &glyph->data[j * FONT_CELL_SIZE], glyph->width);
				}
				characterCount++;
				delete  glyph->data;
				glyph->data = nullptr;

			}
		}

	}
	FT_Stroker_Done(stroker);

	if (!measure) {
		const DirectX::Image* img = fontData->fontTexture.GetImages();
		const DirectX::Image* img2 = fontData->outlineTexture.GetImages();

		assert(img);
		size_t nimg = fontData->fontTexture.GetImageCount();
		assert(nimg > 0);



		loadTexture(fontSize);

		SaveToDDSMemory(fontData->fontTexture.GetImages(), fontData->fontTexture.GetImageCount(), fontData->fontTexture.GetMetadata(), DirectX::DDS_FLAGS_NONE, fontData->texture);
		SaveToDDSMemory(fontData->outlineTexture.GetImages(), fontData->outlineTexture.GetImageCount(), fontData->outlineTexture.GetMetadata(), DirectX::DDS_FLAGS_NONE, fontData->texture2);
		wchar_t fileName[260];
		wsprintf(fileName, L"LanguageBarrier/font_%02d.dds", fontSize);
		SaveToDDSFile(fontData->fontTexture.GetImages(), fontData->fontTexture.GetImageCount(), fontData->fontTexture.GetMetadata(), DirectX::DDS_FLAGS_NONE, fileName);
		wsprintf(fileName, L"LanguageBarrier/outline_%02d.dds", fontSize);
		SaveToDDSFile(fontData->outlineTexture.GetImages(), fontData->outlineTexture.GetImageCount(), fontData->outlineTexture.GetMetadata(), DirectX::DDS_FLAGS_NONE, fileName);


		if (surfaceArray[FONT_TEXTURE_ID].texPtr[0] == nullptr) {
			lb::gameLoadTexture(FONT_TEXTURE_ID, fontData->texture.GetBufferPointer(), fontData->texture.GetBufferSize());
		}
		if (surfaceArray[OUTLINE_TEXTURE_ID].texPtr[0] == nullptr) {
			lb::gameLoadTexture(OUTLINE_TEXTURE_ID, fontData->texture2.GetBufferPointer(), fontData->texture2.GetBufferSize());
		}
		fontData->fontTexture.Release();
		fontData->outlineTexture.Release();

		fontData->texture.Release();
		fontData->texture2.Release();
	}
	return;

}

void TextRendering::loadTexture(int fontSize)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC rscViewDesc;



	memset(&rscViewDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	auto result = DirectX::CreateTexture(gameExePMgsD3D11State->pid3d11deviceC, this->fontData[fontSize].fontTexture.GetImages(),
		this->fontData[fontSize].fontTexture.GetImageCount(), this->fontData[fontSize].fontTexture.GetMetadata(), (ID3D11Resource**)&this->fontData[fontSize].fontTexturePtr);
	result = DirectX::CreateTexture(gameExePMgsD3D11State->pid3d11deviceC, this->fontData[fontSize].outlineTexture.GetImages(),
		this->fontData[fontSize].outlineTexture.GetImageCount(), this->fontData[fontSize].outlineTexture.GetMetadata(), (ID3D11Resource**)&this->fontData[fontSize].outlineTexturePtr);
	rscViewDesc.Format = DXGI_FORMAT_A8_UNORM;
	rscViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	rscViewDesc.Buffer.NumElements = 1;
	rscViewDesc.Texture2D.MipLevels = this->fontData[fontSize].fontTexture.GetMetadata().mipLevels;
	result = gameExePMgsD3D11State->pid3d11deviceC->CreateShaderResourceView(this->fontData[fontSize].fontTexturePtr, &rscViewDesc, &this->fontData[fontSize].fontShaderRscView);
	result = gameExePMgsD3D11State->pid3d11deviceC->CreateShaderResourceView(this->fontData[fontSize].outlineTexturePtr, &rscViewDesc, &this->fontData[fontSize].outlineShaderRscView);
}

void TextRendering::saveCache()
{
	std::ofstream os("LanguageBarrier/fontData.bin", std::ios::binary);
	cereal::BinaryOutputArchive archive(os);

	for (auto it = this->fontData.begin(); it != this->fontData.end();) {

		if (it->second.fontTexturePtr == nullptr) {
			it = this->fontData.erase(it);
		}
		else ++it;

	}

	archive(this->fontData);

	return;
}

void TextRendering::loadCache()
{

	std::ifstream os("LanguageBarrier/fontData.bin", std::ios::binary);
	if (!os.fail()) {
		cereal::BinaryInputArchive archive(os);
		archive(this->fontData);
		for (auto it = fontData.begin(); it != fontData.end(); it++) {

			wchar_t fileName[260];
			int size = it->first;
			wsprintf(fileName, L"languagebarrier/font_%02d.dds", it->first);
			auto g = DirectX::LoadFromDDSFile(fileName, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, nullptr, this->fontData[size].fontTexture);
			if (g != S_OK) {
				this->fontData.clear();
				return;
			}
			wsprintf(fileName, L"languagebarrier/outline_%02d.dds", it->first);
			g = DirectX::LoadFromDDSFile(fileName, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, nullptr, this->fontData[size].outlineTexture);
			if (g != S_OK) {
				this->fontData.clear();

				return;
			}
			const auto fontData = &this->fontData[size];
			loadTexture(size);
			DirectX::ScratchImage dummy;
			if (surfaceArray[FONT_TEXTURE_ID].texPtr[0] == nullptr) {
				dummy.Initialize2D(DXGI_FORMAT::DXGI_FORMAT_A8_UNORM, 1, 1, 1, 1);

				SaveToDDSMemory(dummy.GetImages(), dummy.GetImageCount(), dummy.GetMetadata(), DirectX::DDS_FLAGS_NONE, fontData->texture);


				lb::gameLoadTexture(FONT_TEXTURE_ID, fontData->texture.GetBufferPointer(), fontData->texture.GetBufferSize());
			}
			if (surfaceArray[OUTLINE_TEXTURE_ID].texPtr[0] == nullptr) {
				dummy.Initialize2D(DXGI_FORMAT::DXGI_FORMAT_A8_UNORM, 1, 1, 1, 1);
				SaveToDDSMemory(dummy.GetImages(), dummy.GetImageCount(), dummy.GetMetadata(), DirectX::DDS_FLAGS_NONE, fontData->texture2);
				lb::gameLoadTexture(OUTLINE_TEXTURE_ID, fontData->texture2.GetBufferPointer(), fontData->texture2.GetBufferSize());
			}
			fontData->fontTexture.Release();
			fontData->outlineTexture.Release();

		}
	}
	return;

}



void TextRendering::renderGlyph(FontData* fontData, uint16_t n, bool measure)
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
	if (!measure) {
		if (FT_Render_Glyph(face->glyph, FT_Render_Mode::FT_RENDER_MODE_NORMAL))
		{
			return;

		}

		if (FT_Glyph_To_Bitmap(&glyph, FT_Render_Mode::FT_RENDER_MODE_NORMAL, 0, 1))
		{
			return;
		}
	}
	FT_Bitmap* bitmapGlyph = &((FT_BitmapGlyph)glyph)->bitmap;


	fontData->glyphData.glyphMap[n] = FontGlyph();
	const auto& glyphData = &fontData->glyphData.glyphMap[n];

	glyphData->advance = face->glyph->advance.x / 64;
	glyphData->rows = face->glyph->bitmap.rows;
	glyphData->width = face->glyph->bitmap.width;
	glyphData->left = face->glyph->bitmap_left;
	glyphData->top = face->glyph->bitmap_top;
	if (!measure) {
		glyphData->data = new uint8_t[FONT_CELL_SIZE * FONT_CELL_SIZE];
		memset(glyphData->data, 0, FONT_CELL_SIZE * FONT_CELL_SIZE);

		for (int j = 0; j < face->glyph->bitmap.rows; j++) {
			memcpy(&glyphData->data[j * FONT_CELL_SIZE], &bitmapGlyph->buffer[bitmapGlyph->pitch * j], bitmapGlyph->pitch);
		}

	}
	FT_Done_Glyph(glyph);

}

void TextRendering::RenderOutline(FontData* fontData, uint16_t n, bool measure)
{

	FT_Face face = this->ftFace;


	// generation of an outline for single glyph:
	FT_UInt glyphIndex = FT_Get_Char_Index(face, n);
	FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
	FT_Glyph glyph;
	auto b = FT_Get_Glyph(face->glyph, &glyph);
	if (!measure)
		FT_Glyph_StrokeBorder(&glyph, stroker, false, true);
	{
		FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, true);
	}
	FT_BitmapGlyph bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);

	const auto& glyphData = &fontData->glyphData.outlineMap[n];
	glyphData->rows = bitmapGlyph->bitmap.rows;
	glyphData->width = bitmapGlyph->bitmap.width;
	glyphData->left = bitmapGlyph->left;
	glyphData->top = bitmapGlyph->top;
	uint8_t* glyphBuffer = (uint8_t*)fontData->glyphData.outlineMap[n].data;
	if (!measure) {
		glyphData->data = new uint8_t[FONT_CELL_SIZE * FONT_CELL_SIZE];
		if (glyphData->data == nullptr) {
			return;
		}
		memset(glyphData->data, 0, FONT_CELL_SIZE * FONT_CELL_SIZE);

		for (int j = 0; j < bitmapGlyph->bitmap.rows; j++) {
			memcpy(&glyphData->data[j * FONT_CELL_SIZE], &bitmapGlyph->bitmap.buffer[bitmapGlyph->bitmap.pitch * j], bitmapGlyph->bitmap.pitch);
		}

	}
	FT_Done_Glyph(glyph);
}
FontData* TextRendering::getFont(int height, bool measure)
{

	this->FONT_CELL_SIZE = height * 1.33;
	if (fontData.find(height) == fontData.end()) {
		fontData[height] = FontData();
		this->buildFont(height, measure);
	}
	if (fontData[height].fontTexturePtr == nullptr && !measure) {
		this->buildFont(height, measure);
	}
	return &fontData[height];
}

void TextRendering::replaceFontSurface(int size)
{

	auto currentFontData = this->getFont(size, false);

	if (surfaceArray[FONT_TEXTURE_ID].texPtr[0] == nullptr && currentFontData->texture.GetBufferPointer() != nullptr) {
		lb::gameLoadTexture(FONT_TEXTURE_ID, currentFontData->texture.GetBufferPointer(), currentFontData->texture.GetBufferSize());
		currentFontData->texture.Release();
	}
	if (surfaceArray[OUTLINE_TEXTURE_ID].texPtr[0] == nullptr && currentFontData->texture2.GetBufferPointer() != nullptr) {
		lb::gameLoadTexture(OUTLINE_TEXTURE_ID, currentFontData->texture2.GetBufferPointer(), currentFontData->texture2.GetBufferSize());
		currentFontData->texture.Release();
	}

	surfaceArray[FONT_TEXTURE_ID].width = FONT_CELL_SIZE * GLYPHS_PER_ROW;
	surfaceArray[FONT_TEXTURE_ID].height = FONT_CELL_SIZE * ceil((float)NUM_GLYPHS / GLYPHS_PER_ROW);
	surfaceArray[FONT_TEXTURE_ID].field_40 = FONT_CELL_SIZE * GLYPHS_PER_ROW;
	surfaceArray[FONT_TEXTURE_ID].field_44 = FONT_CELL_SIZE * ceil((float)NUM_GLYPHS / GLYPHS_PER_ROW);
	surfaceArray[OUTLINE_TEXTURE_ID].field_40 = FONT_CELL_SIZE * GLYPHS_PER_ROW;
	surfaceArray[OUTLINE_TEXTURE_ID].field_44 = FONT_CELL_SIZE * ceil((float)NUM_GLYPHS / GLYPHS_PER_ROW);
	surfaceArray[OUTLINE_TEXTURE_ID].width = FONT_CELL_SIZE * GLYPHS_PER_ROW;
	surfaceArray[OUTLINE_TEXTURE_ID].height = FONT_CELL_SIZE * ceil((float)NUM_GLYPHS / GLYPHS_PER_ROW);
	surfaceArray[FONT_TEXTURE_ID].texPtr[0] = currentFontData->fontTexturePtr;
	surfaceArray[FONT_TEXTURE_ID].shaderRscView = currentFontData->fontShaderRscView;
	surfaceArray[OUTLINE_TEXTURE_ID].texPtr[0] = currentFontData->outlineTexturePtr;
	surfaceArray[OUTLINE_TEXTURE_ID].shaderRscView = currentFontData->outlineShaderRscView;

}

FontGlyph* FontData::getGlyphInfoByChar(wchar_t character, FontType type)
{
	switch (type) {
	case Regular:
		return &this->glyphData.glyphMap[character];
		break;
	case Outline:
		return &this->glyphData.outlineMap[character];
		break;
	case Italics:
		return &this->glyphData.glyphMap[character];
		break;
	default:
		break;

	}
}


FontGlyph* FontData::getGlyphInfo(int id, FontType type)
{

	switch (type) {
	case Regular:
		return &this->glyphData.glyphMap[TextRendering::Get().fullCharMap[id]];
		break;
	case Outline:
		return &this->glyphData.outlineMap[TextRendering::Get().fullCharMap[id]];
		break;
	case Italics:
		return &this->glyphData.glyphMap[TextRendering::Get().fullCharMap[id]];
		break;
	default:
		break;

	}
}
