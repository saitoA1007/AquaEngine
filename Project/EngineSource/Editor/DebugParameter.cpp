#include "DebugParameter.h"

using namespace GameEngine;

GameParamEditor* DebugParameter::gameParamEditor_ = nullptr;

DebugParameter::DebugParameter(const std::string& rootGroupName) {
	rootGroupName_ = rootGroupName;
}

void DebugParameter::Apply() {
    for (auto& binding : bindings_) {
        binding->Apply();
        binding->ClearDirty();
    }
}

bool DebugParameter::ApplyIfDirty() {
#ifdef USE_IMGUI
    bool anyDirty = false;
    for (auto& binding : bindings_) {
        if (binding->IsDirty()) {
            binding->Apply();
            binding->ClearDirty();
            anyDirty = true;
        }
    }
    return anyDirty;
#else
    return false;
#endif
}