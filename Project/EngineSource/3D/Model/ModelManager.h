#pragma once
#include <unordered_map>
#include "ModelLoader.h"
#include "Model.h"

namespace GameEngine {

	class ModelManager final {
	public:

		// 登録するデータ
		struct ModelEntryData {
			std::string name; // ロードしたモデルファイル名(.obj,gltfなど)
			std::unique_ptr<Model> model; // モデルデータ
		};

	public:

		ModelManager() = default;
		~ModelManager();

		void Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, TextureManager* textureManager, SrvManager* srvManager);

		/// <summary>
		/// モデルデータを登録
		/// </summary>
		/// <param name="modelFile">モデルファイル名 : 登録名にもなる</param>
		/// <param name="modelName">.obj名</param>
		void RegisterModel(const std::string& modelFile,const std::string& objFileName);

		/// <summary>
		/// モデルデータを登録
		/// </summary>
		/// <param name="modelName">登録したいモデル名</param>
		/// <param name="model">モデルデータ</param>
		void RegisterModel(const std::string& modelName, std::unique_ptr<Model> model);

		/// <summary>
		/// 
		/// 辻褄合わせで作ったので後で削除するように
		/// 
		/// </summary>
		/// <param name="modelName"></param>
		void RegisterGridPlaneModel(const std::string& modelName,const Vector2& size);

		/// <summary>
		/// 登録を外す
		/// </summary>
		/// <param name="handle"></param>
		void UnregisterModel(uint32_t handle);

		/// <summary>
		/// モデルの名前からハンドルを取得
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		uint32_t GetHandleByName(const std::string& name) const;

		/// <summary>
		/// ハンドルからモデルの名前を取得
		/// </summary>
		/// <param name="handle"></param>
		/// <returns></returns>
		std::string GetNameByHandle(uint32_t handle) const;

		/// <summary>
		/// ハンドルからモデルを取得
		/// </summary>
		/// <param name="handle"></param>
		/// <returns></returns>
		[[nodiscard]]
		Model* GetHandleByModel(uint32_t handle) const;

		/// <summary>
		/// 名前からモデルを取得
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		[[nodiscard]]
		Model* GetNameByModel(const std::string& name) const;

		/// <summary>
		/// リソースファイルにあるモデルデータを全て取得する
		/// </summary>
		void LoadAllModel();

	private:
		ModelManager(ModelManager&) = delete;
		ModelManager& operator=(ModelManager&) = delete;

		// モデル生成
		ModelLoader loader_;

		// ハンドルからモデルデータを保存
		std::unordered_map<uint32_t, ModelEntryData> models_;
		// モデルデータの名前からハンドルを保存
		std::unordered_map<std::string, uint32_t> nameToHandles_;

		// 次のハンドル
		uint32_t nextHandle_ = 1;
	};
}