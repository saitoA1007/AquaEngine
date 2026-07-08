#pragma once
#include "Vector3.h"

// 前方宣言
class CameraController;

enum class CameraState {
	kLockOn,
	kFollow,

	kMaxCount
};

struct CameraCommonData {
	Vector3 idealPosition = {};
	Vector3 idealTarget = {};

	float targetFov = 0.45f;

	// 注視点の追従
	float targetLerpRate = 0.1f;
	// 位置の追従
	float positionLerpRate = 0.07f;
	// Fovの補間
	float fovLerpRate = 0.05f;
};

class ICameraState {
public:
	ICameraState(CameraController* controller) : controller_(controller) {}
	virtual ~ICameraState() = default;

	virtual void Enter() {}
	virtual void Update(float dt60) = 0;
	virtual void Exit() {}

	virtual CameraState GetNextState() const { return CameraState::kMaxCount; }

	const CameraCommonData& GetCommonData() const { return commonData_; }

protected:
	CameraController* controller_ = nullptr;
	CameraCommonData commonData_;
};