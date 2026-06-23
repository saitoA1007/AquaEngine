#pragma once
#include "IEditorWindow.h"
#include "GameParamEditor.h"
#include "ImGuiManager.h"
#include "TextureManager.h"

namespace GameEngine {

	class InspectorWindow : public IEditorWindow {
	public:
		InspectorWindow(GameParamEditor* gameParamEditor, TextureManager* textureManager);

		void Draw() override;
		std::string GetName() const override { return "ParameterInspector"; }

	private:
		GameParamEditor* gameParamEditor_ = nullptr;
		
		TextureManager* textureManager_ = nullptr;

	private:

		void DrawGroup(GameParamEditor::Group& group);

		void DrawItems(GameParamEditor::Group& group);

	};

	// ImGuiで表示する用のパラメータを管理する
	struct DebugParameterVisitor {
		const std::string& itemName;
		bool& isDirty;
		TextureManager* textureManager_;
		explicit DebugParameterVisitor(const std::string& name,bool& dirty, TextureManager* textureManager)
			: itemName(name), isDirty(dirty), textureManager_(textureManager) {}

		// ## 付きのIDを生成するヘルパー
		std::string HiddenLabel() const {
			return "##" + itemName;
		}

		void operator()(int32_t& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::DragInt(HiddenLabel().c_str(), &value, 1)){ isDirty = true; }
		}

		void operator()(uint32_t& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::DragScalar(HiddenLabel().c_str(), ImGuiDataType_U32, &value, 1.0f)){ isDirty = true; }
		}

		void operator()(float& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::DragFloat(HiddenLabel().c_str(), &value, 0.01f)){ isDirty = true; }
		}

		void operator()(Vector2& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::DragFloat2(HiddenLabel().c_str(), reinterpret_cast<float*>(&value), 0.01f)){ isDirty = true; }
		}

		void operator()(Vector3& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::DragFloat3(HiddenLabel().c_str(), reinterpret_cast<float*>(&value), 0.01f)){ isDirty = true; }
		}

		void operator()(Vector4& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::ColorEdit4(HiddenLabel().c_str(), reinterpret_cast<float*>(&value))){ isDirty = true; }
		}

		void operator()(Range3& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {
				bool isChangeMin = ImGui::DragFloat3("Min", reinterpret_cast<float*>(&value.min), 0.01f);
				bool isChangeMax = ImGui::DragFloat3("Max", reinterpret_cast<float*>(&value.max), 0.01f);

				if (isChangeMin || isChangeMax) {
					value.min = Math::Min(value.min, value.max);
					value.max = Math::Max(value.min, value.max);
					isDirty = true;
				}
				ImGui::TreePop();
			}
		}

		void operator()(Range4& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {
				bool isChangeMin = ImGui::ColorEdit4("Min", reinterpret_cast<float*>(&value.min));
				bool isChangeMax = ImGui::ColorEdit4("Max", reinterpret_cast<float*>(&value.max));

				if (isChangeMin || isChangeMax) {
					value.min = Math::MinVector4(value.min, value.max);
					value.max = Math::MaxVector4(value.min, value.max);
					isDirty = true;
				}
				ImGui::TreePop();
			}
		}

		void operator()(EmitterShape& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {

				// 形状選択
				int typeIdx = static_cast<int>(value.type);
				if (ImGui::Combo("Shape", &typeIdx, EmitShapeTypeNames, kEmitShapeTypeCount)) {
					value.type = static_cast<EmitShapeType>(typeIdx);
					isDirty = true;
				}

				// 形状ごとのパラメータ
				switch (value.type) {
				case EmitShapeType::Point:
					ImGui::TextDisabled("No parameters");
					break;

				case EmitShapeType::Sphere:
				case EmitShapeType::Hemisphere:
					if (ImGui::DragFloat("Radius", &value.radius, 0.01f, 0.0f, FLT_MAX))
						isDirty = true;
					if (ImGui::Checkbox("EmitFromShell", &value.emitFromShell))
						isDirty = true;
					break;

				case EmitShapeType::Box:
					if (ImGui::DragFloat3("BoxSize", &value.boxSize.x, 0.01f, 0.0f, FLT_MAX))
						isDirty = true;
					break;
				}

				ImGui::TreePop();
			}
		}

		void operator()(TextureData& value) const {
			// 名前からハンドルを解決する
			if (textureManager_ && !value.name.empty()) {
				value.handle = textureManager_->GetHandleByName(value.name);
			}

			ImGui::Text("%s", itemName.c_str());

			// コンボボックスでテクスチャを選択
			const char* comboLabel = value.name.empty() ? "None" : value.name.c_str();
			if (ImGui::BeginCombo(HiddenLabel().c_str(), comboLabel)) {
				if (textureManager_) {
					for (const auto& texName : textureManager_->GetRegisteredTextureNames()) {
						bool isSelected = (texName == value.name);
						if (ImGui::Selectable(texName.c_str(), isSelected)) {
							value.name = texName;
							value.handle = textureManager_->GetHandleByName(texName);
							isDirty = true;
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
				}
				ImGui::EndCombo();
			}

			// プレビュー表示
			if (textureManager_ && !value.name.empty()) {
				D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = textureManager_->GetTextureSrvHandlesGPU(value.handle);
				ImGui::Image((ImTextureID)gpuHandle.ptr, ImVec2(64, 64));
			}
		}

		void operator()(bool& value) const {
			if (ImGui::Checkbox(itemName.c_str(), &value)) { isDirty = true; }
		}

		void operator()(std::string& value) const {
			ImGui::Text("%s", value.c_str());
		}

		// 対応出来ない型がきた場合の処理
		template<typename T>
		void operator()(T& value) const {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "[%s] は未対応の型です", HiddenLabel().c_str());
		}
	};
}
