#include <string>
#include <vector>
#include <Windows.h>  // Required for WideCharToMultiByte
#include "Utils.h"

// This function safely converts a wide (UTF-16) string to a narrow (UTF-8)
// string.
std::string WideToUtf8(const std::wstring& wide_string) {
  if (wide_string.empty()) {
    return "";
  }
  // First, find the required buffer size.
  int size_needed =
      WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(),
                          NULL, 0, NULL, NULL);
  if (size_needed <= 0) {
    // Handle error if you want, for now, return empty
    return "";
  }
  // Allocate buffer and perform the conversion.
  std::string utf8_string(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(),
                      &utf8_string[0], size_needed, NULL, NULL);
  return utf8_string;
}

// Overload for C-style wide strings
std::string WideToUtf8(const wchar_t* wide_c_string) {
  if (wide_c_string == nullptr) {
    return "";
  }
  return WideToUtf8(std::wstring(wide_c_string));
}