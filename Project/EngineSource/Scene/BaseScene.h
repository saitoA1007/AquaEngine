#pragma once
#include"SceneContext.h"
#include"ITransitionEffect.h"
#include <string>

/// <summary>
/// 各シーンの元
/// </summary>
class BaseScene {
public:

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~BaseScene() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	virtual void Initialize(SceneContext* context) = 0;

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update() = 0;

	/// <summary>
	/// デバック時、処理して良いものを更新する
	/// </summary>
	virtual void DebugUpdate() = 0;

	/// <summary>
	/// 描画処理
	/// </summary>
	virtual void Draw(const bool& isDebugView) = 0;

	/// <summary>
	/// 終了したことを伝える
	/// </summary>
	/// <returns></returns>
	virtual bool IsFinished() = 0;

	/// <summary>
	/// 次のシーン遷移する場面の名前を取得
	/// </summary>
	/// <returns>次のシーン名</returns>
	virtual std::string NextSceneName() = 0;

	// 遷移クラス
	virtual std::unique_ptr<ITransitionEffect> GetTransitionEffect() = 0;

protected:
	// エンジン機能
	SceneContext* context_ = nullptr;
};