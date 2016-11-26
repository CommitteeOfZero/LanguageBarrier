#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <locale>
#include <codecvt>
#include <Shlwapi.h>
#include <ShlObj.h>
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
    json fileJson;
    try {
      std::ifstream infile(filename);
      fileJson << infile;
    } catch (...) {
    }
    if (defaultStr != NULL) {
      std::stringstream ss;
      ss << defaultStr;
      json defaultJson;
      defaultJson << ss;
      j = json_merge(defaultJson, fileJson);
    } else {
      j = fileJson;
    }
    save();
  }

 public:
  static Config& config() {
    std::string defaultConfig;
    std::ifstream in("languagebarrier\\defaultconfig.json",
                     std::ios::in | std::ios::binary);
    in.seekg(0, std::ios::end);
    defaultConfig.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&defaultConfig[0], defaultConfig.size());
    in.close();

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring appdatadir = converter.from_bytes(patchdef().j["appdatadir"].get<std::string>());

	std::wstringstream wss;
	wss << appdatadir << L"\\config.json";

    static Config s(defaultConfig.c_str(),
                    wss.str(),
                    FOLDERID_LocalAppData);
    return s;
  }

  static Config& patchdef() {
	  static Config s(NULL, L"languagebarrier\\patchdef.json");
	  return s;
  }
  static Config& sigs() {
    static Config s(NULL, L"languagebarrier\\signatures.json");
    return s;
  }

  json j;

  void save() {
    std::ofstream outfile(filename);
    outfile << j;
  };
  static void init() {
	patchdef();
    config();
    sigs();
  };
};
}

#endif  // !__CONFIG_H__
