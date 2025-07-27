#pragma once
#include "LanguageBarrier.h"  // For sigScan, LanguageBarrierLog
#include "lbjson.h"           // For the json class
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <set>
#include "SigScan.h"
namespace lb {

class IHook {
 public:
  virtual ~IHook() = default;
  virtual const std::string& getName() const = 0;
};

template <typename T>
class Hook : public IHook {
 public:
  using FuncType = T;
  FuncType original = nullptr;

  // We don't need a detour member anymore, as it's only used once.
  std::string name_;
  uintptr_t target_ = 0;

  Hook(std::string name) : name_(std::move(name)) {}

  const std::string& getName() const override { return name_; }

  // This one method does everything.
  bool findAndEnable(const json& signatures, FuncType detour) {
    if (!signatures.contains(name_)) {
      LanguageBarrierLog("Hook Error: Signature key \"" + name_ +
                         "\" not found.");
      return false;
    }

    target_ = sigScan("game", name_.c_str());
    if (!target_) {
      LanguageBarrierLog("Hook Error: sigScan failed for \"" + name_ + "\".");
      return false;
    }

    if (MH_CreateHook(reinterpret_cast<LPVOID>(target_),
                      reinterpret_cast<LPVOID>(detour),
                      reinterpret_cast<LPVOID*>(&original)) != MH_OK) {
      LanguageBarrierLog("Hook Error: MH_CreateHook failed for \"" + name_ +
                         "\".");
      return false;
    }

    if (MH_EnableHook(reinterpret_cast<LPVOID>(target_)) != MH_OK) {
      // If enabling fails, we should try to undo the creation.
      MH_RemoveHook(reinterpret_cast<LPVOID>(target_));
      LanguageBarrierLog("Hook Error: MH_EnableHook failed for \"" + name_ +
                         "\".");
      return false;
    }

    LanguageBarrierLog("Hook enabled successfully for \"" + name_ + "\".");
    return true;
  }
};
class HookManager {
 public:
  static HookManager& instance();
  HookManager(const HookManager&) = delete;
  HookManager& operator=(const HookManager&) = delete;

  // The registration function does everything now.
  template <typename T>
  void registerAndEnableHook(const std::string& name, T detour,
                             Hook<T>** ppHook, const json& signatures);

  bool isHookManaged(const std::string& name) const;

 private:
  HookManager() = default;
  // We just store the hook objects by name for later retrieval.
  std::unordered_map<std::string, std::unique_ptr<IHook>> enabled_hooks_;
};

// --- Template Implementation ---
template <typename T>
void HookManager::registerAndEnableHook(const std::string& name, T detour,
                                        Hook<T>** ppHook,
                                        const json& signatures) {
  if (isHookManaged(name)) return;

  auto hook = std::make_unique<Hook<T>>(name);

  // Attempt to find, create, and enable the hook.
  if (hook->findAndEnable(signatures, detour)) {
    // Success! Store the hook object.
    if (ppHook) {
      *ppHook = hook.get();
    }
    enabled_hooks_[name] = std::move(hook);
  }
  // If it fails, the unique_ptr 'hook' is destroyed. The error is already
  // logged.
}
}