#pragma once
#include "RayLibShaderCompiler.h"
#include "StateObjectBuilder.h"
#include "ShaderTableBuilder.h"
#include "SrvManager.h"

namespace GameEngine {

	namespace AppHitGroups {
		static const std::wstring DefaultModel = L"DefaultModel";
		static const std::wstring IceModel = L"IceModel";
	}

	class RaytracingPipeline {
	public:
		RaytracingPipeline() = default;
		~RaytracingPipeline() = default;

		// 初期化
		void Initialize(ID3D12Device5* device, SrvManager* srvManager, DXC* dxc);

		// シェーダーテーブルを作成
		void CreateShaderTable();

	public:

		const D3D12_DISPATCH_RAYS_DESC& GetDispatchRayDesc() const { return dispatchRayDesc_; }

		ID3D12StateObject* GetStateObject() const { return stateObject_.Get(); }

		ID3D12RootSignature* GetGlobalRootSignature() const {return rootSignatureGlobal_.Get();}

		// RayQueryでレイトレーシングを行うコンピュートシェーダーのPSO
		ID3D12PipelineState* GetRayQueryPipelineState() const { return rayQueryPipelineState_.Get(); }

		// RayQueryでパストレーシングを行うコンピュートシェーダーのPSO
		ID3D12PipelineState* GetPathTracePipelineState() const { return pathTracePipelineState_.Get(); }

		// パストレーシングのデノイズ(PathTraceDenoise.CS.hlsl)
		ID3D12RootSignature* GetDenoiseRootSignature() const { return rootSignatureDenoise_.Get(); }
		ID3D12PipelineState* GetTemporalPipelineState() const { return temporalPipelineState_.Get(); }
		ID3D12PipelineState* GetStoreCameraPipelineState() const { return storeCameraPipelineState_.Get(); }
		ID3D12PipelineState* GetAtrousPipelineState() const { return atrousPipelineState_.Get(); }
		// デノイズのルートシグネチャのUAVの数(u0~)
		static constexpr uint32_t kDenoiseUavCount = 12;

		// ビジビリティバッファ(VisibilityBuffer.hlsl)
		ID3D12RootSignature* GetVisibilityRootSignature() const { return rootSignatureVisibility_.Get(); }
		ID3D12PipelineState* GetVisibilityPipelineState() const { return visibilityPipelineState_.Get(); }
		// ビジビリティバッファのインスタンス定数の数(番号4つ + 3x4行列)
		static constexpr uint32_t kVisibilityConstantCount = 16;

		// RayQueryのディスパッチ数。1スレッドグループで8x8画素を処理する
		uint32_t GetRayQueryThreadGroupCountX() const { return (dispatchRayDesc_.Width + kRayQueryThreadGroupSize_ - 1) / kRayQueryThreadGroupSize_; }
		uint32_t GetRayQueryThreadGroupCountY() const { return (dispatchRayDesc_.Height + kRayQueryThreadGroupSize_ - 1) / kRayQueryThreadGroupSize_; }

	private:
		ID3D12Device5* device_ = nullptr;
		// srv管理機能
		SrvManager* srvManager_ = nullptr;
		// シェーダーコンパイラ
		DXC* dxc_ = nullptr;

		// レイトレーシング用のhlslをコンパイルする機能
		RayLibShaderCompiler rayLibShaderCompiler_;

		// ステートオブジェクトの生成機能
		StateObjectBuilder stateObjectBuilder_;
		Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject_;

		// シェーダーテーブル作成機能
		ShaderTableBuilder shaderTableBuilder_;

		// レイトレーシングを開始する時に衣装する構造体
		D3D12_DISPATCH_RAYS_DESC dispatchRayDesc_;

		// ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureGlobal_;

		// RayQuery用のPSO。ルートシグネチャはグローバルルートシグネチャを共有する
		Microsoft::WRL::ComPtr<ID3D12PipelineState> rayQueryPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pathTracePipelineState_;

		// デノイズ用
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureDenoise_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> temporalPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> storeCameraPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> atrousPipelineState_;

		// ビジビリティバッファ用
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureVisibility_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> visibilityPipelineState_;
		static constexpr uint32_t kRayQueryThreadGroupSize_ = 8;

	private:

		// グローバルルートシグネチャを作成
		void CreateGlobalRootsignature();

		// ステートオブジェクトを作成
		void CreateStateObject();

		// RayQuery用のコンピュートPSOを作成
		void CreateRayQueryPipeline();

		// パストレーシングのデノイズ用のルートシグネチャとPSOを作成
		void CreateDenoisePipeline();

		// ビジビリティバッファを描画するルートシグネチャとPSOを作成
		void CreateVisibilityPipeline();

		// hlslとエントリーポイントを指定してコンピュートPSOを作成
		Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateComputePipeline(const std::wstring& hlslPath, const std::wstring& entryPoint, ID3D12RootSignature* rootSignature);
	};
}