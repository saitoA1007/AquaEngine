#pragma once
#include <vector>
#include <memory>
#include "IGameObject.h"

namespace GameEngine {

    /// <summary>
    /// ゲームオブジェクト管理クラス
    /// </summary>
    class GameObjectManager {
    public:
        GameObjectManager() = default;
        ~GameObjectManager() = default;

        /// <summary>
        /// オブジェクトを生成して追加する
        /// </summary>
        template<typename T, typename... Args>
        T* AddObject(Args&&... args) {
            static_assert(std::is_base_of<IGameObject, T>::value, "T must derive from GameObjectBase");
            auto obj = std::make_unique<T>(std::forward<Args>(args)...);
            T* ptr = obj.get();
            obj->Initialize();
            objects_.push_back(std::move(obj));
            return ptr;
        }

        // 初期化処理
        void InitializeAll();

        // 更新処理
        void UpdateAll();

        // 描画処理
        void DrawAll();
        // デバック更新処理
        void DebugUpdateAll();

        // 解放
        void ClearAll() { objects_.clear(); }

    private:
        // オブジェクトデータ
        std::vector<std::unique_ptr<IGameObject>> objects_;

        size_t currentSize_ = 0;
        size_t preSize_ = 0;
    };
}