#pragma once
#include"VertexData.h"
#include"AnimationData.h"

#include"Model.h"

namespace GameEngine {

	class Animator {
	public:

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="model"></param>
		/// <param name="animationData"></param>
		void Initialize(Model* model, const AnimationData* animationData);

		/// <summary>
		/// 更新処理(再生を自動管理)
		/// </summary>
		void Update();

		/// <summary>
		/// 更新処理(再生を手動管理)
		/// </summary>
		/// <param name="time"></param>
		void Update(const float& time);

		/// <summary>
		/// 最大再生時間を取得
		/// </summary>
		/// <returns></returns>
		float GetMaxTime() const { return animationData_->duration; }

		/// <summary>
		/// 現在の再生時間を取得
		/// </summary>
		/// <returns></returns>
		float GetTimer() const { return timer_; }

		/// <summary>
		/// ループ状態を設定
		/// </summary>
		/// <param name="isLoop"></param>
		void SetIsLoop(const bool& isLoop) { isLoop_ = isLoop; }

		/// <summary>
		/// アニメーションデータを設定
		/// </summary>
		/// <param name="animationData"></param>
		void SetAnimationData(const AnimationData* animationData) { animationData_ = animationData; }

		/// <summary>
		/// アニメーションに付属するモデルデータを設定
		/// </summary>
		/// <param name="model"></param>
		void SetModelData(Model* model);

	private:
		// ループの管理
		bool isLoop_ = true;

		// 再生するアニメーションのデータ
		const AnimationData* animationData_ = nullptr;

		// 使用するモデルデータ
		SkinCluster* skinCluster_ = nullptr;
		Skeleton* skeleton_ = nullptr;

		// 時間
		float timer_ = 0.0f;

	private:

		/// <summary>
		/// アニメーション用の値を取得
		/// </summary>
		/// <param name="keyframes"></param>
		/// <param name="time"></param>
		/// <returns></returns>
		static Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);
		static Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);

		/// <summary>
		/// skeletonに対してアニメーションを適応する
		/// </summary>
		/// <param name="skeleton"></param>
		/// <param name="animation"></param>
		/// <param name="animationTime"></param>
		static void ApplyAnimation(Skeleton& skeleton, const AnimationData& animation, float animationTime);

		void SkeletonUpdate(Skeleton& skeleton);

		void SkinClusterUpdate(SkinCluster& skinCluster, const Skeleton& skeleton);
	};
}
