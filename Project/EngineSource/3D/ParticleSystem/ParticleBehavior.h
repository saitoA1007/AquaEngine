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
#include "ParticleUpdateModules.h"
#include "ParticleEmitModules.h"

namespace GameEngine{

	// 前方宣言
	class TextureManager;

	/// <summary>
	/// パーティクルのシミュレーション空間
	/// </summary>
	enum class ParticleSimulationSpace {
		// ローカル空間：発生後も常に親に追従する（親が動くとパーティクルも一緒に動く）
		kLocal,
		// ワールド空間：発生時だけ親の影響を受け、その後は世界に置き去りになる
		kWorld,
	};

	class ParticleBehavior : public IGameObject {
	public:
		ParticleBehavior(const std::string& name, uint32_t maxNum, TextureManager* textureManager, Model* model);
		~ParticleBehavior() = default;

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

	public:

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

		// 発射形状の半径の範囲を設定
		void SetShapeEmit(float radius) {
			if (auto* ShapeEmit = modulesControl_->GetModule<ShapeEmitModule>("ShapeEmit")) {
				ShapeEmit->SetRadius(radius);
			}
		}

		// 最終的な大きさ
		void SetSizeOverLifeTime(float endScale) {
			if (auto* ShapeEmit = modulesControl_->GetModule<SizeOverLifeTimeModule>("SizeOverLifeTime")) {
				ShapeEmit->SetEndScale(endScale);
			}
		}

		void SetIsLoop(bool isLoop) {
			main_.isLoop = isLoop;
		}

		// パーティクルの移動方向を設定
		void SetDirection(const Vector3& direction) {
			if (auto* directionModule = modulesControl_->GetModule<DirectionEmitModule>("DirectionEmit")) {
				directionModule->SetDirection(direction);
			}
		}

		// パーティクルの生存時間を設定
		void SetLifeTime(float minLifeTime, float maxLifeTime) {
			if (auto* lifeTimeModule = modulesControl_->GetModule<LifeTimeEmitModule>("LifeTimeEmit")) {
				lifeTimeModule->SetLifeTime(minLifeTime, maxLifeTime);
			}
		}

		bool IsLoop() const { return main_.isLoop; }

		// 色を設定
		void SetColor(Vector4 color) {
			main_.color = color;
		}

		// サイズを設定
		void SetScale(Vector3 scale) {
			main_.scale = scale;
		}

		/// <summary>
		/// 親を設定する
		/// </summary>
		/// <param name="parentMatrix">親のワールド行列（nullptrで親子付けを解除）</param>
		/// <param name="space">
		/// kLocal : 発生後も常に親に追従する
		/// kWorld : 発生時の親の姿勢だけを反映し、その後は追従しない
		/// </param>
		void SetParent(const Matrix4x4* parentMatrix, ParticleSimulationSpace space = ParticleSimulationSpace::kLocal) {
			parentMatrix_ = parentMatrix;
			simulationSpace_ = space;
		}

		/// <summary>
		/// 親子付けを解除する
		/// </summary>
		/// <param name="keepWorldPosition">
		/// true にすると、現在生きているパーティクルを解除時のワールド座標に焼き込んでその場に残す
		/// </param>
		void ClearParent(bool keepWorldPosition = true);

		/// <summary>
		/// シミュレーション空間を切り替える
		/// </summary>
		void SetSimulationSpace(ParticleSimulationSpace space) { simulationSpace_ = space; }

		// 親の行列を取得（未設定ならnullptr）
		const Matrix4x4* GetParent() const { return parentMatrix_; }

		// 親が設定されているか
		bool HasParent() const { return parentMatrix_ != nullptr; }
		// シミュレーション空間を取得
		ParticleSimulationSpace GetSimulationSpace() const { return simulationSpace_; }

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

		// 親行列
		const Matrix4x4* parentMatrix_ = nullptr;

		// シミュレーション空間
		ParticleSimulationSpace simulationSpace_ = ParticleSimulationSpace::kLocal;

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

		/// <summary>
		/// 常に親に追従するか（親があり、かつローカル空間のとき true）
		/// </summary>
		bool IsFollowParent() const {
			return parentMatrix_ != nullptr && simulationSpace_ == ParticleSimulationSpace::kLocal;
		}
	};
}

