#include "Skeleton.h"
#include <cassert>
#include <algorithm>
#include "MyMath.h"
using namespace GameEngine;

void Skeleton::Create(ID3D12GraphicsCommandList4* cmdList, const SkeletonData& skeletonData,const ModelData& modelData) {
	assert(!skeletonData.joints.empty() && "Skeleton joints are empty!");
	assert(!modelData.meshes.empty() && "Model has no meshes!");
	assert(!modelData.meshes[0].vertices.empty() && "Model mesh[0] has no vertices!");

	// アウトプット用の頂点リソースを作成
	outputvertexBuffer_.Create(cmdList, modelData.meshes[0].vertices);

	// 頂点数を取得
	verticesNum_ = static_cast<uint32_t>(modelData.meshes[0].vertices.size());

	// 定数バッファを生成
	constBuffer_.Create();
	// 頂点数を設定
	auto* data = constBuffer_.GetData();
	data->numVertices = static_cast<uint32_t>(modelData.meshes[0].vertices.size());

	// スケルトンデータを取得
	skeletonData_ = skeletonData;

	// palette用のリソースを作成
	skinCluster_.wellBuffer.Create(static_cast<uint32_t>(skeletonData_.joints.size()));
	// spanを使ってアクセスするようにする
	auto* mappedPalette = skinCluster_.wellBuffer.GetData();
	skinCluster_.mappedPalette = { mappedPalette,skeletonData_.joints.size() };

	std::vector<VertexInfluence> influence;
	influence.resize(modelData.meshes[0].vertices.size());
	// 0埋め
	for (auto& inf : influence) {
		inf.weights.fill(0.0f);
	}
	// influence用リソース作成
	skinCluster_.influenceBuffer.Create(influence);
	// spanを使ってアクセスするようにする
	VertexInfluence* mappedInfluence = skinCluster_.influenceBuffer.GetVertexData();
	skinCluster_.mappedInfluence = { mappedInfluence,modelData.meshes[0].vertices.size() };

	// inverseBindPoseMatrixを格納する場所を作成して、単位行列で埋める
	skinCluster_.inverseBindPoseMatrices.resize(skeletonData_.joints.size());
	std::generate(skinCluster_.inverseBindPoseMatrices.begin(), skinCluster_.inverseBindPoseMatrices.end(), Matrix4x4::MakeIdentity);

	// ModelのSkinClusterの情報を解析
	for (const auto& jointWeight : modelData.skinClusterData) {
		auto it = skeletonData_.jointMap.find(jointWeight.first); // jointWeight.firstはjoint名なので、skeletonに対象となるjointが含まれているか判断
		if (it == skeletonData_.jointMap.end()) { // 存在しない場合は次に回す
			continue;
		}
		// (*it).secondにはjointのindexが入っているので、該当のindexのinverseBindPoseMatrixを代入
		skinCluster_.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
			auto& currentInfluence = skinCluster_.mappedInfluence[vertexWeight.vertexIndex]; // 該当のvertexIndexのinfluence情報を参照しておく
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index) { // 空いているところに入れる
				if (currentInfluence.weights[index] == 0.0f) { // weight==0が空いている状態なので、その場所にweightとjointのindexを代入
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}

}