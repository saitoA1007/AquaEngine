#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "DXC.h"

namespace GameEngine {

	// RayQuery(インラインレイトレーシング)をComputeShaderで実行するパイプライン
	class RayQueryPipeline {
	public:

		// ルートパラメータのインデックス
		enum RootParam : uint32_t {
			kTLAS,
			kTextures,
			kBufferRefs,
			kBuffers,
			kCamera,
			kLight,
			kOutput,
			kOutputDepth,
			kSkybox,

			kCount
		};

	public:
		RayQueryPipeline() = default;
		~RayQueryPipeline() = default;

		// 初期化
		void Initialize(ID3D12Device* device, DXC* dxc);

		// マテリアルの登録内容が変わっていればパイプラインを再生成する
		void ReloadIfNeeded();

		// ディスパッチするスレッドグループ数を取得
		static uint32_t GetDispatchCount(uint32_t size) { return (size + kThreadGroupSize - 1) / kThreadGroupSize; }

	public:

		ID3D12PipelineState* GetPipelineState() const { return pipelineState_.Get(); }

		ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

	private:

		// スレッドグループのサイズ。シェーダーのnumthreadsと合わせる
		static constexpr uint32_t kThreadGroupSize = 8;

		// 使用するシェーダー
		const std::wstring kShaderPath_ = L"Resources/Shaders/RayQuery/RayQueryRender.CS.hlsl";
		const std::wstring kShaderProfile_ = L"cs_6_6";

		ID3D12Device* device_ = nullptr;
		DXC* dxc_ = nullptr;

		// ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
		// パイプライン
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
		// 再生成前のパイプライン。GPUが使用中の可能性があるため、次の再生成まで保持する
		Microsoft::WRL::ComPtr<ID3D12PipelineState> retiredPipelineState_;

		// パイプライン生成時のマテリアル登録のバージョン
		uint32_t materialVersion_ = 0;

	private:

		// ルートシグネチャを作成
		void CreateRootSignature();

		// パイプラインを作成
		void CreatePipelineState();
	};
}
