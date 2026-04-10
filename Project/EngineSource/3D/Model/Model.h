#pragma once
#include <d3d12.h>
#include<vector>
#include <optional>
#include <unordered_map>
#include <wrl.h>

#include"VertexData.h"
#include"Mesh.h"
#include"Material.h"
#include"WorldTransform.h"
#include"WorldTransforms.h"
#include"AnimationData.h"

#include"SrvManager.h"
#include"TextureManager.h"

#include"LightManager.h"
#include"Camera.h"

#include"TransformationMatrix.h"

#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>

namespace GameEngine {
	
	class Model final {
	public:
		Model() = default;
		~Model() = default;

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="device"></param>
		/// <param name="commandList"></param>
		static void StaticInitialize(ID3D12Device* device, TextureManager* textureManager, SrvManager* srvManager);

		/// <summary>
		/// OBJファイルからメッシュ生成
		/// </summary>
		/// <param name="objFilename">.objファイル名</param>
		/// <param name="filename">格納ファイル名</param>
		/// <returns></returns>
		[[nodiscard]]
		static std::unique_ptr<Model> CreateModel(const std::string& objFilename, const std::string& filename);

		/// <summary>
		/// 球モデル生成
		/// </summary>
		/// <returns>生成されたモデル</returns>
		[[nodiscard]]
		static std::unique_ptr<Model> CreateSphere(uint32_t subdivision);

		/// <summary>
		/// 三角形の平面を生成
		/// </summary>
		/// <returns></returns>
		[[nodiscard]]
		static std::unique_ptr<Model> CreateTrianglePlane();

		/// <summary>
		/// 平面モデルを生成
		/// </summary>
		/// <param name="size"></param>
		/// <returns></returns>
		[[nodiscard]]
		static std::unique_ptr<Model> CreatePlane(const Vector2& size);

		/// <summary>
		/// グリッドを描画
		/// </summary>
		/// <param name="size">グリッドサイズ</param>
		/// <returns></returns>
		[[nodiscard]]
		static std::unique_ptr<Model> CreateGridPlane(const Vector2& size);

		/// <summary>
		/// アニメーションデータを読み込み
		/// </summary>
		/// <param name="directoryPath"></param>
		/// <param name="objFilename"></param>
		/// <param name="filename"></param>
		/// <returns></returns>
		[[nodiscard]]
		static AnimationData LoadAnimationFile(const std::string& objFilename, const std::string& filename);

		[[nodiscard]]
		static std::map<std::string, AnimationData> LoadAnimationsFile(const std::string& objFilename, const std::string& filename);

		/// <summary>
		/// スケルトンを作成する
		/// </summary>
		/// <param name="rootNode"></param>
		/// <returns></returns>
		[[nodiscard]]
		static Skeleton CreateSkeleton(const Node& rootNode);

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
		void SetDefaultShiness(const float& shininess, const std::string& materialName = "default");

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

	public:

		ModelData modelData_;
		Node node_;

		// アニメーションデータ
		std::optional<Skeleton> skeletonBron_ = std::nullopt;
		std::optional<SkinCluster> skinClusterBron_ = std::nullopt;

	private:
		Model(Model&) = delete;
		Model& operator=(Model&) = delete;

		// デバイス
		static ID3D12Device* device_;

		// テクスチャ
		static TextureManager* textureManager_;
		// srvの管理
		static SrvManager* srvManager_;

		// ファイル名
		static inline const std::string kDirectoryPath_ = "Resources/Models";

		// 複数メッシュに対応
		std::vector<std::unique_ptr<Mesh>> meshes_;

		// 複数マテリアルに対応
		std::unordered_map<std::string, std::unique_ptr<Material>> materials_;

		// Nodeのローカル行列を保持しておく変数
		Matrix4x4 localMatrix_;

		bool isLoad_ = false;

		// モデルの名前
		std::string modelName_;

	private:

		/// <summary>
		/// モデルデータのファイル読み込み
		/// </summary>
		/// <param name="directoryPath"></param>
		/// <param name="objFilename"></param>
		/// <param name="filename"></param>
		/// <returns></returns>
		[[nodiscard]]
		ModelData LoadModelFile(const std::string& directoryPath, const std::string& objFilename, const std::string& filename);

		/// <summary>
		/// Node情報を取得
		/// </summary>
		/// <param name="node"></param>
		/// <returns></returns>
		[[nodiscard]]
		Node ReadNode(aiNode* node);

		/// <summary>
		/// ジョイントを作成する
		/// </summary>
		/// <param name="node"></param>
		/// <param name="parent"></param>
		/// <param name="joints"></param>
		/// <returns></returns>
		[[nodiscard]]
		static int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

		[[nodiscard]]
		static SkinCluster CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData);
	};
}