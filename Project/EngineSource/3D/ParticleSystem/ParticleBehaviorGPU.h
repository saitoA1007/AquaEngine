#pragma once
#include "IGameObject.h"
#include "StructuredBuffer.h"
#include "ParticleData.h"

namespace GameEngine {

	// 前方宣言
	class PSOManager;

	class ParticleBehaviorGPU : public IGameObject {
	public:
		ParticleBehaviorGPU(const std::string& name, uint32_t maxNum);
		~ParticleBehaviorGPU() = default;

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="commandList"></param>
		/// <param name="psoManager"></param>
		static void StaticInitialize(ID3D12GraphicsCommandList4* commandList, PSOManager* psoManager);

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

	private:
		static ID3D12GraphicsCommandList4* commandList_;
		static ID3D12RootSignature* rootSignature_;
		static ID3D12PipelineState* pipelineState_;

		// パーティクルの名前
		std::string name_;

		// パーティクルデータ
		StructuredBuffer<ParticleCS> particleBuffer_;

	};
}

