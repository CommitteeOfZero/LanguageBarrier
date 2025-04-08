#include "ScriptDebugger.h"
#include "ScriptDebuggerHooks.h"
#include "Game.h"
#include "Config.h"
#include "Script.h"

#include <unordered_map>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include "imgui.h"

namespace lb {
int const VmMaxThreads = 100;
int const VmMaxThreadVars = 32;

static int ScrWorkNumberFormat = 0;
static int ScrWorkIndexStart = 0;
static int ScrWorkIndexEnd = 0;
static int FlagWorkIndexStart = 0;
static int FlagWorkIndexEnd = 0;

static std::unordered_map<uint32_t, std::map<int, int>> ScriptDebugByteCodePosToLine;
static std::unordered_map<uint32_t, std::map<int, int>>
    ScriptDebugLineToByteCodePos;
static std::map<uint32_t, std::vector<std::string>> ScriptDebugSource;

static bool ScriptVariablesEditorShown = false;
static bool ObjectViewerShown = false;
static bool UiViewerShown = false;
static bool ScriptDebuggerShown = false;

static uint32_t DebugThreadId = -1;
static bool DebuggerBreak = false;
static bool DebuggerContinueRequest = false;
static bool DebuggerStepRequest = false;
static std::map<int, std::pair<uint32_t, uint32_t>> DebuggerBreakpoints;

void __cdecl sc3ExecHook(ScriptThreadState* thread) {
  uint8_t* scrVal;
  uint32_t opcodeGrp;
  uint32_t opcode;
  uint32_t opcodeGrp1;
  int calDummy;

  if (DebugThreadId == thread->thread_id) {
    if (DebuggerBreak && !DebuggerStepRequest && !DebuggerContinueRequest)
      return;
  }

  *gameExeSCRflag = 0;
  do {
    auto scriptIp = (uint32_t)thread->pc -
                    gameExeSc3ScriptBuffers[thread->script_buffer_id];
    for (const auto& breakpoint : DebuggerBreakpoints) {
      uint32_t currentScriptId =
          gameExeScriptIdsToFileIds[thread->script_buffer_id];
      if (currentScriptId == breakpoint.second.first &&
          scriptIp == breakpoint.second.second && !DebuggerStepRequest &&
          !DebuggerContinueRequest) {
        DebuggerBreak = true;
        return;
      }
    }
    if (DebugThreadId == thread->thread_id) {
      DebuggerStepRequest = false;
    }
    if (DebuggerContinueRequest) {
      DebuggerContinueRequest = false;
      DebuggerBreak = false;
    }

    scrVal = thread->pc;
    opcodeGrp = *scrVal;
    if ((uint8_t)opcodeGrp == 0xFE) {
      thread->pc += 1;
      gameExeSc3Eval(thread, &calDummy);
    } else {
      opcode = *(scrVal + 1);
      opcodeGrp1 = opcodeGrp & 0x7F;

      if (opcodeGrp1 == 0x10) {
        InstTableUser1[opcode](thread);
      } else if (opcodeGrp1 == 0x01) {
        InstTableGraph[opcode](thread);
      } else if (!opcodeGrp1) {
        InstTableSystem[opcode](thread);
      }
    }
    if (DebugThreadId == thread->thread_id && DebuggerBreak)
      *gameExeSCRflag = 1;
  } while (!*gameExeSCRflag);
}

static void HelpMarker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::BeginItemTooltip()) {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

static void ParseScriptDebugData(uint32_t scriptId) {
  if (ScriptDebugSource.find(scriptId) != ScriptDebugSource.end()) return;

  std::map<int, int> byteCodePosToLine;
  std::map<int, int> lineToByteCodePos;
  std::vector<std::string> sourceLines;

  std::string fileName =
      config["patch"]["scriptdbg"][std::to_string(scriptId)].get<std::string>();

  std::ifstream infile("languagebarrier\\scriptdbg\\" + fileName);

  int lineId = 0;
  for (std::string line; std::getline(infile, line); lineId++) {
    if (line.empty()) continue;

    if (line[line.size() - 1] == '\r') line.pop_back();
    size_t firstColLength = line.find(',');

    if (firstColLength == std::string::npos ||
        firstColLength == line.length() - 1)
      continue;

    size_t secondColLength = line.find(',', firstColLength + 1);
    if (secondColLength == std::string::npos ||
        secondColLength == line.length() - 1)
      continue;

    uint32_t byteCodePos = std::atoi(line.substr(0, firstColLength).c_str());

    std::string sourceLine =
        line.substr(secondColLength + 2, line.length() - secondColLength);

    sourceLines.push_back(sourceLine);
    byteCodePosToLine[byteCodePos] = lineId;
    lineToByteCodePos[lineId] = byteCodePos;
  }

  ScriptDebugByteCodePosToLine[scriptId] = byteCodePosToLine;
  ScriptDebugLineToByteCodePos[scriptId] = lineToByteCodePos;
  ScriptDebugSource[scriptId] = sourceLines;

  infile.close();
}

void ShowSingleWindow() {
  if (ImGui::Begin("Debug Menu")) {
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

    if (ImGui::BeginTabBar("DebugTabBar", ImGuiTabBarFlags_None)) {
      if (ImGui::BeginTabItem("\"Debug Editer\"")) {
        ShowScriptVariablesEditor();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Script Debugger")) {
        ShowScriptDebugger();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }

  ImGui::End();
}

void ShowDockableArea() {
  if (ImGui::Begin("Debug Menu##DebugMenuDockArea", 0,
                   ImGuiWindowFlags_MenuBar)) {
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("Tools")) {
        ImGui::MenuItem("\"Debug Editer\"", NULL, &ScriptVariablesEditorShown);
        ImGui::MenuItem("Script Debugger", NULL, &ScriptDebuggerShown,
                        config["patch"].contains("script"));
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }

    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
  }
  ImGui::End();

  if (ScriptVariablesEditorShown) {
    if (ImGui::Begin("\"Debug Editer\"##ScriptVarEditorWindow"),
        &ScriptVariablesEditorShown) {
      ShowScriptVariablesEditor();
    }
    ImGui::End();
  }

  if (ScriptDebuggerShown) {
    if (ImGui::Begin("Script Debugger##ScriptDebuggerWindow"),
        &ScriptDebuggerShown) {
      ShowScriptDebugger();
    }
    ImGui::End();
  }
}

void ShowDebugMenu() {
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ShowDockableArea();
  } else {
    ShowSingleWindow();
  }
}

void ShowScriptVariablesEditor() {
  ImGui::PushItemWidth(10.0f * ImGui::GetFontSize());

  if (ImGui::TreeNode("ScrWork Config")) {
    ImGui::Text("Display number format");
    ImGui::SameLine();
    ImGui::RadioButton("DEC", &ScrWorkNumberFormat, 0);
    ImGui::SameLine();
    ImGui::RadioButton("HEX", &ScrWorkNumberFormat, 1);

    ImGui::PushButtonRepeat(true);
    ImGui::Spacing();
    ImGui::Text("Start index");
    ImGui::DragInt("##ScrWorkStartIndex", &ScrWorkIndexStart, 0.5f, 0, 8000,
                   "%04d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SameLine();
    if (ImGui::Button("+1##IncreaseScrWorkStartIndex")) ScrWorkIndexStart += 1;
    ImGui::SameLine();
    if (ImGui::Button("-1##DecreaseScrWorkStartIndex")) ScrWorkIndexStart -= 1;
    ImGui::SameLine();
    if (ImGui::Button("+100##IncreaseScrWorkStartIndex1"))
      ScrWorkIndexStart += 100;
    ImGui::SameLine();
    if (ImGui::Button("-100##DecreaseScrWorkStartIndex1"))
      ScrWorkIndexStart -= 100;
    ImGui::SameLine();
    if (ImGui::Button("+1000##IncreaseScrWorkStartIndex2"))
      ScrWorkIndexStart += 1000;
    ImGui::SameLine();
    if (ImGui::Button("-1000##DecreaseScrWorkStartIndex2"))
      ScrWorkIndexStart -= 1000;
    ImGui::PopButtonRepeat();

    ImGui::Spacing();
    ImGui::Text("End index");
    ImGui::DragInt("##ScrWorkEndIndex", &ScrWorkIndexEnd, 0.5f, 0, 8000, "%04d",
                   ImGuiSliderFlags_AlwaysClamp);

    ImGui::PushButtonRepeat(true);
    ImGui::SameLine();
    if (ImGui::Button("+1##IncreaseScrWorkEndIndex")) ScrWorkIndexEnd += 1;
    ImGui::SameLine();
    if (ImGui::Button("-1##DecreaseScrWorkEndIndex")) ScrWorkIndexEnd -= 1;
    ImGui::SameLine();
    if (ImGui::Button("+100##IncreaseScrWorkEndIndex1")) ScrWorkIndexEnd += 100;
    ImGui::SameLine();
    if (ImGui::Button("-100##DecreaseScrWorkEndIndex1")) ScrWorkIndexEnd -= 100;
    ImGui::SameLine();
    if (ImGui::Button("+1000##IncreaseScrWorkEndIndex2"))
      ScrWorkIndexEnd += 1000;
    ImGui::SameLine();
    if (ImGui::Button("-1000##DecreaseScrWorkEndIndex2"))
      ScrWorkIndexEnd -= 1000;
    ImGui::PopButtonRepeat();

    if (ScrWorkIndexEnd < ScrWorkIndexStart)
      ScrWorkIndexEnd = ScrWorkIndexStart;

    ImGui::TreePop();
  }

  ImGui::Spacing();

  if (ImGui::TreeNode("FlagWork Config")) {
    ImGui::Text("Start index");
    ImGui::DragInt("##FlagWorkStartIndex", &FlagWorkIndexStart, 0.5f, 0, 7000,
                   "%04d", ImGuiSliderFlags_AlwaysClamp);

    ImGui::PushButtonRepeat(true);
    ImGui::SameLine();
    if (ImGui::Button("+1##IncreaseFlagWorkStartIndex"))
      FlagWorkIndexStart += 1;
    ImGui::SameLine();
    if (ImGui::Button("-1##DecreaseFlagWorkStartIndex"))
      FlagWorkIndexStart -= 1;
    ImGui::SameLine();
    if (ImGui::Button("+100##IncreaseFlagWorkStartIndex1"))
      FlagWorkIndexStart += 100;
    ImGui::SameLine();
    if (ImGui::Button("-100##DecreaseFlagWorkStartIndex1"))
      FlagWorkIndexStart -= 100;
    ImGui::SameLine();
    if (ImGui::Button("+1000##IncreaseFlagWorkStartIndex2"))
      FlagWorkIndexStart += 1000;
    ImGui::SameLine();
    if (ImGui::Button("-1000##DecreaseFlagWorkStartIndex2"))
      FlagWorkIndexStart -= 1000;
    ImGui::PopButtonRepeat();

    ImGui::Spacing();
    ImGui::Text("End index");
    ImGui::DragInt("##FlagWorkEndIndex", &FlagWorkIndexEnd, 0.5f, 0, 7000,
                   "%04d", ImGuiSliderFlags_AlwaysClamp);

    ImGui::PushButtonRepeat(true);
    ImGui::SameLine();
    if (ImGui::Button("+1##IncreaseFlagWorkEndIndex")) FlagWorkIndexEnd += 1;
    ImGui::SameLine();
    if (ImGui::Button("-1##DecreaseFlagWorkEndIndex")) FlagWorkIndexEnd -= 1;
    ImGui::SameLine();
    if (ImGui::Button("+100##IncreaseFlagWorkEndIndex1"))
      FlagWorkIndexEnd += 100;
    ImGui::SameLine();
    if (ImGui::Button("-100##DecreaseFlagWorkEndIndex1"))
      FlagWorkIndexEnd -= 100;
    ImGui::SameLine();
    if (ImGui::Button("+1000##IncreaseFlagWorkEndIndex2"))
      FlagWorkIndexEnd += 1000;
    ImGui::SameLine();
    if (ImGui::Button("-1000##DecreaseFlagWorkEndIndex2"))
      FlagWorkIndexEnd -= 1000;
    ImGui::PopButtonRepeat();

    if (FlagWorkIndexEnd < FlagWorkIndexStart)
      FlagWorkIndexEnd = FlagWorkIndexStart;

    ImGui::TreePop();
  }

  ImGuiStyle& style = ImGui::GetStyle();
  float windowVisibleX2 =
      ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

  if (ImGui::CollapsingHeader("ScrWork Editor")) {
    for (int i = ScrWorkIndexStart; i <= ScrWorkIndexEnd; i++) {
      ImGui::PushID(i);
      float width = 0.0f;
      char buf[32];
      snprintf(buf, 32, "ScrWork[%04d]", i);
      int one = 1;
      int ten = 10;
      ImGui::InputScalar(buf, ImGuiDataType_S32, &gameExeScrWork[i], &one, &ten,
                         ScrWorkNumberFormat == 1 ? "%08lX" : "%d");
      float lastX2 = ImGui::GetItemRectMax().x;
      float nextButtonX2 =
          lastX2 + style.ItemSpacing.x + ImGui::GetItemRectSize().x;
      if (i + 1 <= ScrWorkIndexEnd && nextButtonX2 < windowVisibleX2)
        ImGui::SameLine();
      ImGui::PopID();
    }
  }

  if (ImGui::CollapsingHeader("FlagWork Editor")) {
    for (int i = FlagWorkIndexStart; i <= FlagWorkIndexEnd; i++) {
      ImGui::PushID(i);
      char buf[32];
      snprintf(buf, 32, "Flags[%04d]", i);
      bool flagVal = gameExeGetFlag(i);
      if (ImGui::Checkbox(buf, &flagVal)) gameExeSetFlag(i, flagVal);
      float lastX2 = ImGui::GetItemRectMax().x;
      float nextButtonX2 =
          lastX2 + style.ItemSpacing.x + ImGui::GetItemRectSize().x;
      if (i + 1 <= FlagWorkIndexEnd && nextButtonX2 < windowVisibleX2)
        ImGui::SameLine();
      ImGui::PopID();
    }
  }

  ImGui::PopItemWidth();
}

static int ScriptDebuggerSelectedThreadId = -1;
static int ThreadVarsNumberFormat = 0;
static bool AutoScrollSourceView = true;

void ShowScriptDebugger() {
  if (ScriptDebuggerSelectedThreadId == -1 ||
      gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].pc == 0) {
    int firstThreadId = 0;
    for (int i = 0; i < VmMaxThreads; i++) {
      if (gameExeSc3ThreadPool[i].pc != 0) {
        firstThreadId = i;
        break;
      }
    }
    ScriptDebuggerSelectedThreadId = firstThreadId;
  }

  char comboPreviewValue[128];
  auto groupType = ThreadGroupType::_from_integral_nothrow(
      gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].thread_group_id);
  std::string fileName =
      config["patch"]["script"]
            [std::to_string(
                 gameExeScriptIdsToFileIds
                     [gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                          .script_buffer_id])]
                .get<std::string>();
  if (groupType) {
    snprintf(comboPreviewValue, 128, "[%s][%d] %s",
             groupType.value()._to_string(),
             gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].thread_id,
             fileName.c_str());
  } else {
    snprintf(
        comboPreviewValue, 128, "[%d][%d] %s",
        gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].thread_group_id,
        gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].thread_id,
        fileName.c_str());
  }

  if (ImGui::BeginCombo("Thread##vmThreadCombo", comboPreviewValue,
                        ImGuiComboFlags_WidthFitPreview)) {
    for (int i = 0; i < VmMaxThreads; i++) {
      if (gameExeSc3ThreadPool[i].pc != 0) {
        const bool isSelected = (ScriptDebuggerSelectedThreadId == i);
        auto groupType = ThreadGroupType::_from_integral_nothrow(
            gameExeSc3ThreadPool[i].thread_group_id);
        std::string fileName =
            config["patch"]["script"]
                  [std::to_string(
                       gameExeScriptIdsToFileIds[gameExeSc3ThreadPool[i]
                                                     .script_buffer_id])]
                      .get<std::string>();
        if (groupType) {
          snprintf(comboPreviewValue, 128, "[%s][%d] %s",
                   groupType.value()._to_string(),
                   gameExeSc3ThreadPool[i].thread_id, fileName.c_str());
        } else {
          snprintf(comboPreviewValue, 128, "[%d][%d] %s",
                   gameExeSc3ThreadPool[i].thread_group_id,
                   gameExeSc3ThreadPool[i].thread_id, fileName.c_str());
        }
        if (ImGui::Selectable(comboPreviewValue, isSelected))
          ScriptDebuggerSelectedThreadId = i;
        if (isSelected) ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  ImGui::SeparatorText("Thread flags:");
  if (ImGui::BeginTable("tableThreadFlags", 3, ImGuiTableFlags_None)) {
    ImGui::TableNextColumn();
    if (ImGui::CheckboxFlags(
            "Destroy",
            &gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].accumulator,
            TF_Destroy))
      ScriptDebuggerSelectedThreadId = 0;
    ImGui::SameLine();
    HelpMarker("Setting this will DESTROY the thread IMMEDIATELY!");
    ImGui::CheckboxFlags(
        "Animate",
        &gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].accumulator,
        TF_Animate);
    ImGui::TableNextColumn();
    ImGui::CheckboxFlags(
        "Display",
        &gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].accumulator,
        TF_Display);
    ImGui::CheckboxFlags(
        "Pause",
        &gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].accumulator,
        TF_Pause);
    ImGui::TableNextColumn();
    ImGui::CheckboxFlags(
        "Message",
        &gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].accumulator,
        TF_Message);
    ImGui::EndTable();
  }

  ImGui::SeparatorText("Thread data:");
  if (ImGui::BeginTable("tableThreadData", 3, ImGuiTableFlags_None)) {
    ImGui::TableNextColumn();
    ImGui::Text("ID: %d",
                gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].thread_id);
    ImGui::Text(
        "IP: %08lX",
        (uint32_t)gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].pc -
            gameExeSc3ScriptBuffers
                [gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                     .script_buffer_id]);
    ImGui::Text("ExecPriority: %d",
                gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                    .execution_priority);

    ImGui::TableNextColumn();
    auto drawType = DrawComponentType::_from_integral_nothrow(
        gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].draw_type);
    if (drawType) {
      ImGui::Text("DrawType: %s", drawType.value()._to_string());
    } else {
      ImGui::Text(
          "DrawType: %d",
          gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].draw_type);
    }
    ImGui::Text(
        "DrawPriority: %d",
        gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].draw_priority);
    ImGui::Text("Alpha: %d",
                gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].alpha);

    ImGui::TableNextColumn();
    auto groupType = ThreadGroupType::_from_integral_nothrow(
        gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].thread_group_id);
    if (groupType) {
      ImGui::Text("Group: %s", groupType.value()._to_string());
    } else {
      ImGui::Text(
          "Group: %d",
          gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].thread_group_id);
    }
    ImGui::Text(
        "DialoguePageId: %d",
        gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].somePageNumber);
    ImGui::Text(
        "WaitCounter: %d",
        gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].sleep_timeout);

    ImGui::EndTable();
  }

  ImGui::Spacing();
  if (ImGui::TreeNode("Source View")) {
    uint32_t scriptId = (uint32_t)gameExeScriptIdsToFileIds
        [gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId].script_buffer_id];
    uint32_t scriptIp =
        (uint32_t)((uint32_t)
                       gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                           .pc -
                   gameExeSc3ScriptBuffers
                       [gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                            .script_buffer_id]);

    ParseScriptDebugData(scriptId);
    if (ScriptDebugSource.find(scriptId) != ScriptDebugSource.end()) {
      auto byteCodePosTable = ScriptDebugByteCodePosToLine[scriptId];
      auto lineToByteCodePosTable = ScriptDebugLineToByteCodePos[scriptId];
      auto currentLine = byteCodePosTable.lower_bound(scriptIp);
      int currentLineNum = -1;
      if (currentLine != byteCodePosTable.end()) {
        currentLineNum = currentLine->second;
      }

      DebugThreadId = ScriptDebuggerSelectedThreadId;
      ImGui::Checkbox("Auto scroll source view", &AutoScrollSourceView);
      ImGui::SameLine();
      if (DebuggerBreak) {
        if (ImGui::Button("Continue")) DebuggerContinueRequest = true;
      } else {
        if (ImGui::Button("Break")) DebuggerBreak = true;
      }
      ImGui::SameLine();
      ImGui::BeginDisabled(!DebuggerBreak);
      if (ImGui::Button("Step"))
        DebuggerStepRequest = true;
      else
        DebuggerStepRequest = false;
      ImGui::EndDisabled();
      ImGui::SameLine();
      if (ImGui::Button("Clear breakpoints")) {
        DebuggerBreakpoints.clear();
      }

      if (ImGui::TreeNode("Breakpoint List")) {
        if (ImGui::BeginTable(
                "BreakpointList", 1,
                ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
          char buf[256];
          for (auto it = DebuggerBreakpoints.cbegin(), nextIt = it;
               it != DebuggerBreakpoints.cend(); it = nextIt) {
            ++nextIt;
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            std::string fileName =
                config["patch"]["script"][std::to_string(it->second.first)]
                    .get<std::string>();
            bool isBreakpoint = true;
            snprintf(buf, 256, "%08lX - %d - %s - %s", it->second.second,
                     it->first, fileName.c_str(),
                     ScriptDebugSource[it->second.first][it->first].c_str());
            ImGui::Selectable(buf, &isBreakpoint,
                              ImGuiSelectableFlags_SpanAllColumns);
            if (!isBreakpoint) {
              DebuggerBreakpoints.erase(it);
            }
          }
          ImGui::EndTable();
        }

        ImGui::TreePop();
      }

      if (ImGui::BeginTable(
              "Source", 2,
              ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders |
                  ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
              ImVec2(ImGui::GetContentRegionAvail().x,
                     ImGui::GetContentRegionAvail().y * 0.9f))) {
        ImGuiListClipper clipper;
        clipper.Begin(ScriptDebugSource[scriptId].size());
        while (clipper.Step()) {
          for (int row = clipper.DisplayStart; row < clipper.DisplayEnd;
               row++) {
            ImGui::PushID(row);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", row + 1);
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Header,
                                  ImVec4(1.0f, 0.0f, 0.0f, 0.65f));
            bool isBreakpoint =
                DebuggerBreakpoints.find(row) != DebuggerBreakpoints.end();
            isBreakpoint =
                isBreakpoint &&
                DebuggerBreakpoints.find(row)->second.first == scriptId;
            ImGui::Selectable(ScriptDebugSource[scriptId][row].c_str(),
                              &isBreakpoint,
                              ImGuiSelectableFlags_SpanAllColumns);
            if (isBreakpoint) {
              DebuggerBreakpoints[row] = std::make_pair(
                  gameExeScriptIdsToFileIds
                      [gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                           .script_buffer_id],
                  lineToByteCodePosTable[row]);
            } else if (DebuggerBreakpoints.find(row) !=
                           DebuggerBreakpoints.end() &&
                       DebuggerBreakpoints.find(row)->second.first ==
                           scriptId) {
              DebuggerBreakpoints.erase(row);
            }
            ImGui::PopStyleColor();
            if (currentLineNum == row) {
              ImGui::TableSetBgColor(
                  ImGuiTableBgTarget_RowBg0,
                  ImGui::GetColorU32(ImVec4(0.0f, 0.7f, 0.0f, 0.65f)));
            }
            ImGui::PopID();
          }
        }
        if (AutoScrollSourceView)
          ImGui::SetScrollY((currentLineNum - 5) *
                            ImGui::GetTextLineHeightWithSpacing());
        ImGui::EndTable();
      }
    }
    ImGui::TreePop();
  }

  ImGui::Spacing();
  if (ImGui::TreeNode("Call Stack")) {
    for (int i = gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                     .call_stack_depth -
                 1;
         i >= 0; i--) {
      ImGui::PushID(i);
      std::string fileName =
          config["patch"]["script"]
                [std::to_string(
                     gameExeScriptIdsToFileIds
                         [gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                              .ret_address_script_buffer_ids[i]])]
                    .get<std::string>();
      ImGui::Text("%s - %08X", fileName.c_str(),
                  gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                      .ret_address_ids[i]);
      ImGui::PopID();
    }

    ImGui::TreePop();
  }
  ImGui::Spacing();

  ImGuiStyle& style = ImGui::GetStyle();
  float windowVisibleX2 =
      ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
  ImGui::PushItemWidth(10.0f * ImGui::GetFontSize());

  if (ImGui::TreeNode("Variables")) {
    ImGui::Text("Display number format");
    ImGui::SameLine();
    ImGui::RadioButton("DEC", &ThreadVarsNumberFormat, 0);
    ImGui::SameLine();
    ImGui::RadioButton("HEX", &ThreadVarsNumberFormat, 1);

    for (int i = 0; i < VmMaxThreadVars; i++) {
      ImGui::PushID(i);
      char buf[32];
      snprintf(buf, 32, "Vars[%02d]", i);
      int one = 1;
      ImGui::InputScalar(buf, ImGuiDataType_S32,
                         &gameExeSc3ThreadPool[ScriptDebuggerSelectedThreadId]
                              .thread_local_variables[i],
                         &one, 0, ThreadVarsNumberFormat == 1 ? "%08X" : "%d");
      float lastX2 = ImGui::GetItemRectMax().x;
      float nextButtonX2 =
          lastX2 + style.ItemSpacing.x + ImGui::GetItemRectSize().x;
      if (i + 1 < VmMaxThreadVars && nextButtonX2 < windowVisibleX2)
        ImGui::SameLine();
      ImGui::PopID();
    }

    ImGui::TreePop();
  }

  ImGui::PopItemWidth();
}

}  // namespace lb