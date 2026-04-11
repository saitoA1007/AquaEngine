#include "GameObjectManager.h"
#include <algorithm>
using namespace GameEngine;

void GameObjectManager::UpdateAll() {
    preSize_ = currentSize_;
    currentSize_ = objects_.size();

    // オブジェクトの更新処理をおこなう
    for (auto& obj : objects_) {
        if (obj->IsActive() && !obj->IsDead()) {
            obj->Update();
        }
    }

    // 削除フラグが立ったオブジェクトを削除
    std::erase_if(objects_, [](const std::unique_ptr<IGameObject>& obj) {
        return obj->IsDead();
        });

    // ソートをおこなう
    if (currentSize_ != preSize_) {
        std::sort(objects_.begin(), objects_.end(),
            [](const std::unique_ptr<IGameObject>& a, const std::unique_ptr<IGameObject>& b) {
                return a->GetUpdateOrder() < b->GetUpdateOrder();
            });
    }
}

void GameObjectManager::DrawAll() {
    for (auto& obj : objects_) {
        if (!obj->IsDead()) {
            obj->Draw();
        }
    }
}

void GameObjectManager::DebugUpdateAll() {
    for (auto& obj : objects_) {
        if (!obj->IsDead()) {
            obj->DebugUpdate();
        }
    }
}