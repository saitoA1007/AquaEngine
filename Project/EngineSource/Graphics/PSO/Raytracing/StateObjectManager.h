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

	class StateObjectManager {
	public:
		StateObjectManager() =default;
		~StateObjectManager() = default;

		void Initialize(ID3D12Device5* device, SrvManager* srvManager, DXC* dxc,
			RenderPassController* renderPassController, ModelManager* modelManager,TLAS* tlas);

		void Create();

	private:
		ID3D12Device5* device_ = nullptr;
		// srv管理機能
		SrvManager* srvManager_ = nullptr;
		// モデル管理機能
		ModelManager* modelManager_ = nullptr;
		// 描画パス
		RenderPassController* renderPassController_ = nullptr;
		TLAS* tlas_ = nullptr;

		// レイトレーシング用のhlslをコンパイルする機能
		RayLibShaderCompiler rayLibShaderCompiler_;

		// ステートオブジェクトの生成機能
		StateObjectBuilder stateObjectBuilder_;

		// シェーダーテーブル作成機能
		ShaderTableBuilder shaderTableBuilder_;

		// レイトレーシングを開始する時に衣装する構造体
		D3D12_DISPATCH_RAYS_DESC dispatchRayDesc_;

		// ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureGlobal_;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsRayGen_;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsModel_;

	private:

		void CreateGlobalRootsignature();

		void CreateLocalRootsignature();

		void CreateStateObject();

		void CreateShaderTable();
	};
}