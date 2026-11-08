#pragma once
#include <array>
#include <memory>
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"
#include "Text.h"
#include "Application/GamePlay/OptionSettings.h"

// 前方宣言
namespace GameEngine {
	class TextureManager;
	class FontManager;
}

/// <summary>
/// オプションUIの管理
/// </summary>
class OptionUIManager : public GameEngine::IGameObject {
public:

	// 選択項目
	enum class Item {
		kBgmVolume, // BGM音量
		kSeVolume,  // SE音量
		kCameraX,   // 視点の左右操作
		kCameraY,   // 視点の上下操作
		kBack,      // 戻る

		kMaxCount
	};

public:
	OptionUIManager(GameEngine::TextureManager* textureManager, GameEngine::FontManager* fontManager);
	~OptionUIManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 選択中の項目を設定
	void SetSelectItem(Item item) { selectItem_ = item; }
	Item GetSelectItem() const { return selectItem_; }

	// 表示する設定値を反映
	void SetSettings(const OptionSettings& settings);

private: // 調整項目

	// 背景の色
	Vector4 bgColor_ = { 0.0f,0.0f,0.0f,0.75f };
	// 選択帯の色
	Vector4 cursorColor_ = { 1.0f,1.0f,1.0f,0.15f };
	// 通常の文字色
	Vector4 textColor_ = { 1.0f,1.0f,1.0f,1.0f };
	// 選択中の文字色
	Vector4 selectTextColor_ = { 1.0f,0.85f,0.3f,1.0f };
	// 音量バーの背景色
	Vector4 barBgColor_ = { 0.3f,0.3f,0.3f,1.0f };

	// タイトルの位置と大きさ
	Vector2 titlePos_ = { 640.0f,110.0f };
	float titleSize_ = 56.0f;

	// 項目の配置
	float rowStartY_ = 230.0f;
	float rowSpacing_ = 80.0f;
	float labelX_ = 300.0f;
	float valueX_ = 860.0f;
	float textSize_ = 36.0f;

	// 選択帯の大きさ
	Vector2 cursorSize_ = { 820.0f,64.0f };

	// 音量バーの大きさ
	Vector2 barSize_ = { 300.0f,16.0f };
	// 音量の数値をバーからどれだけ離すか
	float volumeNumberOffsetX_ = 30.0f;

	// 操作説明の位置と大きさ
	Vector2 guidePos_ = { 640.0f,660.0f };
	float guideSize_ = 24.0f;

	// フェードインの時間
	float fadeTime_ = 0.2f;

private:

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 半透明の背景
	GameEngine::Sprite bgSprite_;

	// 選択中の項目の帯
	GameEngine::Sprite cursorSprite_;

	// 音量バー
	std::array<GameEngine::Sprite, 2> volumeBarBgSprites_;
	std::array<GameEngine::Sprite, 2> volumeBarSprites_;

	// タイトル文字
	std::unique_ptr<GameEngine::Text> titleText_;

	// 各項目の名前
	std::array<std::unique_ptr<GameEngine::Text>, static_cast<uint32_t>(Item::kMaxCount)> labelTexts_;

	// 各項目の値
	std::array<std::unique_ptr<GameEngine::Text>, static_cast<uint32_t>(Item::kMaxCount) - 1> valueTexts_;

	// 操作説明
	std::unique_ptr<GameEngine::Text> guideText_;

	// 選択中の項目
	Item selectItem_ = Item::kBgmVolume;

	// 表示中の設定
	OptionSettings settings_;
	bool isSettingsApplied_ = false;

	// フェードインのタイマー
	float fadeTimer_ = 1.0f;

private:

	// 値の文字を更新
	void UpdateValueTexts();

	// 項目の行のY座標
	float GetRowY(uint32_t index) const {
		return rowStartY_ + rowSpacing_ * static_cast<float>(index);
	}

	// アルファを掛けた色
	static Vector4 MulAlpha(const Vector4& color, float alpha) {
		return { color.x,color.y,color.z,color.w * alpha }; 
	}
};
