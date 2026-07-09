#pragma once
#include "IGameObject.h"

namespace GameEngine {

	// 前方宣言
	class PSOManager;

	class ParticleBehaviorGPU : public IGameObject {
	public:
		ParticleBehaviorGPU();
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


	};
}

