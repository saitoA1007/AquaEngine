#pragma once
#include"Camera.h"
#include"InputCommand.h"

class CameraController {
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="inputCommand"></param>
	/// <param name="targetPos"></param>
	void Update(GameEngine::InputCommand* inputCommand,const Vector3& targetPos);

	/// <summary>
	/// カメラデータ
	/// </summary>
	/// <returns></returns>
	GameEngine::Camera& GetCamera() const { return *camera_.get(); }

private:

	// カメラ
	std::unique_ptr<GameEngine::Camera> camera_;
	Vector3 position_ = { 0.0f,4.0f,-10.0f };

	// 距離
	static inline const float kDistance_ = 40.0f;

	// 回転の移動量
	Vector2 rotateMove_ = { 3.1f,1.0f };

private:

	/// <summary>
	/// カメラをターゲットの方向に向かせる
	/// </summary>
	/// <param name="eye">カメラの位置</param>
	/// <param name="center">ターゲットの位置</param>
	/// <param name="up">向き</param>
	/// <returns></returns>
	Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
};