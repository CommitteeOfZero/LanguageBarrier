#ifndef __SIGEXPR_H__
#define __SIGEXPR_H__

#include <cstdint>
#include <map>
#include <string>

namespace lb {
enum SigExprTokenType {
  Start,
  Plus,
  Minus,
  Deref,
  LParen,
  RParen,
  Number,
  Ptr,
  EOL
};

struct SigExprToken_t {
  SigExprTokenType type;
  uintptr_t value;
};

const static std::map<SigExprTokenType, std::string> SigExprTokenTypeString = {
    {Start, "Start"},   {Plus, "Plus"},     {Minus, "Minus"},
    {Deref, "Deref"},   {LParen, "LParen"}, {RParen, "RParen"},
    {Number, "Number"}, {Ptr, "Ptr"},       {EOL, "EOL"}};

class SigExprLexer {
 public:
  SigExprLexer(const std::string& _input) : input(_input) {
    currentToken = {Start, 0};
  }

  SigExprToken_t getToken();
  void consumeToken();

 private:
  const std::string& input;
  size_t pos = 0;
  SigExprToken_t currentToken;
};

class SigExpr {
 public:
  SigExpr(const std::string& _input, uintptr_t _ptr)
      : input(_input), lexer(_input), ptr(_ptr) {}

  uintptr_t evaluate();

 private:
  SigExprLexer lexer;
  const std::string& input;
  const uintptr_t ptr;

  uintptr_t expression();
  uintptr_t summand(bool onlyDereferable);
};
}

#endif  // !__SIGEXPR_H__