#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <Shlwapi.h>
#include <ShlObj.h>
#include "data\defaultConfigJsonStr.h"
#include "data\defaultSignaturesJsonStr.h"
#include "data\defaultFmvJsonStr.h"
#include "data\defaultFileredirectionJsonStr.h"
#include "data\defaultStringredirectionJsonStr.h"
#include "lbjson.h"

namespace lb {
class Config {
  Config() = delete;
  Config(const Config&) = delete;
  Config& operator=(const Config&) = delete;

 private:
  const std::wstring filename;
  Config(const char* defaultStr, const std::wstring& _filename)
      : filename(_filename) {
    load(defaultStr);
  }
  Config(const char* defaultStr, const std::wstring& pathEnd,
         REFKNOWNFOLDERID rfid)
      : filename(getPath(pathEnd, rfid)) {
    wchar_t dir[MAX_PATH];
    wcsncpy_s(dir, &filename[0], MAX_PATH);
    PathRemoveFileSpec(&dir[0]);
    SHCreateDirectoryEx(NULL, dir, NULL);
    load(defaultStr);
  }

  const std::wstring getPath(const std::wstring& pathEnd,
                             REFKNOWNFOLDERID rfid) {
    wchar_t* appdata;
    SHGetKnownFolderPath(rfid, NULL, NULL, &appdata);
    // apparently it doesn't like writing to the output directly
    std::wstringstream result;
    result << appdata;
    CoTaskMemFree(appdata);
    result << L"\\" << pathEnd;
    return result.str();
  }

  void load(const char* defaultStr) {
    std::stringstream ss;
    ss << defaultStr;
    json tmp1;
    tmp1 << ss;
    json tmp2;
    try {
      std::ifstream infile(filename);
      tmp2 << infile;
    } catch (...) {
    }
    j = json_merge(tmp1, tmp2);
    save();
  }

 public:
  static Config& config() {
    static Config s(defaultConfigJsonStr, L"Committee of Zero\\SGHD\\config.json",
                    FOLDERID_LocalAppData);
    return s;
  }
  static Config& sigs() {
    static Config s(defaultSignaturesJsonStr,
                    L"languagebarrier\\signatures.json");
    return s;
  }
  static Config& fmv() {
    static Config s(defaultFmvJsonStr, L"languagebarrier\\fmv.json");
    return s;
  }
  static Config& fileredirection() {
    static Config s(defaultFileredirectionJsonStr,
                    L"languagebarrier\\fileredirection.json");
    return s;
  }
  static Config& stringredirection() {
    static Config s(defaultStringredirectionJsonStr,
                    L"languagebarrier\\stringredirection.json");
    return s;
  }

  json j;

  void save() {
    std::ofstream outfile(filename);
    outfile << j;
  };
  static void init() {
    config();
    sigs();
    fmv();
    fileredirection();
    stringredirection();
  };
};
}

#endif  // !__CONFIG_H__
