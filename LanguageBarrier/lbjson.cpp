#include "LanguageBarrier.h"
#include "lbjson.h"

namespace lb {
json json_merge(const json &a, const json &b) {
  json result = a.flatten();
  json tmp = b.flatten();

  for (json::iterator it = tmp.begin(); it != tmp.end(); ++it) {
    result[it.key()] = it.value();
  }

  return result.unflatten();
}
}