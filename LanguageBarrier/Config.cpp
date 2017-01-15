#include "LanguageBarrier.h"
#define DEFINE_CONFIG
#include "Config.h"
#include "lbjson.h"
#include <codecvt>
#include <fstream>
#include <sstream>
#include <Shlwapi.h>
#include <ShlObj.h>

json patchdef;
json rawConfig;

namespace lb {
void configLoadFiles();

void configInit() {
  configLoadFiles();

  config["patch"] = patchdef["base"];

  for (json::iterator it = patchdef["settings"].begin();
       it != patchdef["settings"].end(); it++) {
    const json& o = it.value();
    if (o["type"].get<std::string>() == "bool") {
      if (rawConfig.count(it.key()) == 1 &&
          rawConfig[it.key()].get<bool>() == true) {
        config["patch"] = json_merge(config["patch"], o["setters"]);
      }
    } else if (o["type"].get<std::string>() == "choice") {
      if (rawConfig.count(it.key()) == 1) {
        const std::string& choice = rawConfig[it.key()].get<std::string>();
        if (o["choices"].count(choice) == 1) {
          config["patch"] = json_merge(config["patch"], o["choices"][choice]);
        }
      }
    }
  }
}
const std::string& configGetGameName()
{
	return config["gamedef"]["gameName"].get<std::string>();
}
const std::string& configGetPatchName()
{
	return patchdef["patchName"].get<std::string>();
}
void configLoadFiles() {
  {
    std::ifstream i("languagebarrier\\gamedef.json");
    json j;
    i >> j;
    config["gamedef"] = j;
  }

  {
    std::ifstream i("languagebarrier\\patchdef.json");
    i >> patchdef;
  }

  {
    std::ifstream i("languagebarrier\\defaultconfig.json");
    json defaultconfig;
    i >> defaultconfig;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring appdatadir =
        converter.from_bytes(patchdef["appdatadir"].get<std::string>());
    wchar_t* appdata;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, NULL, NULL, &appdata);
    // apparently it doesn't like writing to the output directly
    std::wstringstream path;
    path << appdata;
    CoTaskMemFree(appdata);
    path << L"\\" << appdatadir;
    SHCreateDirectoryEx(NULL, path.str().c_str(), NULL);
    path << L"\\config.json";

    try {
      std::ifstream i2(path.str());
	  i2.exceptions(i2.exceptions() | std::ifstream::failbit);
      json j;
      j << i2;
      rawConfig = json_merge(defaultconfig, j);
    } catch (...) {
      rawConfig = defaultconfig;
    }

    std::ofstream o(path.str());
    rawConfig >> o;
  }
}
}