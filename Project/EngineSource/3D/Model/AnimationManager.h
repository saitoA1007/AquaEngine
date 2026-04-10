#pragma once
#include<map>
#include <unordered_map>
#include"Model.h"

#include"AnimationData.h"

namespace GameEngine {

	class AnimationManager final {
	public:
		AnimationManager() = default;
		~AnimationManager();

		/// <summary>
		/// アニメーションデータを登録する
		/// </summary>
		/// <param name="filename"></param>
		/// <param name="objFilename"></param>
		void RegisterAnimation(const std::string& filename,const std::string& objFilename);

		/// <summary>
		/// 登録を外す
		/// </summary>
		/// <param name="handle"></param>
		void UnregisterAnimation(const std::string& name);

		/// <summary>
		/// 名前からアニメーションのデータを取得
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		[[nodiscard]]
		const AnimationData& GetNameByAnimation(const std::string& name) const;

		/// <summary>
		/// 名前からアニメーションデータ達を取得する
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		[[nodiscard]]
		const std::map<std::string, AnimationData>& GetNameByAnimations(const std::string& name) const;

		/// <summary>
		/// リソースファイルにあるモデルデータを全て取得する
		/// </summary>
		void LoadAllModel();

	private:
		AnimationManager(AnimationManager&) = delete;
		AnimationManager& operator=(AnimationManager&) = delete;

		// アニメーションのデータリスト
		// <アニメーションの名前> -> <アニメーションデータの名前、アニメーションデータ>
		std::unordered_map<std::string,std::map<std::string, AnimationData>> animations_;
	};
}