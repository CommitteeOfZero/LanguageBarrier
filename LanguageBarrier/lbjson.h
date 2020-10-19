#ifndef __LBJSON_H__
#define __LBJSON_H__

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace lb {
// https://github.com/nlohmann/json/issues/252
json json_merge(const json &a, const json &b);
}

#endif  // !__LBJSON_H__
