#include "pch.h"
#include "OptionUIManager.h"
#include "TextureManager.h"
#include "FontManager.h"
#include "EasingManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

OptionUIManager::OptionUIManager(GameEngine::TextureManager* textureManager, GameEngine::FontManager* fontManager) {

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("OptionUI");
	int i = 0;
	std::string subGroup = "Color";
	debugParame_->Register("Bg", bgColor_, i++, subGroup);
	debugParame_->Register("Cursor", cursorColor_, i++, subGroup);
	debugParame_->Register("Text", textColor_, i++, subGroup);
	debugParame_->Register("SelectText", selectTextColor_, i++, subGroup);
	debugParame_->Register("BarBg", barBgColor_, i++, subGroup);
	i = 0;
	subGroup = "Layout";
	debugParame_->Register("TitlePos", titlePos_, i++, subGroup);
	debugParame_->Register("TitleSize", titleSize_, i++, subGroup);
	debugParame_->Register("RowStartY", rowStartY_, i++, subGroup);
	debugParame_->Register("RowSpacing", rowSpacing_, i++, subGroup);
	debugParame_->Register("LabelX", labelX_, i++, subGroup);
	debugParame_->Register("ValueX", valueX_, i++, subGroup);
	debugParame_->Register("TextSize", textSize_, i++, subGroup);
	debugParame_->Register("CursorSize", cursorSize_, i++, subGroup);
	debugParame_->Register("BarSize", barSize_, i++, subGroup);
	debugParame_->Register("VolumeNumberOffsetX", volumeNumberOffsetX_, i++, subGroup);
	debugParame_->Register("GuidePos", guidePos_, i++, subGroup);
	debugParame_->Register("GuideSize", guideSize_, i++, subGroup);
	i = 0;
	subGroup = "Animation";
	debugParame_->Register("FadeTime", fadeTime_, i++, subGroup);
	debugParame_->Apply();

	// 単色の画像
	uint32_t whiteGH = textureManager->GetHandleByName("white2x2.png");
	bgSprite_.textureHandle_ = whiteGH;
	cursorSprite_.textureHandle_ = whiteGH;
	cursorSprite_.anchorPoint_ = { 0.5f,0.5f };
	for (uint32_t j = 0; j < volumeBarSprites_.size(); ++j) {
		volumeBarBgSprites_[j].textureHandle_ = whiteGH;
		volumeBarBgSprites_[j].anchorPoint_ = { 0.0f,0.5f };
		volumeBarSprites_[j].textureHandle_ = whiteGH;
		volumeBarSprites_[j].anchorPoint_ = { 0.0f,0.5f };
	}

	// 文字
	const Font* font = fontManager->GetFont("Gisshiri");
	titleText_ = std::make_unique<Text>(font, "オプション");
	titleText_->anchorPoint_ = { 0.5f,0.5f };

	const std::array<std::string, static_cast<uint32_t>(Item::kMaxCount)> labels = {
		"BGM 音量",
		"SE 音量",
		"視点操作 左右",
		"視点操作 上下",
		"もどる",
	};
	for (uint32_t j = 0; j < static_cast<uint32_t>(Item::kMaxCount); ++j) {
		labelTexts_[j] = std::make_unique<Text>(font, labels[j]);
		labelTexts_[j]->anchorPoint_ = { 0.0f,0.5f };
	}
	for (auto& text : valueTexts_) {
		text = std::make_unique<Text>(font, "", 16);
	}

	guideText_ = std::make_unique<Text>(font, "B ：もどる", 48);
	guideText_->anchorPoint_ = { 0.5f,0.5f };

	// 初期値を表示
	SetSettings(settings_);
}

void OptionUIManager::Initialize() {
	selectItem_ = Item::kBgmVolume;
	fadeTimer_ = 0.0f;
}

void OptionUIManager::SetSettings(const OptionSettings& settings) {
	// 変化があった時だけ文字を作り直す
	bool isChanged = !isSettingsApplied_ ||
		settings.bgmVolumeLevel != settings_.bgmVolumeLevel ||
		settings.seVolumeLevel != settings_.seVolumeLevel ||
		settings.isInvertCameraX != settings_.isInvertCameraX ||
		settings.isInvertCameraY != settings_.isInvertCameraY;

	settings_ = settings;
	isSettingsApplied_ = true;

	if (isChanged) {
		UpdateValueTexts();
	}
}

