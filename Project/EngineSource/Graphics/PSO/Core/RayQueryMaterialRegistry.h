#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace GameEngine {

	/// <summary>
	/// RayQueryで使用するマテリアルのIDを管理する
	/// マテリアルIDはTLASのInstanceContributionToHitGroupIndexとして渡し、
	/// シェーダー側のMaterialDispatch.hlsliでマテリアル関数を呼び分ける
	/// </summary>
	class RayQueryMaterialRegistry {
	public:

		// 組み込みのマテリアルID。シェーダー側のMATERIAL_ID_*と合わせる
		static constexpr uint32_t kDefaultMaterialId = 0;
		static constexpr uint32_t kIceMaterialId = 1;
		// マテリアルグラフから生成したマテリアルの開始ID
		static constexpr uint32_t kFirstGeneratedMaterialId = 2;

	public:

		static RayQueryMaterialRegistry& GetInstance() {
			static RayQueryMaterialRegistry instance;
			return instance;
		}

		/// <summary>
		/// 登録済みのマテリアルをファイルから読み込む
		/// </summary>
		void Load();

		/// <summary>
		/// マテリアルグラフから生成したマテリアルを登録し、MaterialDispatch.hlsliを書き出す
		/// </summary>
		/// <param name="identifier">HLSLの識別子として使えるマテリアル名</param>
		/// <returns>マテリアルID</returns>
		uint32_t RegisterMaterial(const std::string& identifier);

		/// <summary>
		/// マテリアル名からIDを取得する。見つからなければデフォルトマテリアルのIDを返す
		/// ModelComponent::SetHitGroupに渡して使用する
		/// </summary>
		uint32_t GetMaterialId(const std::string& identifier) const;

		/// <summary>
		/// マテリアルの登録内容が変わるたびに増える。パイプラインの再生成判定に使う
		/// </summary>
		uint32_t GetVersion() const { return version_; }

	private:
		RayQueryMaterialRegistry() = default;
		~RayQueryMaterialRegistry() = default;
		RayQueryMaterialRegistry(const RayQueryMaterialRegistry&) = delete;
		RayQueryMaterialRegistry& operator=(const RayQueryMaterialRegistry&) = delete;

		struct Entry {
			std::string identifier;
			uint32_t id;
		};

		// 登録済みのマテリアル
		std::vector<Entry> entries_;

		uint32_t version_ = 0;

		// 登録内容の保存先
		const std::string kRegistryPath_ = "Resources/Shaders/RayQuery/Materials/Generated/MaterialRegistry.json";
		// マテリアル分岐の書き出し先
		const std::string kDispatchPath_ = "Resources/Shaders/RayQuery/MaterialDispatch.hlsli";

	private:

		// 登録内容を保存する
		void Save() const;

		// マテリアル分岐のhlslを書き出す
		void WriteDispatch() const;
	};
}
