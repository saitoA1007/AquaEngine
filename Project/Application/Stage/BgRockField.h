#pragma once
#include <array>
#include <memory>
#include <vector>
#include "IGameObject.h"
#include "DebugParameter.h"
#include "ModelComponent.h"
#include "IceMaterial.h"
#include "Vector2.h"

/// <summary>
/// ステージの外側に背景の岩を円状に散らして配置する
/// </summary>
class BgRockField : public GameEngine::IGameObject {
public:
	// 1つの輪の配置設定
	struct RingParam {
		// 配置する数
		uint32_t count = 0;
		// 中心からの距離
		float radius = 0.0f;
		// 距離のばらつき
		float radiusJitter = 0.0f;
		// 大きさの最小値と最大値
		Vector2 scaleRange = { 1.0f,1.0f };
		// 高さのずれ(負の値で地面に埋める)
		float heightOffset = 0.0f;
		// 氷の岩になる割合(0~1)
		float iceRatio = 0.5f;
	};

public:
	BgRockField(GameEngine::Model* iceRockModel, GameEngine::Model* rockModel);
	~BgRockField() = default;

	// 初期化
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:

	// 輪の数
	static inline const uint32_t kRingNum = 3;
	// 1つの輪に置ける最大数
	static inline const uint32_t kMaxCountPerRing = 64;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// モデル
	GameEngine::Model* iceRockModel_ = nullptr;
	GameEngine::Model* rockModel_ = nullptr;

	// 氷の岩で共有するマテリアル
	GameEngine::IceMaterial iceMaterial_;
	// 普通の岩の色
	Vector4 rockColor_ = { 0.55f,0.6f,0.7f,1.0f };

	// 輪ごとの配置設定
	std::array<RingParam, kRingNum> rings_;

	// 配置の中心
	Vector3 centerPos_ = { 0.0f,-0.5f,0.0f };
	// 乱数のシード値
	uint32_t seed_ = 1;
	// X,Z軸の傾きの最大値
	float maxTilt_ = 0.2f;
	// 高さ方向だけ伸び縮みさせる倍率の範囲
	Vector2 heightScaleRange_ = { 0.7f,1.4f };

	// 生成した岩
	std::vector<std::unique_ptr<GameEngine::ModelComponent>> iceRocks_;
	std::vector<std::unique_ptr<GameEngine::ModelComponent>> rocks_;
	// 実際に配置している数
	uint32_t activeIceRockNum_ = 0;
	uint32_t activeRockNum_ = 0;

private:

	// 値を登録
	void RegisterParameter();

	// 現在の設定で岩を配置し直す
	void Generate();

	// 使用していない岩を取得する。足りなければ生成する
	GameEngine::ModelComponent* AcquireRock(bool isIce);
};
