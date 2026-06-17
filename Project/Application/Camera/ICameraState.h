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
	Vector3 idealPosition;
	Vector3 idealTarget;
};

class ICameraState {
public:
	virtual ~ICameraState() = default;

	virtual void Enter() = 0;
	virtual void Update() = 0;
	virtual void Exit() = 0;

protected:

	CameraCommonData commonData_;
};