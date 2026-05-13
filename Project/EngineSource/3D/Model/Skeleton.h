#pragma once
#include "VertexData.h"
#include "AnimationData.h"
#include "ConstantBuffer.h"

namespace GameEngine {

	// スキニング情報
	struct SkinningInformation
	{
		uint32_t numVertices; // 処理する頂点数
	};

	class Skeleton {
	public:
		
		/// <summary>
		/// ボーンデータを作成
		/// </summary>
		/// <param name="modelData"></param>
		void Create(ID3D12GraphicsCommandList4* cmdList, const SkeletonData& skeletonData,const ModelData& modelData);

		SkeletonData* GetSkeletonData() { return &skeletonData_; }
		SkinCluster* GetSkinCluster() { return &skinCluster_; }
		const SkinCluster* GetSkinClusterData() const { return &skinCluster_; }
		VertexBuffer<VertexData>* GetOutputVertexBuffer() { return &outputvertexBuffer_; }
		ConstantBuffer<SkinningInformation>* GetConstantBuffer() { return &constBuffer_; }
		
		const uint32_t& GetVerticesNum() const { return verticesNum_; }

	private:
		uint32_t verticesNum_ = 0;

		SkeletonData skeletonData_;
		SkinCluster skinCluster_;

		// アウトプット用の頂点リソース
		VertexBuffer<VertexData> outputvertexBuffer_;

		// スキニング情報
		ConstantBuffer<SkinningInformation> constBuffer_;
	};
}