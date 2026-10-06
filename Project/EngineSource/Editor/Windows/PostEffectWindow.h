#pragma once
#include "IEditorWindow.h"
#include "ImGuiManager.h"

namespace GameEngine {

	// 前方宣言
	class PostEffectManager;

	class PostEffectWindow : public IEditorWindow {
	public:
		PostEffectWindow(PostEffectManager* postEffectManager);
		~PostEffectWindow();

		void Draw() override;
		std::string GetName() const override { return "PostEffect"; };

	private:
		// ポストエフェクト管理
		PostEffectManager* postEffectManager_ = nullptr;
		ax::NodeEditor::EditorContext* context_ = nullptr;

		// 初回とリセット後にノード位置をエディタへ反映する
		bool applyPositions_ = true;
		// 右クリックしたキャンバス座標
		ImVec2 popupCanvasPos_{};
	};
}