#pragma once
#include "AnimationData.h"
#include "TextureManager.h"
#include "SrvManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GameEngine {

    class Model;

	class ModelLoader final {
	public:
		ModelLoader() = default;
		~ModelLoader() = default;

        // 初期化処理
		void Initialize(ID3D12Device* device, TextureManager* textureManager, SrvManager* srvManager);

    public: // 生成処理

        /// <summary>
        /// OBJファイルからモデルを生成
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreateModel(const std::string& objFilename, const std::string& filename);

        /// <summary>
        /// 球モデルを生成
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreateSphere(uint32_t subdivision);

        /// <summary>
        /// 平面モデルを生成
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreatePlane(const Vector2& size);

        /// <summary>
        /// グリッド平面モデルを生成
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreateGridPlane(const Vector2& size);

        /// <summary>
        /// アニメーションデータを読み込み
        /// </summary>
        [[nodiscard]]
        AnimationData LoadAnimationFile(const std::string& objFilename,
            const std::string& filename);

        /// <summary>
        /// アニメーションデータを読み込む
        /// </summary>
        /// <param name="objFilename"></param>
        /// <param name="filename"></param>
        /// <returns></returns>
        [[nodiscard]]
        static std::map<std::string, AnimationData> LoadAnimationsFile(const std::string& objFilename, const std::string& filename);

        /// <summary>
        /// スケルトンを生成
        /// </summary>
        [[nodiscard]]
        Skeleton CreateSkeleton(const Node& rootNode);

    private:
        ModelLoader(const ModelLoader&) = delete;
        ModelLoader& operator=(const ModelLoader&) = delete;

        ID3D12Device* device_ = nullptr;
        TextureManager* textureManager_ = nullptr;
        SrvManager* srvManager_ = nullptr;

        static inline const std::string kDirectoryPath_ = "Resources/Models";

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
        int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

        [[nodiscard]]
        SkinCluster CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData);
	};
}