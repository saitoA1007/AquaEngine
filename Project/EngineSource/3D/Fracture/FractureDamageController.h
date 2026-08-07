#pragma once
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <string>
#include <vector>
#include "Model.h"
#include "FractureBreakState.h"

namespace GameEngine {

	/// <summary>
	/// 破壊オブジェクトの状態を管理する
	/// </summary>
	class FractureDamageController {
	public:
		// モデルの破壊チャンク情報から初期化
		void Initialize(Model* model);

		// 落下シミュレーションとひび割れバネ物理を更新
		void Update(float deltaTime);

		// 指定位置にダメージを蓄積させる。
		void ApplyChipDamage(const Vector3& impactPos, float damageAmount, float craterRadius, int craterPlaneCount);

		FractureBreakState& GetBreakState() { return breakState_; }
		const FractureBreakState& GetBreakState() const { return breakState_; }

	private:
		// モデル
		Model* model_ = nullptr;

		// このオブジェクトが持つ破壊グループ名
		std::string groupName_;
		// chunkIdから素早く引くためのマップ
		std::unordered_map<uint32_t, const FractureChunkEntry*> chunksById_;

		// 無傷、マクロ破片、マイクロ破片の3インスタンス
		FractureBreakState breakState_;

		// 既に切り離し済みのチャンクID
		std::unordered_set<uint32_t> destroyedChunkIds_;

		// チャンクごとの蓄積ダメージ
		std::unordered_map<uint32_t, float> chunkDamage_;

		// chunkIdがbreakState_.Intact()の何番目のインスタンスか
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
		// チャンクを切り離して破片化する
		void ApplyDamage(const Vector3& impactPos, float damageRadius, float craterRadius, int craterPlaneCount);

		// 隣接グラフをフラッドフィルして、切り離すチャンク群を選ぶ
		std::vector<uint32_t> SelectDetachedChunks(uint32_t seedChunkId, float damageRadius, const Vector3& impactPos) const;

		// impactPosに一番近いまだ壊れていないチャンクを探す
		std::optional<uint32_t> FindNearestChunk(const Vector3& impactPos) const;

		// 破片に落下運動を与える
		void SimulateFallingDebris(FractureInstance& instance, float deltaTime);

		// 破片の爆発
		void ApplyExplosionImpulse(FractureInstance& instance,
			const std::vector<uint32_t>& chunkIds, const Vector3& impactPos, float damageRadius);

		void ApplyExplosionImpulseUniform(FractureInstance& instance,
			const Vector3& impactPos, float strength);

		void UpdateCrackVisual(uint32_t chunkId, float ratio, float damageDelta);
		void RebuildIntactIndexMap(const std::vector<uint32_t>& ids);

		void SimulateCrackPhysics();
	};
}
