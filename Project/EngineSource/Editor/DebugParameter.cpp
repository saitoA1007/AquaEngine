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

void DebugParameter::RemoveItem(const std::string& key, const std::string& subGroupName) {
    std::string path;
    if (subGroupName.empty()) {
        path = rootGroupName_;
    } else {
        path = rootGroupName_ + "/" + subGroupName;
    }

    // 登録を解除する
    auto it = std::remove_if(bindings_.begin(), bindings_.end(), [&](const std::unique_ptr<IParamBinding>& binding) {
        if (binding->GetGroupName() == path && binding->GetKeyName() == key) {
            binding->Remove();
            return true;
        }
        return false;
    });

    bindings_.erase(it, bindings_.end());
}

void DebugParameter::RemoveGroup(const std::string& subGroupName) {
    std::string path;
    if (subGroupName.empty()) {
        path = rootGroupName_;
    } else {
        path = rootGroupName_ + "/" + subGroupName;
    }

    // グループの登録を解除する
    gameParamEditor_->RemoveGroup(path);

    // グループの値を削除する
    auto it = std::remove_if(bindings_.begin(), bindings_.end(), [&](const std::unique_ptr<IParamBinding>& binding) {
        const std::string& groupName = binding->GetGroupName();
        if (groupName == path || groupName.starts_with(path + "/")) {
            return true;
        }
        return false;
        });

    // 実際に対象の要素を vector から削除・メモリ解放する
    bindings_.erase(it, bindings_.end());
}