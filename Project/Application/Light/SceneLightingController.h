#pragma once
#include"LightManager.h"
#include"DirectionalLight.h"

/// <summary>
/// シーンのライトを管理する
/// </summary>
class SceneLightingController {
public:

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="device"></param>
	void Initialize(ID3D12Device* device);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// ライトのリソースを取得
	/// </summary>
	/// <returns></returns>
	ID3D12Resource* GetResource() const {return lightManager_->GetResource();}

private:

	// ライト
	std::unique_ptr<GameEngine::LightManager> lightManager_;
	// 平行光源
	GameEngine::DirectionalLight::DirectionalLightData directionalData_;
	// 点光源
	//GameEngine::PointLight::PointLightData pointLightData_;
	// スポットライト
	//GameEngine::SpotLight::SpotLightData spotLightData_;

private:

	/// <summary>
	/// 値を登録する
	/// </summary>
	void RegisterBebugParam();

	/// <summary>
	/// 値を適応する
	/// </summary>
	void ApplyDebugParam();
};