#include"AnimationManager.h"

#include"Model.h"
using namespace GameEngine;

AnimationManager::~AnimationManager() {
	animations_.clear();
}

void AnimationManager::RegisterAnimation(const std::string& filename, const std::string& objFilename) {
	
	// 同名のアニメーションデータが登録されている場合は早期リターン
	auto getName = animations_.find(filename);
	if (getName != animations_.end()) {
		return;
	}

	// 指定したモデルに存在するアニメーションデータをを全て取得する
	animations_[filename] = Model::LoadAnimationsFile(objFilename, filename);
}

void AnimationManager::UnregisterAnimation(const std::string& name) {
	auto getAnimation = animations_.find(name);
	if (getAnimation == animations_.end()) {
		return;
	}

	// 登録したアニメーションデータを削除
	animations_.erase(getAnimation);
}

[[nodiscard]]
const AnimationData& AnimationManager::GetNameByAnimation(const std::string& name) const {
	 auto getAnimations = animations_.find(name);

	 // 見つからなかった場合のエラーハンドリング
	 if (getAnimations == animations_.end()) {
		 assert(0 && "Animation File Not Found");
	 }

	 const auto& animationMap = getAnimations->second;

	 // アニメーションデータが空でないか確認
	 if (animationMap.empty()) {
		 assert(0 && "Animation Data is Empty");
	 }

	 return animationMap.begin()->second;
}

[[nodiscard]]
const std::map<std::string, AnimationData>& AnimationManager::GetNameByAnimations(const std::string& name) const {
	auto getAnimations = animations_.find(name);
	if (getAnimations == animations_.end()) {
		assert(0);
	}

	// アニメーションデータ達を返す
	return getAnimations->second;
}

void AnimationManager::LoadAllModel() {

}