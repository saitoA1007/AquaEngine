#pragma once
#include <vector>
#include "ParticleData.h"
#include "Matrix4x4.h"
#include "ParticleModules.h"
#include "DebugParameter.h"
#include "ModulesControl.h"
#include "IGameObject.h"
#include "Camera.h"
#include "WorldTransforms.h"

namespace GameEngine{

	class ParticleBehavior : public IGameObject {
	public:
		ParticleBehavior(const std::string& name, uint32_t maxNum, uint32_t textureHandle, Model* model, Camera* camera);
		~ParticleBehavior() = default;

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

		/// <summary>
		/// パーティクルの生成
		/// </summary>
		/// <param name="pos"></param>
		void Emit(const Vector3& pos);

		/// <summary>
		/// 行列のデータを取得
		/// </summary>
		/// <returns></returns>
		WorldTransforms* GetWorldTransforms() const { return worldTransforms_.get(); }

		/// <summary>
		/// テクスチャを設定する
		/// </summary>
		/// <param name="texture"></param>
		void SetTexture(const uint32_t& texture) { textureHandle_ = texture; }

		/// <summary>
		/// テクスチャの取得
		/// </summary>
		/// <returns></returns>
		const uint32_t& GetTexture() const { return textureHandle_; }

		/// <summary>
		/// 現在の数
		/// </summary>
		/// <returns></returns>
		uint32_t GetCurrentNumInstance() const { return currentNumInstance_; }

		/// <summary>
		/// 発生位置を設定
		/// </summary>
		/// <param name="pos"></param>
		void SetEmitterPos(const Vector3& pos) {
			emitterPos_ = pos; 
			isSetEmitPos_ = true;
		}

		// ターゲット位置を取得
		void SetAttractionTarget(const Vector3& targetPos) {
			if (auto* attraction = modulesControl_->GetModule<AttractionModule>("Attraction")) {
				attraction->SetTargetPosition(targetPos);
			}

			if (auto* Vortex = modulesControl_->GetModule<VortexModule>("Vortex")) {
				Vortex->SetCenterPosition(targetPos);
			}
		}

		void SetIsLoop(bool isLoop) {
			main_.isLoop = isLoop;
		}

		bool IsLoop() const { return main_.isLoop; }

	private:
		// パラメータ機能
		std::unique_ptr<DebugParameter> debugParame_;

		// 描画用のトランスフォーム
		std::unique_ptr<WorldTransforms> worldTransforms_;

		// モジュールの管理
		std::unique_ptr<ModulesControl> modulesControl_;

		// カメラ
		Camera* camera_ = nullptr;

		// モデル
		Model* model_ = nullptr;

		// 使用テクスチャ
		uint32_t textureHandle_ = 0;

		// パーティクルの配列
		std::vector<ParticleData> particles_;           
		uint32_t activeCount_ = 0;
		// 最大パーティクル数
		uint32_t maxNumInstance_ = 0;                 
		// 現在のパーティクルの数
		uint32_t currentNumInstance_ = 0;

		// 発生位置
		Vector3 emitterPos_ = { 0.0f,0.0f,0.0f };

		bool isPlay_ = false;
		bool isStop_ = false;
		float playTimer_ = 0.0f;

		bool isSetEmitPos_ = false;

		// 発生する時間
		float spawnTimer_ = 0.0f;

		// パーティクルの名前
		std::string name_;

		// メインモジュール
		MainModule main_;

	private:

		/// <summary>
		/// パーティクルを生成する
		/// </summary>
		/// <returns></returns>
		ParticleData MakeNewParticle();

		/// <summary>
		/// パーティクルの発生管理
		/// </summary>
		void Create();

		/// <summary>
		/// 移動処理
		/// </summary>
		void Move(const Matrix4x4& cameraMatrix);
	};
}

