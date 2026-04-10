#pragma once
#include"ParticleData.h"
#include "Matrix4x4.h"
#include "WorldTransforms.h"
#include <vector>

namespace GameEngine{

	class ParticleBehavior {
	public:

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="maxNum"></param>
		/// <param name="textureHandle"></param>
		/// <param name="particleEmitter"></param>
		void Initialize(const std::string& name,uint32_t maxNum, uint32_t textureHandle);

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update(const Matrix4x4& cameraMatrix);

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
		void SetEmitterPos(const Vector3& pos) { emitterPos_ = pos; }

		/// <summary>
		/// パーティクルのパラメータを設定
		/// </summary>
		/// <param name="particelEmitter"></param>
		void SetParticleEmitter(const ParticelEmitter& particelEmitter) { particleEmitter_ = particelEmitter; }

	private:
		// パーティクルの配列
		std::vector<ParticleData> particles_;           
		// 描画用のトランスフォーム
		std::unique_ptr<WorldTransforms> worldTransforms_;  
		// 最大パーティクル数
		uint32_t maxNumInstance_ = 0;                 
		// 現在のパーティクルの数
		uint32_t currentNumInstance_ = 0;

		uint32_t textureHandle_ = 0;

		// 発生位置
		Vector3 emitterPos_ = { 0.0f,0.0f,0.0f };

		// パーティクルのデータ
		ParticelEmitter particleEmitter_;

		// 発生する時間
		float spawnTimer_ = 0.0f;

		// パーティクルの名前
		std::string name_;

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
		/// デバックした値を登録
		/// </summary>
		void RegisterBebugParam();

		/// <summary>
		/// デバックした値を取得
		/// </summary>
		void ApplyDebugParam();
	};
}

