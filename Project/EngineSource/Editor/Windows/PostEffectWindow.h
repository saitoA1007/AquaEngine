#pragma once
#include "IEditorWindow.h"
#include "ImGuiManager.h"

namespace GameEngine {

	// 前方宣言
	class PostEffectManager;
	class TextureManager;
	class RenderPassController;

	class PostEffectWindow : public IEditorWindow {
	public:
		PostEffectWindow(PostEffectManager* postEffectManager, TextureManager* textureManager, RenderPassController* renderPassController);
		~PostEffectWindow();

		void Draw() override;
		std::string GetName() const override { return "PostEffect"; };

	private:
		// ポストエフェクト管理
		PostEffectManager* postEffectManager_ = nullptr;
		// テクスチャ管理
		TextureManager* textureManager_ = nullptr;
		// 描画のテクスチャ
		RenderPassController* renderPassController_ = nullptr;

		ax::NodeEditor::EditorContext* context_ = nullptr;

		// 初回とリセット後にノード位置をエディタへ反映する
		bool applyPositions_ = true;
		// 右クリックしたキャンバス座標
		ImVec2 popupCanvasPos_{};

		// 画像
		uint32_t headerBgHandle_ = 0;
		uint32_t iconRestoreHandle_ = 0;
		uint32_t iconSaveHandle_ = 0;
	};
}