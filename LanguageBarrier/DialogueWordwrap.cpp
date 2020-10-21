#include "LanguageBarrier.h"
#include "SigScan.h"
#include <set>
#include <cstdint>
#include "TextRendering.h"
#include "Config.h"
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

// Linebreaks are allowed *after* type1 punctuation
// space (63) space (0) 、 。 ． ， ？ ！ 〜 ” ー ） 〕 ］ ｝ 〉 》 」 』 】 ☆ ★ ♪ 々 ぁ ぃ ぅ ぇ ぉ っ
// ゃ ゅ ょ ァ ィ ゥ ェ ォ ッ ャ ュ ョ –
static std::set<uint16_t> type1_punctuation = {
    0x003F, 0x0000, 0x00BE, 0x00BF, 0x00C1, 0x00C0, 0x00C4, 0x00C5, 0x00E4,
    0x00CB, 0x00E5, 0x00CD, 0x00CF, 0x00D1, 0x00D3, 0x00D5, 0x00D7, 0x00D9,
    0x00DB, 0x00DD, 0x01A5, 0x01A6, 0x00E6, 0x0187, 0x00E8, 0x00E9, 0x00EA,
    0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F0, 0x00F2, 0x00F3, 0x00F4,
    0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x0113};

// Linebreaks are allowed *before* type2 punctuation
// “ （ 〔 ［ ｛ 〈 《 「
static std::set<uint16_t> type2_punctuation = {0x00CA, 0x00CC, 0x00CE, 0x00D0,
                                               0x00D2, 0x00D4, 0x00D6, 0x00D8,
                                               0x00DA, 0x00DC};

enum mask_bytes {
  // seems to be a bitmask, but only certain combinations are used
  other = 0x00,
  type1_punct = 0x01,
  type2_punct = 0x02,
  linebreak = 0x07,
  word_last_char = 0x09,
  word_first_char = 0x0A,
  letter = 0x0B,
  ruby_letter = 0x1B
};

constexpr uint16_t name_start = 0x8001;
constexpr uint16_t name_end = 0x8002;

typedef void(__cdecl *DlgWordwrapGenerateMaskProc)(int unk0);
static DlgWordwrapGenerateMaskProc gameExeDlgWordwrapGenerateMask =
    NULL;  // = (DlgWordwrapGenerateMaskProc)0x004459F0;
static DlgWordwrapGenerateMaskProc gameExeDlgWordwrapGenerateMaskReal = NULL;

static uint16_t *gameExeDlgWordwrapString = NULL;  // = (uint16_t*)0x16C4E40;
static int *gameExeDlgWordwrapLength = NULL;       // = (int*)0x16C4E38;
static uint8_t *gameExeDlgWordwrapMask = NULL;     // = (uint8_t*)0x16D4840;

