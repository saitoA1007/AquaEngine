#pragma once
#include "ParticleModules.h"
#include "EasingManager.h"

namespace GameEngine {

	// 速度変化
	class VelocityOverLifeTimeModule : public IParticleModule {
	public:
		~VelocityOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EaseType", easeType_, index++, subGroup);
			param->Register("EndVelocity", endVelocity_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EaseType", subGroup);
			param->RemoveItem("EndVelocity", subGroup);
		}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;

	private:
		Vector3 endVelocity_;
		EaseType easeType_ = EaseType::kLinear;
	};

	// サイズを変化させる
	class SizeOverLifeTimeModule : public IParticleModule {
	public:
		~SizeOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EaseType", easeType_, index++, subGroup);
			param->Register("SeparateAxesEndSize", separateAxesEndSize_, index++, subGroup);
			param->Register("EndSize", endSize_, index++, subGroup);
			param->Register("SeparateAxes", separateAxes_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EaseType", subGroup);
			param->RemoveItem("SeparateAxesEndSize", subGroup);
			param->RemoveItem("EndSize", subGroup);
			param->RemoveItem("SeparateAxes", subGroup);
		}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;

		void SetEndScale(float scale) {
			endSize_ = scale;
		}

	private:
		float endSize_ = 0.0f;
		Vector3 separateAxesEndSize_ = {};
		// 各軸制御
		bool separateAxes_ = false;
		EaseType easeType_ = EaseType::kLinear;
	};

	// 透明度補間
	class AlphaOverLifeTimeModule : public IParticleModule {
	public:
		~AlphaOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EaseType", easeType_, index++, subGroup);
			param->Register("EndAlpha", endAlpha_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EaseType", subGroup);
			param->RemoveItem("EndAlpha", subGroup);
		}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;

	private:
		float endAlpha_ = 0.0f;
		EaseType easeType_ = EaseType::kLinear;
	};

	// 色変化
	class ColorOverLifeTimeModule : public IParticleModule {
	public:
		~ColorOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EndColor", endRGB_, index++, subGroup);
			param->Register("EaseType", easeType_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EndColor", subGroup);
			param->RemoveItem("EaseType", subGroup);
		}

		void Create(ParticleData& particleData) override;
		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;

		// 外部から色を設定する
		void SetHSV(const Vector3& endRGB) {
			endRGB_ = { endRGB.x,endRGB.y,endRGB.z };
		}

	private:
		// 色相、彩度、明度
		Vector3 startHSV_ = {};
		Vector4 endRGB_ = { 1.0f, 1.0f, 1.0f,1.0f };
		EaseType easeType_ = EaseType::kLinear;
	};

	// 引力モジュール
	class AttractionModule : public IParticleModule {
	public:
		~AttractionModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("TargetPos", targetPos_, index++, subGroup);
			param->Register("Strength", strength_, index++, subGroup);
			param->Register("Damping", damping_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("TargetPos", subGroup);
			param->RemoveItem("Strength", subGroup);
			param->RemoveItem("Damping", subGroup);
		}

		void Update(ParticleData& particleData, float time) override;

		// 目標の位置を設定
		void SetTargetPosition(const Vector3& pos) { targetPos_ = pos; }

	private:
		// 吸い込み先の目標位置
		Vector3 targetPos_ = { 0.0f, 0.0f, 0.0f };
		// 引力の強さ
		float strength_ = 10.0f;
		// 速度の減衰率
		float damping_ = 2.0f;
	};

	// らせんモジュール
	class VortexModule : public IParticleModule {
	public:
		~VortexModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("CenterPos", centerPos_, index++, subGroup);
			param->Register("RotationSpeed", rotationSpeed_, index++, subGroup);
			param->Register("AttractionSpeed", attractionSpeed_, index++, subGroup);
			param->Register("AxisSpeed", axisSpeed_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("CenterPos", subGroup);
			param->RemoveItem("RotationSpeed", subGroup);
			param->RemoveItem("AttractionSpeed", subGroup);
			param->RemoveItem("AxisSpeed", subGroup);
		}

		void Update(ParticleData& particleData, float time) override;

		// 外部から渦の中心を動かすためのセッター
		void SetCenterPosition(const Vector3& pos) { centerPos_ = pos; }

	private:
		// 渦の中心座標
		Vector3 centerPos_ = { 0.0f, 0.0f, 0.0f };
		// 回転の勢い
		float rotationSpeed_ = 5.0f;
		// 吸い込みの強さ
		float attractionSpeed_ = 1.0f;
		// 上昇、下降の速度
		float axisSpeed_ = 2.0f;
	};

	// 速度方向に回転を向けさせる
	class RotationByVelocityModule : public IParticleModule {
	public:
		~RotationByVelocityModule() = default;

		void Register([[maybe_unused]] DebugParameter* param) override {}

		void Remove([[maybe_unused]] DebugParameter* param) override {}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;
	};

}