#ifndef __SCRIPTDEBUGGER_H__
#define __SCRIPTDEBUGGER_H__

#include "LanguageBarrier.h"
#include "Script.h"

#include "better-enums/enum.h"

namespace lb {
enum ThreadStateFlag {  // Applies to both individual threads and thread groups
  TF_None = 0x0,
  TF_Destroy = 0x8000000,
  TF_Animate = 0x10000000,
  TF_Display = 0x20000000,
  TF_Pause = 0x40000000,
  TF_Message = 0x80000000,
};

BETTER_ENUM(DrawComponentType, uint8_t, Text = 0x0, Main = 0x1,
            ExtrasScenes = 0x2, Mask = 0x3, SystemText = 0x3, SaveMenu = 0x4,
            SaveMenu05 = 0x5, SystemIcons = 0x6, TitleMenu = 0x7, Option = 0x9,
            SystemMenu = 0xA, SystemMessage = 0xB, PlayData = 0xC, Album = 0xD,
            ExtrasMusicMode = 0xE, DictionaryMode = 0xF, ExtrasMovieMode = 0x10,
            ExtrasActorsVoice = 0x11, SaveIcon = 0x12,
            GlobalSystemMessage = 0x15, InstallInfo = 0x16,
            SystemMessage17 = 0x17, DebugEditor = 0x1E, None = 0xFF)

BETTER_ENUM(ThreadGroupType, unsigned char, Root = 0x0, System = 0x1,
            GameSys = 0x2, GameSys2 = 0x3, GameSys3 = 0x4, Script = 0x5,
            Script2 = 0x6, Script3 = 0x7, Script4 = 0x8, None = 0xFF)

void __cdecl sc3ExecHook(ScriptThreadState* thread);

void ShowDebugMenu();
void ShowDockableArea();
void ShowSingleWindow();
void ShowScriptVariablesEditor();
void ShowScriptDebugger();
}  // namespace lb

#endif  // !__SCRIPTDEBUGGER_H__