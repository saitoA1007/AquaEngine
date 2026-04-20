#pragma once
#include <vector>
#include <optional>
#include <unordered_map>

#include "Mesh.h"
#include "Material.h"
#include "TransformationMatrix.h"
#include "AnimationData.h"
#include "BLAS.h"

namespace GameEngine {
	
	class Model final {
	public:
		Model() = default;
		~Model() = default;

		// メッシュを追加
		void AddMesh(std::unique_ptr<Mesh> mesh) {
			meshes_.push_back(std::move(mesh));
		}

		// マテリアルを追加
		void AddMaterial(const std::string& name, std::unique_ptr<Material> material) {
			materials_[name] = std::move(material);
		}

		// 作成したMeshを元にBLASを作成する
		void AddBLAS(ID3D12GraphicsCommandList4* cmdList) {
			for (auto& mesh : meshes_) {
				std::unique_ptr<BLAS> blas = std::make_unique<BLAS>();
				blas->Create(cmdList,mesh->GetVertexBufferView(),mesh->GetIndexBufferView(),mesh->GetTotalVertices(),mesh->GetTotalIndices());
				blasList_.push_back(std::move(blas));
			}
		}

		// 外部読み込み用のデータを設定
		void SetLoadModelData(const std::string& modelName, const Matrix4x4& localMatrix) {
			isLoad_ = true;
			modelName_ = modelName;
			localMatrix_ = localMatrix;
		}

		// ボーンデータを追加
		void SetSkeletonData(Skeleton skeletonBone, SkinCluster skinClusterBone) {
			isSkeleton_ = true;
			skeletonBone_ = skeletonBone;
			skinClusterBone_ = skinClusterBone;
		}

	public:

		/// <summary>
		/// デフォルトの色を設定
		/// </summary>
		/// <param name="color"></param>
		void SetDefaultColor(const Vector4& color,const std::string& materialName = "default");

		/// <summary>
		/// 鏡面反射の色を設定
		/// </summary>
		/// <param name="specularColor"></param>
		void SetDefaultSpecularColor(const Vector3& specularColor, const std::string& materialName = "default");

		/// <summary>
		/// 輝度の設定
		/// </summary>
		/// <param name="shininess"></param>
		void SetDefaultShininess(const float& shininess, const std::string& materialName = "default");

		/// <summary>
		/// デフォオルトの光源の有無を設定
		/// </summary>
		/// <param name="isEnableLight"></param>
		void SetDefaultIsEnableLight(const bool& isEnableLight, const std::string& materialName = "default");

		/// <summary>
		/// 影の適応の有無を設定
		/// </summary>
		/// <param name="isEnableLight"></param>
		/// <param name="materialName"></param>
		void SetDefaultIsEnableShadow(const bool& isEnableShadow, const std::string& materialName = "default");

		/// <summary>
		/// デフォルトのuvMatrixを設定
		/// </summary>
		/// <param name="uvMatrix"></param>
		void SetDefaultUVMatrix(const Matrix4x4& uvMatrix, const std::string& materialName = "default");

		/// <summary>
		/// デフォルトのuvMatrixを設定
		/// </summary>
		/// <param name="uvTransform"></param>
		/// <param name="index"></param>
		void SetDefaultUVMatrix(const Transform& uvTransform, const std::string& materialName = "default");

		/// <summary>
		/// /デフォルトのテクスチャを設定
		/// </summary>
		/// <param name="handle"></param>
		/// <param name="materialName"></param>
		void SetDefaultTextureHandle(const uint32_t& handle, const std::string& materialName = "default");

		/// <summary>
		/// モデルの名前を取得
		/// </summary>
		/// <returns></returns>
		const std::string GetModelName() const { return modelName_; }

		const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const { return meshes_; }
		Material* GetMaterial(const std::string& name) const;

		// ローカル行列
		Matrix4x4 GetLocalMatrix() const {return localMatrix_;}

		// ロードしているか
		const bool IsLoad() const { return isLoad_; }
		// ボーンが存在しているか
		const bool IsSkeleton() const { return isSkeleton_; }

		// ボーンデータ
		Skeleton* GetSkeleton() { return &skeletonBone_.value(); }
		SkinCluster* GetSkinCluster() { return &skinClusterBone_.value(); }
		const SkinCluster* GetSkinClusterData() const { return &skinClusterBone_.value(); }

		// BLASを取得
		const std::vector<std::unique_ptr<BLAS>>& GetBLASList() const { return blasList_; }

	private:
		Model(Model&) = delete;
		Model& operator=(Model&) = delete;

		// 複数メッシュに対応
		std::vector<std::unique_ptr<Mesh>> meshes_;

		// 複数マテリアルに対応
		std::unordered_map<std::string, std::unique_ptr<Material>> materials_;

		// ボーンデータ
		std::optional<Skeleton> skeletonBone_ = std::nullopt;
		std::optional<SkinCluster> skinClusterBone_ = std::nullopt;

		// BLAS
		std::vector<std::unique_ptr<BLAS>> blasList_;

		// Nodeのローカル行列を保持しておく変数
		Matrix4x4 localMatrix_;
		// 外部からロードされたか
		bool isLoad_ = false;
		bool isSkeleton_ = false;
		// モデルの名前
		std::string modelName_ = "NoName";
	};
}