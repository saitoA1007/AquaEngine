#pragma once
#include "IEditorWindow.h"
#include "GameParamEditor.h"
#include "ImGuiManager.h"

namespace GameEngine {

	class InspectorWindow : public IEditorWindow {
	public:
		void Draw() override;
		std::string GetName() const override { return "ParameterInspector"; }

	};

	// ImGuiで表示する用のパラメータを管理する
	struct DebugParameterVisitor {
		const std::string& itemName;
		explicit DebugParameterVisitor(const std::string& name) : itemName(name) {}

		void operator()(int32_t& value) const {
			ImGui::DragInt(itemName.c_str(), &value, 1);
		}

		void operator()(uint32_t& value) const {
			ImGui::DragScalar(itemName.c_str(), ImGuiDataType_U32, &value, 1.0f);
		}

		void operator()(float& value) const {
			ImGui::DragFloat(itemName.c_str(), &value, 0.01f);
		}

		void operator()(Vector2& value) const {
			ImGui::DragFloat2(itemName.c_str(), reinterpret_cast<float*>(&value), 0.01f);
		}

		void operator()(Vector3& value) const {
			ImGui::DragFloat3(itemName.c_str(), reinterpret_cast<float*>(&value), 0.01f);
		}

		void operator()(Vector4& value) const {
			ImGui::ColorEdit4(itemName.c_str(), reinterpret_cast<float*>(&value));
		}

		void operator()(Range3& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {
				bool isChangeMin = ImGui::DragFloat3("Min", reinterpret_cast<float*>(&value.min), 0.01f);
				bool isChangeMax = ImGui::DragFloat3("Max", reinterpret_cast<float*>(&value.max), 0.01f);

				if (isChangeMin || isChangeMax) {
					value.min = Min(value.min, value.max);
					value.max = Max(value.min, value.max);
				}
				ImGui::TreePop();
			}
		}

		void operator()(Range4& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {
				bool isChangeMin = ImGui::ColorEdit4("Min", reinterpret_cast<float*>(&value.min));
				bool isChangeMax = ImGui::ColorEdit4("Max", reinterpret_cast<float*>(&value.max));

				if (isChangeMin || isChangeMax) {
					value.min = MinVector4(value.min, value.max);
					value.max = MaxVector4(value.min, value.max);
				}
				ImGui::TreePop();
			}
		}

		void operator()(bool& value) const {
			ImGui::Checkbox(itemName.c_str(), &value);
		}

		void operator()(std::string& value) const {
			//ImGui::InputText(itemName.c_str(), value.data());
			ImGui::Text(itemName.c_str(), &value);
		}

		// 対応出来ない型がきた場合の処理
		template<typename T>
		void operator()(T& value) const {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "[%s] は未対応の型です", itemName.c_str());
		}
	};
}
