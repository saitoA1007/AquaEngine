#pragma once
#include "RayLibShaderCompiler.h"
#include "StateObjectBuilder.h"
#include "ShaderTableBuilder.h"
#include "RenderPassController.h"
#include "SrvManager.h"
#include "TLAS.h"

namespace GameEngine {

	class ModelManager;

	namespace AppHitGroups {
		static const std::wstring DefaultModel = L"DefaultModel";
	}

	class RaytracingPipeline {
	public:
		RaytracingPipeline() = default;
		~RaytracingPipeline() = default;

		void Initialize(ID3D12Device5* device, SrvManager* srvManager, DXC* dxc,
			RenderPassController* renderPassController, TLAS* tlas);

		// シェーダーテーブルを作成
		void CreateShaderTable(ModelManager* modelManager);

	private:
		ID3D12Device5* device_ = nullptr;
		// srv管理機能
		SrvManager* srvManager_ = nullptr;
		// 描画パス
		RenderPassController* renderPassController_ = nullptr;
		TLAS* tlas_ = nullptr;

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
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsRayGen_;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsModel_;

	private:

		// グローバルルートシグネチャを作成
		void CreateGlobalRootsignature();

		// ローカルルートシグネチャを作成
		void CreateLocalRootsignature();

		// ステートオブジェクトを作成
		void CreateStateObject();
	};
}