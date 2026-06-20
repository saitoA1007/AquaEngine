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
			param->Register("RangeVelocity", velocityRange_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("RangeVelocity", subGroup);
		}

		void Create(ParticleData& particleData) override {
			particleData.velocity = {
				RandomGenerator::Get(velocityRange_.min.x, velocityRange_.max.x),
				RandomGenerator::Get(velocityRange_.min.y, velocityRange_.max.y),
				RandomGenerator::Get(velocityRange_.min.z, velocityRange_.max.z),
			};
		}

	private:
		Range3 velocityRange_;
	};

	// 回転の生成
	class RotateEmitModule : public IParticleModule {
	public:
		~RotateEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("RangeRotate", rotateRange_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("RangeRotate", subGroup);
		}

		void Create(ParticleData& particleData) override {
			particleData.transform.rotate = {
				RandomGenerator::Get(rotateRange_.min.x, rotateRange_.max.x),
				RandomGenerator::Get(rotateRange_.min.y, rotateRange_.max.y),
				RandomGenerator::Get(rotateRange_.min.z, rotateRange_.max.z),
			};
		}

	private:
		Range3 rotateRange_;
	};

	// サイズの生成
	class ScaleEmitModule : public IParticleModule {
	public:
		~ScaleEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("RangeScale", scaleRange_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("RangeScale", subGroup);
		}

		void Create(ParticleData& particleData) override {
			particleData.transform.scale = {
				RandomGenerator::Get(scaleRange_.min.x, scaleRange_.max.x),
				RandomGenerator::Get(scaleRange_.min.y, scaleRange_.max.y),
				RandomGenerator::Get(scaleRange_.min.z, scaleRange_.max.z),
			};
		}
		
	private:
		Range3 scaleRange_;
	};

	// 発射形状
	class ShapeEmitModule : public IParticleModule {
	public:
		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("Shape", emitterShape_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("Shape", subGroup);
		}

		void Create(ParticleData& particleData) override {
			Vector3 centerPos = particleData.transform.translate;

			switch (emitterShape_.type) {
			case EmitShapeType::Sphere: {
				Vector3 randomDir;
				while (true) {
					// 立方体の中でランダムに点を取り、球の中に入るまで繰り返す
					randomDir = RandomGenerator::GetVector3(Vector3(-1, -1, -1), Vector3(1, 1, 1));
					if (randomDir.LengthSquared() <= 1.0f && randomDir.LengthSquared() > 0.0001f) {
						break;
					}
				}

				if (emitterShape_.emitFromShell) {
					// 表面
					particleData.transform.translate = centerPos + randomDir.Normalize() * emitterShape_.radius;
				} else {
					// 内部
					particleData.transform.translate = centerPos + randomDir * emitterShape_.radius;
				}
				break;
			}
				
			case EmitShapeType::Hemisphere: {
				Vector3 randomDir;
				while (true) {
					randomDir = RandomGenerator::GetVector3(Vector3(-1, -1, -1), Vector3(1, 1, 1));
					// 阪急
					if (randomDir.LengthSquared() <= 1.0f && randomDir.LengthSquared() > 0.0001f && randomDir.y >= 0.0f) {
						break;
					}
				}

				if (emitterShape_.emitFromShell) {
					// 表面
					particleData.transform.translate = centerPos + randomDir.Normalize() * emitterShape_.radius;
				} else {
					// 内部
					particleData.transform.translate = centerPos + randomDir * emitterShape_.radius;
				}
				break;
			}
				
			case EmitShapeType::Box: {
				Vector3 half = emitterShape_.boxSize * 0.5f;
				particleData.transform.translate = RandomGenerator::GetVector3(centerPos - half, centerPos + half);
				break;
			}	
			}
		}

	private:
		// 形状
		EmitterShape emitterShape_;
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

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override {
			// 目標位置へのベクトルを計算
			Vector3 toTarget = targetPos_ - particleData.transform.translate;
			float distanceSquared = toTarget.LengthSquared();

			// ゼロ除算防止
			if (distanceSquared > 0.0001f) {
				Vector3 direction = toTarget.Normalize();

				// 目標に向かう加速度を現在の速度に加算
				particleData.velocity += direction * strength_ * time;
			}

			// 速度の減衰（これがないと目標を通り過ぎて往復・周回運動してしまいます）
			particleData.velocity = particleData.velocity * (1.0f - damping_ * time);
		}

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

		void Update(ParticleData& particleData, float time) override {
			// 中心点からパーティクルへのベクトル
			Vector3 offset = particleData.transform.translate - centerPos_;
			offset.y = 0.0f;

			// Y軸まわりの回転を計算するため、XZ平面上での距離を測る
			float distanceXZ = offset.Length();

			if (distanceXZ > 0.0001f) {
				// 回転方向のベクトルを計算
				Vector3 tangent;
				tangent.x = -offset.z / distanceXZ;
				tangent.y = 0.0f;
				tangent.z = offset.x / distanceXZ;

				// 中心に向かうベクトルを計算
				Vector3 inward;
				inward.x = -offset.x / distanceXZ;
				inward.y = 0.0f;
				inward.z = -offset.z / distanceXZ;

				// 各要素の速度成分をブレンドして目標速度を作る
				Vector3 targetVelocity = { 0.0f, 0.0f, 0.0f };

				// 回転速度を適用
				targetVelocity += tangent * rotationSpeed_;

				// 吸い込み
				targetVelocity += inward * attractionSpeed_;

				// 軸方向の速度を適用
				targetVelocity.y = axisSpeed_;

				// 現在の速度から目標の渦速度へ徐々に近づける
				particleData.velocity += targetVelocity * time;
			}
		}

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
}