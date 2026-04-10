#include"Animator.h"

#include"MyMath.h"
#include<cassert>
#include"EasingManager.h"
#include"FPSCounter.h"
using namespace GameEngine;

void Animator::Initialize(Model* model, const AnimationData* animationData) {
	SetAnimationData(animationData);
	SetModelData(model);
}

void Animator::Update() {

	timer_ += FpsCounter::deltaTime;

	if (isLoop_) {
		timer_ = std::fmodf(timer_, animationData_->duration);
	} else {
		timer_ = std::min(timer_, animationData_->duration);
	}

	// アニメーションの更新処理
	Update(timer_);
}

void Animator::Update(const float& time) {
	// アニメーションの更新をおこない、骨ごとのLocal情報を更新する
	ApplyAnimation(*skeleton_, *animationData_, time);

	// 現在の骨ごとのLocal情報を基にSkeletonSpaceの情報を更新する
	SkeletonUpdate(*skeleton_);

	// SkeletonSpaceの情報を基に、SkinClusterのMatrixPaletteを更新する
	SkinClusterUpdate(*skinCluster_, *skeleton_);
}

Vector3 Animator::CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time) {

	assert(!keyframes.empty()); // キーがないものはエラーを返す

	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;

		// indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			// 範囲内を補間する
			float t = ((time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time));
			return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	// 一番後の時刻より後ろなので最後の値を返す
	return (*keyframes.rbegin()).value;
}

Quaternion Animator::CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time) {

	assert(!keyframes.empty()); // キーがないものはエラーを返す

	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;

		// indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			// 範囲内を補間する
			float t = ((time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time));
			return Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	// 一番後の時刻より後ろなので最後の値を返す
	return (*keyframes.rbegin()).value;
}

void Animator::ApplyAnimation(Skeleton& skeleton, const AnimationData& animation, float animationTime) {
	for (Joint& joint : skeleton.joints) {
		// 対象のJointのAnimationがあれば、値の適応を行う。
		if (auto it = animation.nodeAnimations.find(joint.name); it != animation.nodeAnimations.end()) {
			const NodeAnimation& rootNodeAnimation = (*it).second;
			joint.transform.translate = CalculateValue(rootNodeAnimation.translate, animationTime);
			joint.transform.rotate = CalculateValue(rootNodeAnimation.rotate, animationTime);
			joint.transform.scale = CalculateValue(rootNodeAnimation.scale, animationTime);
		}
	}
}

void Animator::SkeletonUpdate(Skeleton& skeleton) {
	// すべてのJointを更新。
	for (Joint& joint : skeleton.joints) {
		joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) {
			joint.skeletonSpaceMatrix = joint.localMatrix * skeleton.joints[*joint.parent].skeletonSpaceMatrix;
		} else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

void Animator::SkinClusterUpdate(SkinCluster& skinCluster, const Skeleton& skeleton) {
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix = skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix = InverseTranspose(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix);
	}
}

void Animator::SetModelData(Model* model) {
	if (model->skinClusterBron_.has_value()) {
		skinCluster_ = &model->skinClusterBron_.value();
		skeleton_ = &model->skeletonBron_.value();
	} else {
		assert(0);
	}
}