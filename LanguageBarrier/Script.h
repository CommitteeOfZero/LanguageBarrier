#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include <cstdint>
#include <string>
#include "LanguageBarrier.h"

#ifndef SCRIPT_H_IMPORT
#define SCRIPT_H_IMPORT extern
#endif

struct __declspec(align(4)) ScriptThreadState {
  /* 0000 */ int accumulator;
  /* 0004 */ char gap4[12];
  /* 0010 */ unsigned int execution_priority;
  /* 0014 */ unsigned int thread_group_id;
  /* 0018 */ unsigned int sleep_timeout;
  /* 001C */ char gap28[8];
  /* 0024 */ unsigned int loop_counter;
  /* 0028 */ unsigned int loop_target_label_id;
  /* 002C */ unsigned int call_stack_depth;
  /* 0030 */ unsigned int ret_address_ids[8];
  /* 0050 */ unsigned int ret_address_script_buffer_ids[8];
  /* 0070 */ int thread_id;
  /* 0074 */ int script_buffer_id;
  /* 0078 */ char gap78[8];
  /* 0080 */ unsigned int draw_priority;
  /* 0084 */ unsigned int draw_type;
  /* 0088 */ unsigned int alpha;
  /* 008C */ char gap120[48];
  /* 00BC */ int thread_local_variables[32];
  /* 013C */ int somePageNumber;
  /* 0140 */ ScriptThreadState *next_context;
  /* 0144 */ ScriptThreadState *prev_context;
  /* 0148 */ ScriptThreadState *next_free_context;
  /* 014C */ uint8_t *pc;
};

typedef int(__cdecl *Sc3EvalProc)(ScriptThreadState *sc3, int *pOutResult);
typedef void(__cdecl *ScriptInstProc)(ScriptThreadState *state);

namespace lb {
LB_GLOBAL bool CUSTOM_INST_GETDIC_ENABLED;
LB_GLOBAL std::string CUSTOM_INST_GETDIC_GROUP;
LB_GLOBAL uint8_t CUSTOM_INST_GETDIC_OP;

SCRIPT_H_IMPORT Sc3EvalProc gameExeSc3Eval;

SCRIPT_H_IMPORT int *gameExeSCRflag;

SCRIPT_H_IMPORT ScriptInstProc *InstTableSystem;
SCRIPT_H_IMPORT ScriptInstProc *InstTableGraph;
SCRIPT_H_IMPORT ScriptInstProc *InstTableUser1;

void scriptInit();
}  // namespace lb

#endif  // !__SCRIPT_H__
