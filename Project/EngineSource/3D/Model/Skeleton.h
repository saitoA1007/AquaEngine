#pragma once
#include "VertexData.h"
#include "AnimationData.h"
#include <optional>

namespace GameEngine {

	class Skeleton {
	public:
		
		/// <summary>
		/// ボーンデータを作成
		/// </summary>
		/// <param name="modelData"></param>
		void Create(const SkeletonData& skeletonData,const ModelData& modelData);

		SkeletonData* GetSkeletonData() { return &skeletonData_; }
		SkinCluster* GetSkinCluster() { return &skinCluster_; }
		const SkinCluster* GetSkinClusterData() const { return &skinCluster_; }

	private:
		SkeletonData skeletonData_;
		SkinCluster skinCluster_;
	};
}