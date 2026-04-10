#pragma once
#include"Vector4.h"
#include"Vector3.h"
#include"Vector2.h"
#include"Matrix4x4.h"
#include"MyMath.h"
#include"VertexData.h"

#include<vector>
#include<map>
#include<iostream>

#include <d3d12.h>
#include <wrl.h>

template <typename tValue>
struct Keyframe {
	float time;  // キーフレームの時刻
	tValue value;  // キーフレームの値
};

using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

struct NodeAnimation {
	std::vector<KeyframeVector3> translate;
	std::vector<KeyframeQuaternion> rotate;
	std::vector<KeyframeVector3> scale;
};

// アニメーションのデータの持ち方を変えた方が良いかも
// NodeAnimationの方に float durationを持たせた方がよい。そしてtimerはけしてもいいかも
struct AnimationData {
	float duration;  // アニメーション全体の尺
	float timer = 0.0f; // 使わないので後で消す
	// NodeAnimationの集合体。Node名でひらけるようにしておく
	std::map<std::string, NodeAnimation> nodeAnimations;
};

//======================================================-

// 影響を受けるJointの数
const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence {
	std::array<float, kNumMaxInfluence> weights;
	std::array<int32_t, kNumMaxInfluence> jointIndices;
};

struct WellForGPU {
	Matrix4x4 skeletonSpaceMatrix; // 位置用
	Matrix4x4 skeletonSpaceInverseTransposeMatrix; // 法線用
};

struct SkinCluster {
	std::vector<Matrix4x4> inverseBindPoseMatrices;

	Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
	std::span<VertexInfluence> mappedInfluence;

	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};