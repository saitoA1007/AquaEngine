#pragma once
#include <unordered_set>
#include "IGameObject.h"
#include "Collider.h"
#include "Model.h"
#include "WorldTransform.h"
#include "FractureInstance.h"

namespace GameEngine {

	class DestructibleObject : public IGameObject {
	public:
		DestructibleObject(Model* model, uint32_t colliderId, uint32_t colliderAttribute);
		~DestructibleObject() = default;

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

	public:

		// ワールド行列
		WorldTransform worldTransform_;

		// 一時的なテスト用の項目
		Vector3 colliderSize_ = { 2.5f,2.5f,2.5f };
		float testDamageAmount_ = 2.0f;
		float testCraterRadius_ = 2.0f;
		int testPlaneCount_ = 8;
	private:
		// モデル
		Model* model_ = nullptr;

		// aabbの当たり判定
		AABBCollider collider_;

		// このオブジェクトが持つ破壊グループ名
		std::string groupName_;
		// chunkIdから素早く引くためのマップ（Initializeで構築）
		std::unordered_map<uint32_t, const FractureChunkEntry*> chunksById_;

		// 元の静的チャンク
		FractureInstance intactInstance_;
		bool hasIntact_ = false;

		// 事前分割のまま切り離されて落ちる破片
		FractureInstance macroDebrisInstance_;
		bool hasMacroDebris_ = false;

		// ランタイムカットされた破片
		FractureInstance microDebrisInstance_;
		bool hasMicroDebris_ = false;

		// 既に切り離し済みのチャンクID
		std::unordered_set<uint32_t> destroyedChunkIds_;

		// チャンクごとの蓄積ダメージ
		std::unordered_map<uint32_t, float> chunkDamage_;

		// chunkIdがintactInstance_の何番目のインスタンスか
		std::unordered_map<uint32_t, uint32_t> chunkIndexInIntact_;

		// モデル全体のおおよその中心。ひびを入れるのに使用
		Vector3 modelCenter_ = { 0.0f, 0.0f, 0.0f };

		float kBreakThreshold_ = 3.0f;
		// 閾値到達寸前の最大ズレ量
		float kMaxCrackOffset_ = 0.04f;
		// 閾値到達寸前の最大ランダム回転
		float kMaxCrackRotate_ = 0.15f;
		// 隣接チャンクへ波及させる強さ
		float kNeighborCrackFactor_ = 0.35f;

		// ばね物理のパラメータ
		float kCrackSpringStiffness_ = 400.0f;         // バネ定数
		float kCrackDamping_ = 30.0f;                  // 減衰係数
		float kCrackAngularSpringStiffness_ = 250.0f;
		float kCrackAngularDamping_ = 15.0f;
		float kCrackImpulseStrength_ = 1.8f;
		// 現在バネが揺れているチャンクのID
		std::unordered_set<uint32_t> crackActiveChunkIds_;
	private:

		// 当たり判定のコールバック関数
		void OnCollisionEnter(const GameEngine::CollisionResult& result);

		void ApplyDamage(const Vector3& impactPos, float damageRadius);

		// 隣接グラフをフラッドフィルして、切り離すチャンク群を選ぶ
		std::vector<uint32_t> SelectDetachedChunks(uint32_t seedChunkId, float damageRadius, const Vector3& impactPos) const;

		// impactPosに一番近い（まだ壊れていない）チャンクを探す
		std::optional<uint32_t> FindNearestChunk(const Vector3& impactPos) const;

		// 破片に簡易的な落下運動を与える（本来は物理エンジンに置き換える）
		void SimulateFallingDebris(FractureInstance& instance, float deltaTime);

		// 破片の爆発
		void ApplyExplosionImpulse(FractureInstance& instance,
			const std::vector<uint32_t>& chunkIds, const Vector3& impactPos, float damageRadius);

		void ApplyExplosionImpulseUniform(FractureInstance& instance,
			const Vector3& impactPos, float strength);

		void ApplyChipDamage(const Vector3& impactPos, float damageAmount);
		void UpdateCrackVisual(uint32_t chunkId, float ratio, float damageDelta);
		void RebuildIntactIndexMap(const std::vector<uint32_t>& ids);

		void SimulateCrackPhysics();
	};
}

