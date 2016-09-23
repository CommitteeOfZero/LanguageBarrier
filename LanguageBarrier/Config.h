#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <sstream>
#include <stdexcept>
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
  const std::string filename;
  Config(const char* defaultStr, const std::string& _filename)
      : filename(_filename) {
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
    j = tmp1;
    save();
  };

 public:
  static Config& config() {
    static Config s(defaultConfigJsonStr, "languageBarrier\\config.json");
    return s;
  };
  static Config& sigs() {
    static Config s(defaultSignaturesJsonStr,
                    "languagebarrier\\signatures.json");
    return s;
  }
  static Config& fmv() {
    static Config s(defaultFmvJsonStr, "languagebarrier\\fmv.json");
    return s;
  }
  static Config& fileredirection() {
    static Config s(defaultFileredirectionJsonStr,
                    "languagebarrier\\fileredirection.json");
    return s;
  }
  static Config& stringredirection() {
      static Config s(defaultStringredirectionJsonStr,
          "languagebarrier\\stringredirection.json");
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