void OptionUIManager::UpdateValueTexts() {
	auto invertText = [](bool isInvert) {
		return std::string(isInvert ? "<  リバース  >" : "<  ノーマル  >");
		};

	valueTexts_[static_cast<uint32_t>(Item::kBgmVolume)]->SetText(std::to_string(settings_.bgmVolumeLevel));
	valueTexts_[static_cast<uint32_t>(Item::kSeVolume)]->SetText(std::to_string(settings_.seVolumeLevel));
	valueTexts_[static_cast<uint32_t>(Item::kCameraX)]->SetText(invertText(settings_.isInvertCameraX));
	valueTexts_[static_cast<uint32_t>(Item::kCameraY)]->SetText(invertText(settings_.isInvertCameraY));
}

void OptionUIManager::Update() {
	debugParame_->ApplyIfDirty();

	// フェードイン(ポーズ中でも動くように実時間を使う)
	if (fadeTimer_ < 1.0f) {
		fadeTimer_ = (fadeTime_ > 0.0f) ? std::min(fadeTimer_ + FpsCounter::GetDeltaTime() / fadeTime_, 1.0f) : 1.0f;
	}
	const float alpha = Lerp(0.0f, 1.0f, fadeTimer_, EaseType::kEaseOutQuad);

	const uint32_t selectIndex = static_cast<uint32_t>(selectItem_);

	// 背景
	bgSprite_.position_ = { 0.0f,0.0f };
	bgSprite_.size_ = { 1280.0f,720.0f };
	bgSprite_.color_ = MulAlpha(bgColor_, alpha);
	bgSprite_.Update();

	// 選択帯
	cursorSprite_.position_ = { 640.0f, GetRowY(selectIndex) };
	cursorSprite_.size_ = cursorSize_;
	cursorSprite_.color_ = MulAlpha(cursorColor_, alpha);
	cursorSprite_.Update();

	// タイトル
	titleText_->position_ = titlePos_;
	titleText_->scale_ = { titleSize_,titleSize_ };
	titleText_->color_ = MulAlpha(textColor_, alpha);
	titleText_->Update();

	// 各項目
	for (uint32_t i = 0; i < static_cast<uint32_t>(Item::kMaxCount); ++i) {
		const float rowY = GetRowY(i);
		const Vector4 color = MulAlpha((i == selectIndex) ? selectTextColor_ : textColor_, alpha);

		labelTexts_[i]->position_ = { labelX_, rowY };
		labelTexts_[i]->scale_ = { textSize_,textSize_ };
		labelTexts_[i]->color_ = color;
		labelTexts_[i]->Update();

		if (i >= valueTexts_.size()) { continue; }

		Text& value = *valueTexts_[i];
		value.scale_ = { textSize_,textSize_ };
		value.color_ = color;

		// 音量はバーと数値で表示
		if (i < volumeBarSprites_.size()) {
			const int32_t level = (i == static_cast<uint32_t>(Item::kBgmVolume)) ? settings_.bgmVolumeLevel : settings_.seVolumeLevel;
			const float rate = OptionSettings::ToVolume(level);
			const float barLeft = valueX_ - barSize_.x * 0.5f;

			volumeBarBgSprites_[i].position_ = { barLeft, rowY };
			volumeBarBgSprites_[i].size_ = barSize_;
			volumeBarBgSprites_[i].color_ = MulAlpha(barBgColor_, alpha);
			volumeBarBgSprites_[i].Update();

			volumeBarSprites_[i].position_ = { barLeft, rowY };
			volumeBarSprites_[i].size_ = { barSize_.x * rate, barSize_.y };
			volumeBarSprites_[i].color_ = color;
			volumeBarSprites_[i].Update();

			value.anchorPoint_ = { 0.0f,0.5f };
			value.position_ = { barLeft + barSize_.x + volumeNumberOffsetX_, rowY };
		} else {
			value.anchorPoint_ = { 0.5f,0.5f };
			value.position_ = { valueX_, rowY };
		}
		value.Update();
	}

	// 操作説明
	guideText_->position_ = guidePos_;
	guideText_->scale_ = { guideSize_,guideSize_ };
	guideText_->color_ = MulAlpha(textColor_, alpha);
	guideText_->Update();
}

void OptionUIManager::Draw() {

	// 背景
	renderQueue_->SubmitSprite(&bgSprite_);
	renderQueue_->SubmitSprite(&cursorSprite_);

	// ボリュームの調整バー
	for (uint32_t i = 0; i < volumeBarSprites_.size(); ++i) {
		renderQueue_->SubmitSprite(&volumeBarBgSprites_[i]);
		renderQueue_->SubmitSprite(&volumeBarSprites_[i]);
	}

	renderQueue_->SubmitText(titleText_.get());
	for (auto& text : labelTexts_) {
		renderQueue_->SubmitText(text.get());
	}
	for (auto& text : valueTexts_) {
		renderQueue_->SubmitText(text.get());
	}
	renderQueue_->SubmitText(guideText_.get());
}
