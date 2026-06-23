#pragma once
#include "ParticleModules.h"

namespace GameEngine {

	// 前方宣言
	class TextureManager;

	// テクスチャ
	class TextureModule : public IParticleModule {
	public:
		~TextureModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("Texture", textureData_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("Texture", subGroup);
		}

		void Create(ParticleData& particleData) override {
			// テクスチャを設定
			particleData.textureHandle = textureData_.handle;
		}

		// 名前からテクスチャハンドルを取得
		void SetTexture(TextureManager* textureManager);

	private:
		TextureData textureData_;
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

		void Create(ParticleData& particleData) override;

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

		void Create(ParticleData& particleData) override;

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
			param->Register("SeparateAxes", separateAxes_, index++, subGroup);
			param->Register("RangeScale", scaleRange_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("SeparateAxes", subGroup);
			param->RemoveItem("RangeScale", subGroup);
		}

		void Create(ParticleData& particleData) override;

	private:
		Range3 scaleRange_;
		bool separateAxes_ = false;
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

		void Create(ParticleData& particleData) override;

	private:
		// 形状
		EmitterShape emitterShape_;
	};

	// 色
	class ColorEmitModule : public IParticleModule {
	public:
		~ColorEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("MinColor", minColor_, index++, subGroup);
			param->Register("MaxColor", maxColor_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("MinColor", subGroup);
			param->RemoveItem("MaxColor", subGroup);
		}

		void Create(ParticleData& particleData) override;

	private:
		Vector4 minColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 maxColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
}