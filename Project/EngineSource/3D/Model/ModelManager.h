#pragma once
#include <unordered_map>
#include"Model.h"

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

		/// <summary>
		/// モデルデータを登録
		/// </summary>
		/// <param name="modelFile">モデルファイル名 : 登録名にもなる</param>
		/// <param name="modelName">.obj名</param>
		void RegisterMode(const std::string& modelFile,const std::string& objFileName);

		/// <summary>
		/// モデルデータを登録
		/// </summary>
		/// <param name="modelName">登録したいモデル名</param>
		/// <param name="model">モデルデータ</param>
		void RegisterMode(const std::string& modelName, std::unique_ptr<Model> model);

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

		// ハンドルからモデルデータを保存
		std::unordered_map<uint32_t, ModelEntryData> models_;
		// モデルデータの名前からハンドルを保存
		std::unordered_map<std::string, uint32_t> nameToHandles_;

		// 次のハンドル
		uint32_t nextHandle_ = 1;
	};
}