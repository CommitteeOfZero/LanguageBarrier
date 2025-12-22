#include "Hooking.h"
using namespace lb;



HookManager& HookManager::instance() {
  static HookManager instance;
  return instance;
}

// isHookManaged now checks the new map.
bool HookManager::isHookManaged(const std::string& name) const {
  return enabled_hooks_.count(name) > 0;
}