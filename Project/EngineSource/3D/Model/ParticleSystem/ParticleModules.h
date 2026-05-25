#pragma once
#include "DebugParameter.h"
#include "ParticleData.h"
#include "RandomGenerator.h"
#include "EasingManager.h"

namespace GameEngine {

	struct MainModule {
		bool isLoop = true; // ループするか
		bool isBillBoard = false; // ビルボードを使用するか判断する

		uint32_t spawnMaxCount = 1; // 出現する数
		float spawnCoolTime = 1.0f; // 発生する間隔
		float lifeTime = 1.0f; // 生存時間

		Vector3 emitterPos = {0.0f,0.0f,0.0f}; // 発生位置
		Vector3 rotate = { 0.0f,0.0f,0.0f };
		Vector3 scale = { 1.0f,1.0f,1.0f };
	};

	// パーティクルの拡張機能の基底クラス
	class IParticleModule {
	public:
		virtual ~IParticleModule() = default;

		void SetGroupName(const std::string& groupName,const std::string mainSubGroupName) {
			groupName_ = groupName;
			mainSubGroupName_ = mainSubGroupName;
		}

		// 値を登録
		virtual void Register(DebugParameter* param) = 0;
		// 値を解除。無効かした時の動きをためしたいだけなのにパラメータ自体を消してしまったら出来ないので一旦保留
		virtual void Remove(DebugParameter* param) {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveGroup(subGroup);
		}

		virtual void Create([[maybe_unused]]ParticleData& particleData) { return; }
		virtual void Update([[maybe_unused]]ParticleData& particleData, [[maybe_unused]]float time) { return; }
		
	protected:
		std::string groupName_ = "null";
		std::string mainSubGroupName_ = "null";
		ParticleData* particleData_ = nullptr;
		bool isActiveCreate_ = false;
		bool isActiveUpdate_ = false;
	};

	// 速度の生成
	class VelocityEmitModule : public IParticleModule {
	public:
		~VelocityEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("Min", minVelocity_, index++, subGroup + "/Velocity");
			param->Register("Max", maxVelocity_, index++, subGroup + "/Velocity");
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("Min", subGroup + "/Velocity");
			param->RemoveItem("Max", subGroup + "/Velocity");
		}

		void Create(ParticleData& particleData) override {
			particleData.velocity = {
				RandomGenerator::Get(minVelocity_.x, maxVelocity_.x),
				RandomGenerator::Get(minVelocity_.y, maxVelocity_.y),
				RandomGenerator::Get(minVelocity_.z, maxVelocity_.z),
			};
		}

	private:
		Vector3 minVelocity_;
		Vector3 maxVelocity_;
	};

	// 回転の生成
	class RotateEmitModule : public IParticleModule {
	public:
		~RotateEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("Min", minRotate_, index++, subGroup + "/Rotate");
			param->Register("Max", maxRotate_, index++, subGroup + "/Rotate");
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("Min", subGroup + "/Rotate");
			param->RemoveItem("Max", subGroup + "/Rotate");
		}

		void Create(ParticleData& particleData) override {
			particleData.transform.rotate = {
				RandomGenerator::Get(minRotate_.x, maxRotate_.x),
				RandomGenerator::Get(minRotate_.y, maxRotate_.y),
				RandomGenerator::Get(minRotate_.z, maxRotate_.z),
			};
		}

	private:
		Vector3 minRotate_;
		Vector3 maxRotate_;
	};

	// サイズの生成
	class ScaleEmitModule : public IParticleModule {
	public:
		~ScaleEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("Min", minScale_, index++, subGroup + "/Scale");
			param->Register("Max", maxScale_, index++, subGroup + "/Scale");
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("Min", subGroup + "/Scale");
			param->RemoveItem("Max", subGroup + "/Scale");
		}

		void Create(ParticleData& particleData) override {
			particleData.transform.scale = {
				RandomGenerator::Get(minScale_.x, maxScale_.x),
				RandomGenerator::Get(minScale_.y, maxScale_.y),
				RandomGenerator::Get(minScale_.z, maxScale_.z),
			};
		}
		
	private:
		Vector3 minScale_;
		Vector3 maxScale_;
	};

	struct ShapeModule {
		enum class ShapeType {
			Point,       // 点
			Sphere,      // 球
			Hemisphere,  // 半球
			Box,         // 直方体
		};

		bool enabled = true;
		ShapeType shapeType = ShapeType::Sphere;
	};

	// 速度変化
	class VelocityOverLifeTimeModule : public IParticleModule {
	public:
		~VelocityOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EndVelocity", endVelocity_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EndVelocity", subGroup);
		}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override {
			// 速度を補間
			particleData.velocity = Lerp(particleData.startSpeed, endVelocity_, particleData.currentTime);
		}

	private:
		Vector3 endVelocity_;
	};

	class SizeOverLifeTimeModule : public IParticleModule {
	public:
		~SizeOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("SeparateAxesEndSize", separateAxesEndSize_, index++, subGroup);
			param->Register("EndSize", endSize_, index++, subGroup);
			param->Register("SeparateAxes", separateAxes_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("SeparateAxesEndSize", subGroup);
			param->RemoveItem("EndSize", subGroup);
			param->RemoveItem("SeparateAxes", subGroup);
		}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override {

			if (separateAxes_) {
				particleData.transform.scale = Lerp(particleData.startSize, separateAxesEndSize_, particleData.currentTime);
			} else {
				particleData.transform.scale = Lerp(particleData.startSize, Vector3(endSize_, endSize_, endSize_), particleData.currentTime);
			}
		}

	private:
		float endSize_ = 0.0f;
		Vector3 separateAxesEndSize_ = {};
		// 各軸制御
		bool separateAxes_ = false;
	};

	// 透明度補間
	class AlphaOverLifeTimeModule : public IParticleModule {
	public:
		~AlphaOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EndAlpha", endAlpha_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EndAlpha", subGroup);
		}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override {
			particleData.color.w = Lerp(particleData.startColor.w, endAlpha_, particleData.currentTime);
		}

	private:
		float endAlpha_ = 0.0f;
	};
}