namespace lb {
void next_word(int &pos);
bool is_type1_punct(uint16_t c);
bool is_type2_punct(uint16_t c);
bool is_letter(uint16_t c);
void dlgWordwrapGenerateMaskHook(int unk0);

void dialogueWordwrapInit() {
  gameExeDlgWordwrapString =
      (uint16_t *)sigScan("game", "useOfDlgWordwrapString");
  gameExeDlgWordwrapLength = (int *)sigScan("game", "useOfDlgWordwrapLength");
  gameExeDlgWordwrapMask = (uint8_t *)sigScan("game", "useOfDlgWordwrapMask");
  scanCreateEnableHook("game", "dlgWordwrapGenerateMask",
                       (uintptr_t *)&gameExeDlgWordwrapGenerateMask,
                       (LPVOID)dlgWordwrapGenerateMaskHook,
                       (LPVOID *)gameExeDlgWordwrapGenerateMaskReal);

  type1_punctuation.clear();
  auto input = config["patch"]["type1Punctuation"].get<std::string>();
  auto input2 = config["patch"]["type2Punctuation"].get<std::string>();

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::wstring type1_punct = converter.from_bytes(input);
  std::wstring type2_punct = converter.from_bytes(input2);

  for (auto character : type1_punct) {
      auto index=TextRendering::Get().fullCharMap.find(character);
      type1_punctuation.insert(index);
  }
  type1_punctuation.insert(0x3F);
  type2_punctuation.clear();
  for (auto character : type2_punct) {
	  auto index = TextRendering::Get().fullCharMap.find(character);
	  type2_punctuation.insert(index);
  }

}

void dlgWordwrapGenerateMaskHook(int unk0) {
  int pos = 0;
  bool insideRubyBase = false, insideRubyText = false;

  // If it's a piece of dialogue, we should deal with the name first.
  // s[n] == name_start -> mask[n] = type2_punct (L - Rojikku)
  // s[n] == name_end -> mask[n] = linebreak
  // s[n] == any_character -> mask[n] = letter
  if (gameExeDlgWordwrapString[0] == name_start) {
    gameExeDlgWordwrapMask[pos++] = mask_bytes::type2_punct;
    while (gameExeDlgWordwrapString[pos] != name_end) {
      gameExeDlgWordwrapMask[pos] = mask_bytes::letter;
      pos++;
    }

    if (unk0)
      gameExeDlgWordwrapMask[pos++] = mask_bytes::linebreak;
    else
      gameExeDlgWordwrapMask[pos++] = mask_bytes::type1_punct;
  }

  while (pos < *gameExeDlgWordwrapLength) {
    auto curr = gameExeDlgWordwrapString[pos];
    // s[n] is a control sequence -> mask[n] = other
    if (curr >= 0x8000) {
      uint8_t curMask = mask_bytes::other;
      switch (curr & 0xFF) {
        case 0:
          curMask = mask_bytes::linebreak;
          break;
        case 9:
          insideRubyBase = true;
          curMask = mask_bytes::type2_punct;
          break;
        case 10:
          insideRubyText = true;
          curMask = mask_bytes::letter;
          break;
        case 11:
          insideRubyBase = insideRubyText = false;
          curMask = mask_bytes::type1_punct;
          break;
        case 18:
          curMask = mask_bytes::type2_punct;
          break;
        case 30:
          curMask = mask_bytes::letter;
          break;
      }
      gameExeDlgWordwrapMask[pos] = curMask;
      pos++;
      continue;
    }

    if (insideRubyText) {
      gameExeDlgWordwrapMask[pos] = mask_bytes::ruby_letter;
      pos++;
    } else if (insideRubyBase) {
      // never break lines inside ruby text
      gameExeDlgWordwrapMask[pos] = mask_bytes::letter;
      pos++;
    } else if (is_type1_punct(curr)) {
      gameExeDlgWordwrapMask[pos] = mask_bytes::type1_punct;
      pos++;
    } else if (is_type2_punct(curr)) {
      gameExeDlgWordwrapMask[pos] = mask_bytes::type2_punct;
      pos++;
    } else {
      next_word(pos);
    }
  }
}

void next_word(int &pos) {
  int word_len = 0;
  while (pos < *gameExeDlgWordwrapLength &&
         is_letter(gameExeDlgWordwrapString[pos])) {
    if (word_len == 0)
      gameExeDlgWordwrapMask[pos] = mask_bytes::word_first_char;
    else
      gameExeDlgWordwrapMask[pos] = mask_bytes::letter;

    pos++;
    word_len++;
  }

  if (word_len == 1)
    // 1 letter word -> mask[n] = other
    gameExeDlgWordwrapMask[pos - 1] = mask_bytes::other;
  else
    // 2+ letters -> mark the last character of the word as word_last_char
    gameExeDlgWordwrapMask[pos - 1] = mask_bytes::word_last_char;
}

bool is_type1_punct(uint16_t c) {
  return type1_punctuation.find(c) != type1_punctuation.end();
}

bool is_type2_punct(uint16_t c) {
  return type2_punctuation.find(c) != type2_punctuation.end();
}

bool is_letter(uint16_t c) {
  return c < 0x8000 && !is_type1_punct(c) && !is_type2_punct(c);
}
}