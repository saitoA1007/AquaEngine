#pragma once
#include <string>
#include <vector>
#include <memory>
#include "GameParamEditor.h"

namespace GameEngine {

	class DebugParameter {
	public:
		DebugParameter(const std::string& rootGroupName);

        static void StaticInitialize(GameParamEditor* gameParamEditor) {
            gameParamEditor_ = gameParamEditor;
        }

		template<typename T>
        void Register(const std::string& key, T& valueRef, int priority = INT_MAX,const std::string subGroupName = "") {
            std::string path;
            if (subGroupName.empty()) {
                path = rootGroupName_;
            } else {
                path = rootGroupName_ + "/" + subGroupName;
            }

            // 値を登録する
            gameParamEditor_->AddItem(path, key, valueRef, priority);

            // 値を保持
            bindings_.push_back(std::make_unique<ParamBinding<T>>(path, key, valueRef));
        }

        // 値を適応する
        void Apply();

        // 値が変更された時だけ適応する
        bool ApplyIfDirty();

	private:
        // 基底クラス
        struct IParamBinding {
        public:
            virtual ~IParamBinding() = default;
            virtual void Apply() = 0;
            virtual bool IsDirty() const = 0;
            virtual void ClearDirty() = 0;
        };

        // 型ごとに実装
        template<typename T>
        struct ParamBinding : IParamBinding {
            std::string groupName;
            std::string key;
            T& valueRef;

            ParamBinding(const std::string& group, const std::string& k, T& ref)
                : groupName(group), key(k), valueRef(ref) {
            }

            void Apply() override {
                valueRef = gameParamEditor_->GetValue<T>(groupName, key);
            }

            bool IsDirty() const override {
                return gameParamEditor_->IsDirty(groupName, key);
            }

            void ClearDirty() override {
                gameParamEditor_->ClearDirty(groupName, key);
            }
        };

    private:
        static GameParamEditor* gameParamEditor_;
        std::string rootGroupName_;
        std::vector<std::unique_ptr<IParamBinding>> bindings_;
	};
}

