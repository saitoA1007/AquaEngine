#pragma once
#include <array>
#include "Externals/DirectXTex/d3dx12.h"
#include "DXC.h"
#include "RenderPassController.h"
#include "SrvManager.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "BLAS.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "TestCamera.h"

namespace GameEngine {

	class TestManager {
	public:
		struct VertexData {
			Vector3 position;
			Vector3 Normal;
			Vector4 Color;
		};

		struct PolygonMesh {
			VertexBuffer<VertexData> vertexBuffer_;
			IndexBuffer indexBuffer_;

			BLAS blas_;

			std::wstring shaderName;
		};
	public:
		TestManager() = default;
		~TestManager() = default;

		void Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList, DXC* dxc, SrvManager* srvManager, RenderPassController* renderPassController,
			TestCamera* testCamera);

		void Draw();

	private:
		ID3D12Device5* device_ = nullptr;
		ID3D12GraphicsCommandList4* commandList_ = nullptr;
		DXC* dxc_ = nullptr;
		RenderPassController* renderPassController_ = nullptr;
		SrvManager* srvManager_ = nullptr;
		TestCamera* testCamera_ = nullptr;

		// 床用
		PolygonMesh meshPlane;
		// キューブ用
		PolygonMesh meshCube;

		Microsoft::WRL::ComPtr<ID3D12Resource> tlas_;
		uint32_t tlasSrvIndex_ = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceDescBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> asbUpdate;

		// グローバルのルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureGlobal_;

		// RayGenシェーダーのローカルルートシグネチャ.
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsRGS;
		// 床・キューブ用のローカルルートシグネチャ.
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsModel;

		// ステートオブジェクト
		Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject_;

		// シェーダーテーブル
		Microsoft::WRL::ComPtr<ID3D12Resource> shaderTable_;

		// レイトレーシングを開始する時に衣装する構造体
		D3D12_DISPATCH_RAYS_DESC dispatchRayDesc_;

		std::wstring DefaultHitgroup_ = L"hgObject";

	private:
		// シーンに必要なオブジェクトを生成する.
		void CreateSceneObjects();

		// 床とキューブの BLAS を構築する.
		void CreateSceneBLAS();

		// BLAS を束ねてシーンの TLAS を構築します.
		void CreateSceneTLAS();

		// レイトレーシング用の StateObject を構築します.
		void CreateStateObject();

		// レイトレーシング結果書き込み用バッファを生成します.
		void CreateResultBuffer();

		// ルートシグネチャ(Global) を生成します.
		// ルートシグネチャの仕組みはラスタライズ用PSOと変わらない。
		// 既存の仕組みを活かせるかも
		void CreateRootSignatureGlobal();

		// RayGenシェーダー用ローカルルートシグネチャを生成します.
		void CreateLocalRootSignatureRayGen();

		// ClosestHitシェーダー用ローカルルートシグネチャを生成します.
		void CreateLocalRootSignatureCHS();

		// レイトレーシングで使用する ShaderTable を構築します.
		// レイトレーシングパイプラインで使用する全てのシェーダーの情報が格納されたデータ
		// クラスかする時は単一で持つように設計をおこなう
		void CreateShaderTable();

	private: // ヘルプ関数

		UINT RoundUp(size_t size, UINT align) {
			return UINT(size + align - 1) & ~(align - 1);
		}

		// シェーダーテーブルのバッファ専用
		void CreateBuffer(int tableSize) {
			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Alignment = 0;
			resourceDesc.Width = tableSize;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			HRESULT hr = device_->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(shaderTable_.ReleaseAndGetAddressOf())
			);
			if (FAILED(hr)) {
				throw std::runtime_error("Failed to create shader table buffer.");
			}
		}

		void GetPlane(std::vector<VertexData>& vertices, std::vector<UINT>& indices);

		void GetColoredCube(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices);

		void DeployObjects(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs);

		UINT WriteShaderIdentifier(void* dst, const void* shaderId)
		{
			memcpy(dst, shaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			return D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		}
		UINT WriteGPUDescriptor(
			void* dst, const D3D12_GPU_DESCRIPTOR_HANDLE& descriptor)
		{
			auto handle = descriptor;
			memcpy(dst, &handle, sizeof(handle));
			return UINT(sizeof(handle));
		}

		uint8_t* WriteShaderRecord(uint8_t* dst, const PolygonMesh& mesh, UINT recordSize)
		{
			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtsoProps;
			stateObject_.As(&rtsoProps);
			auto entryBegin = dst;
			auto shader = mesh.shaderName;
			auto id = rtsoProps->GetShaderIdentifier(shader.c_str());
			if (id == nullptr) {
				throw std::logic_error("Not found ShaderIdentifier");
			}

			dst += WriteShaderIdentifier(dst, id);
			// 今回のプログラムでは以下の順序でディスクリプタを記録.
			// [0] : インデックスバッファ
			// [1] : 頂点バッファ
			// ※ ローカルルートシグネチャの順序に合わせる必要がある.
			auto IBGpuHandle = mesh.indexBuffer_.GetSrvGpuHandle();
			auto VBGpuHandle = mesh.vertexBuffer_.GetSrvGpuHandle();
			dst += WriteGPUDescriptor(dst, IBGpuHandle);
			dst += WriteGPUDescriptor(dst, VBGpuHandle);

			dst = entryBegin + recordSize;
			return dst;
		}
	};
}

