#include "Animator.h"
#include <cassert>
#include <algorithm>
#include "MyMath.h"
#include "EasingManager.h"
#include "FPSCounter.h"
#include "Model.h"
#include "PSOManager.h"
using namespace GameEngine;

ID3D12GraphicsCommandList4* Animator::commandList_ = nullptr;
ID3D12RootSignature* Animator::rootSignature_ = nullptr;
ID3D12PipelineState* Animator::pipelineState_ = nullptr;

void Animator::StaticInitialize(ID3D12GraphicsCommandList4* commandList, PSOManager* psoManager) {
	commandList_ = commandList;

	auto psoData = psoManager->GetDrawPsoData("ComputeAnimation");
	pipelineState_ = psoData.graphicsPipelineState;
	rootSignature_ = psoData.rootSignature;
}

void Animator::Initialize(Model* model, const AnimationData* animationData) {
	SetAnimationData(animationData);
	SetModelData(model);
}

void Animator::Update() {

	timer_ += FpsCounter::deltaTime;

	if (isLoop_) {
		timer_ = std::fmodf(timer_, animationData_->duration);
	} else {
		timer_ = (std::min)(timer_, animationData_->duration);
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

void Animator::ComputeUpdate() {
	timer_ += FpsCounter::deltaTime;

	if (isLoop_) {
		timer_ = std::fmodf(timer_, animationData_->duration);
	} else {
		timer_ = (std::min)(timer_, animationData_->duration);
	}

	// アニメーションの更新処理
	Update(timer_);

	// コンピュートシェーダーで頂点を更新
	UpdateCompute();
}

void Animator::ComputeUpdate(const float& time) {
	// アニメーションの更新をおこない、骨ごとのLocal情報を更新する
	ApplyAnimation(*skeleton_, *animationData_, time);

	// 現在の骨ごとのLocal情報を基にSkeletonSpaceの情報を更新する
	SkeletonUpdate(*skeleton_);

	// SkeletonSpaceの情報を基に、SkinClusterのMatrixPaletteを更新する
	SkinClusterUpdate(*skinCluster_, *skeleton_);

	// コンピュートシェーダーで頂点を更新
	UpdateCompute();
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

void Animator::ApplyAnimation(SkeletonData& skeleton, const AnimationData& animation, float animationTime) {
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

void Animator::SkeletonUpdate(SkeletonData& skeleton) {
	// すべてのJointを更新。
	for (Joint& joint : skeleton.joints) {
		joint.localMatrix = Math::MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) {
			joint.skeletonSpaceMatrix = joint.localMatrix * skeleton.joints[*joint.parent].skeletonSpaceMatrix;
		} else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

void Animator::SkinClusterUpdate(SkinCluster& skinCluster, const SkeletonData& skeleton) {
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix = skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix = Math::InverseTranspose(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix);
	}
}

void Animator::SetModelData(Model* model) {
	if (model->IsSkeleton()) {
		model_ = model;
		auto* skeleton = model->GetSkeleton();
		skinCluster_ = skeleton->GetSkinCluster();
		skeleton_ = skeleton->GetSkeletonData();
	} else {
		assert(false && "Not found Model Bone");
	}
}

void Animator::UpdateCompute() {

	commandList_->SetComputeRootSignature(rootSignature_);
	commandList_->SetPipelineState(pipelineState_);

	auto* skeleton = model_->GetSkeleton();
	auto* outputBuffer = skeleton->GetOutputVertexBuffer();
	const auto& meshes = model_->GetMeshes();
	const auto& mesh = meshes[0];

	// 頂点を書き込み状態に変更
	outputBuffer->TransitionUAV(commandList_);
	
	commandList_->SetComputeRootDescriptorTable(0, skinCluster_->wellBuffer.GetSrvHandleGPU());
	commandList_->SetComputeRootDescriptorTable(1, mesh->GetVertexBuffer().GetSrvGpuHandle());
	commandList_->SetComputeRootDescriptorTable(2, skinCluster_->influenceBuffer.GetSrvGpuHandle());
	commandList_->SetComputeRootDescriptorTable(3, outputBuffer->GetUAVGpuHandle());
	commandList_->SetComputeRootConstantBufferView(4, skeleton->GetConstantBuffer()->GetGpuVirtualAddress());

	commandList_->Dispatch(UINT(skeleton->GetVerticesNum() + 1023) / 1024, 1, 1);

	// 頂点を読み取り状態に変更
	outputBuffer->TransitionSRV(commandList_);

	// BLASを更新する
	mesh->GetBLAS()->Update(commandList_, outputBuffer->GetView());
}