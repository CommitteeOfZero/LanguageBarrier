#define SCRIPT_H_IMPORT
#include "Script.h"
#include "Game.h"
#include "SigScan.h"

typedef void(__cdecl *ScriptInstProc)(ScriptThreadState *state);

static ScriptInstProc *InstTableSystem = NULL;
static ScriptInstProc *InstTableGraph = NULL;
static ScriptInstProc *InstTableUser1 = NULL;

// Custom instruction stuff

static inline void ConsumeOpcode(ScriptThreadState *state) {
  state->pc = (char *)state->pc + 2;
}
static inline uint8_t ReadScriptArgUint8(ScriptThreadState *state) {
  uint8_t result = *(uint8_t *)state->pc;
  state->pc = (char *)state->pc + 1;
  return result;
}
static inline int ReadScriptArgExpr(ScriptThreadState *state) {
  int result;
  lb::gameExeSc3Eval(state, &result);
  return result;
}

namespace lb {
void overrideScriptInst(const std::string &group, uint8_t op,
                        ScriptInstProc overrideProc);
void __cdecl customScriptInstGetDic(ScriptThreadState *state);

void scriptInit() {
  gameExeSc3Eval = (Sc3EvalProc)sigScan("game", "sc3Eval");
  InstTableSystem = (ScriptInstProc *)sigScan("game", "instTableSystem");
  InstTableGraph = (ScriptInstProc *)sigScan("game", "instTableGraph");
  InstTableUser1 = (ScriptInstProc *)sigScan("game", "instTableUser1");

  if (CUSTOM_INST_GETDIC_ENABLED) {
    overrideScriptInst(CUSTOM_INST_GETDIC_GROUP, CUSTOM_INST_GETDIC_OP,
                       &customScriptInstGetDic);
  }
}

void overrideScriptInst(const std::string &group, uint8_t op,
                        ScriptInstProc overrideProc) {
  std::stringstream logstr;
  logstr << "Overriding instruction " << group << "/" << op;
  LanguageBarrierLog(logstr.str());

  if (group == "system") {
    write_perms(&InstTableSystem[op], overrideProc);
  } else if (group == "graph") {
    write_perms(&InstTableGraph[op], overrideProc);
  } else if (group == "user1") {
    write_perms(&InstTableUser1[op], overrideProc);
  } else {
    LanguageBarrierLog("Invalid group!");
  }
}

void __cdecl customScriptInstGetDic(ScriptThreadState *state) {
  // Custom_GetDic(uint8 unused, expr tipId, flagRefExpr outFlag)
  // outFlag = 1 when tip locked, outFlag = 0 when tip already unlocked
  //
  // unused is there because we already had script code hoping this
  // function existed as a mode of SetDic /shrug

  ConsumeOpcode(state);
  ReadScriptArgUint8(state);
  int tipId = ReadScriptArgExpr(state);
  int outFlag = ReadScriptArgExpr(state);
  gameExeSetFlag(outFlag, !gameExeChkViewDic(tipId, 0));
}

}  // namespace lb