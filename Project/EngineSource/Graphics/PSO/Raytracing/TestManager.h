#pragma once
#include <array>
#include <numbers>
#include "Externals/DirectXTex/d3dx12.h"
#include "DXC.h"
#include "RenderPassController.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "BLAS.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "TestCamera.h"
#include "MyMath.h"

namespace GameEngine {

	namespace AppHitGroups {
		static const std::wstring NoPhongMaterial = L"hgNoPhongSpheres";
		static const std::wstring PhongMaterial = L"hgPhongSpheres";
		static const std::wstring Floor = L"hgFloor";
	}

	class TestManager {
	public:
	
		struct VertexData {
			Vector3 position;
			Vector3 Normal;
			Vector4 Color;
		};

		struct VertexPN {
			Vector3 Position;
			Vector3 Normal;
		};
		struct VertexPNT {
			Vector3 Position;
			Vector3 Normal;
			Vector2 UV;
		};

		template<class T>
		struct PolygonMesh {
			VertexBuffer<T> vertexBuffer_;
			IndexBuffer indexBuffer_;

			BLAS blas_;

			std::wstring shaderName;
		};

		enum SphereTypeCount {
			NormalSpheres = 10,
			ReflectSpheres = 10,
			RefractSpheres = 10,
			SpheresAll = (NormalSpheres + ReflectSpheres + RefractSpheres),
		};

		struct MaterialParam
		{
			Vector4 albedo;
			Vector4 specular; // スペキュラー色 & w要素にPower
		};
	public:
		TestManager() = default;
		~TestManager() = default;

		void Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList, DXC* dxc, SrvManager* srvManager, RenderPassController* renderPassController,
			TestCamera* testCamera, TextureManager* textureManager);

		void Draw();

	private:
		ID3D12Device5* device_ = nullptr;
		ID3D12GraphicsCommandList4* commandList_ = nullptr;
		DXC* dxc_ = nullptr;
		RenderPassController* renderPassController_ = nullptr;
		SrvManager* srvManager_ = nullptr;
		TestCamera* testCamera_ = nullptr;
		TextureManager* textureManager_ = nullptr;

		// 床用
		PolygonMesh<VertexPNT> meshPlane;
		// 球用
		PolygonMesh<VertexPN> meshSphere_;

		// 球の座標リスト
		std::array<Matrix4x4, ReflectSpheres> spheresReflect_;
		std::array<Matrix4x4, RefractSpheres> spheresRefract_;
		std::array<Matrix4x4, NormalSpheres> spheresNormal_;
		// Phong描画するための通常スフィアのマテリアル情報.
		std::array<MaterialParam, NormalSpheres> normalSphereMaterials_;
		Microsoft::WRL::ComPtr<ID3D12Resource> normalSphereMaterialCB_;

		Microsoft::WRL::ComPtr<ID3D12Resource> tlas_;
		uint32_t tlasSrvIndex_ = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceDescBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> asbUpdate;

		// グローバルのルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureGlobal_;

		// RayGenシェーダーのローカルルートシグネチャ.
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsRGS;
		// 床・キューブ用のローカルルートシグネチャ.
		//Microsoft::WRL::ComPtr<ID3D12RootSignature> rsModel;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsFloor_; // 床のローカルルートシグネチャ.
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsSphere1_; // スフィアのローカルルートシグネチャ(反射・屈折用).
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rsSphere2_; // スフィアのローカルルートシグネチャ(Phong用).


		// ステートオブジェクト
		Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject_;

		// シェーダーテーブル
		Microsoft::WRL::ComPtr<ID3D12Resource> shaderTable_;

		// レイトレーシングを開始する時に衣装する構造体
		D3D12_DISPATCH_RAYS_DESC dispatchRayDesc_;

		Vector4 colorTable[5] = {
			Vector4(1.0f, 1.0f, 1.0f, 0.0f),
			Vector4(0.5f, 0.8f, 0.4f, 0.0f),
			Vector4(0.7f, 0.6f, 0.2f, 0.0f),
			Vector4(0.2f, 0.3f, 0.6f, 0.0f),
			Vector4(0.1f, 0.8f, 0.9f, 0.0f),
		};
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

		// スフィア用のローカルルートシグネチャを生成します.
		void CreateSphereLocalRootSignature();

		// 床用のローカルルートシグネチャを生成します.
		void CreateFloorLocalRootSignature();

		// ClosestHitシェーダー用ローカルルートシグネチャを生成します.
		//void CreateLocalRootSignatureCHS();

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

		void GetPlane(std::vector<VertexPNT>& vertices, std::vector<UINT>& indices);

		void GetColoredCube(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices);

		void DeployObjects(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs);

		UINT WriteShaderIdentifier(void* dst, const void* shaderId)
		{
			std::memcpy(dst, shaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			return D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		}
		UINT WriteGPUDescriptor(
			void* dst, const D3D12_GPU_DESCRIPTOR_HANDLE& descriptor)
		{
			auto handle = descriptor;
			std::memcpy(dst, &handle, sizeof(handle));
			return UINT(sizeof(handle));
		}
		UINT WriteGpuResourceAddr(void* dst, const Microsoft::WRL::ComPtr<ID3D12Resource>& res)
		{
			D3D12_GPU_VIRTUAL_ADDRESS addr = res->GetGPUVirtualAddress();
			std::memcpy(dst, &addr, sizeof(addr));
			return UINT(sizeof(addr));
		}
		UINT WriteGpuResourceAddr(void* dst, const D3D12_GPU_VIRTUAL_ADDRESS addr)
		{
			std::memcpy(dst, &addr, sizeof(addr));
			return UINT(sizeof(addr));
		}

		template<class T>
		uint8_t* WriteHitgroupFloor(uint8_t* dst, const PolygonMesh<T>& mesh, UINT hitgroupRecordSize) {
			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtsoProps;
			stateObject_.As(&rtsoProps);
			auto recordStart = dst;
			auto id = rtsoProps->GetShaderIdentifier(AppHitGroups::Floor.c_str());
			if (id == nullptr) {
				throw std::logic_error("Not found ShaderIdentifier");
			}
			dst += WriteShaderIdentifier(dst, id);
			dst += WriteGPUDescriptor(dst, mesh.indexBuffer_.GetSrvGpuHandle());
			dst += WriteGPUDescriptor(dst, mesh.vertexBuffer_.GetSrvGpuHandle());
			dst += WriteGPUDescriptor(dst, srvManager_->GetGPUHandle(textureManager_->GetHandleByName("grass.png")));

			dst = recordStart + hitgroupRecordSize;
			return dst;
		}
		template<class T>
		uint8_t* WriteHitgroupMaterial(uint8_t* dst, const PolygonMesh<T>& mesh, UINT hitgroupRecordSize) {
			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtsoProps;
			stateObject_.As(&rtsoProps);
			auto recordStart = dst;
			auto id = rtsoProps->GetShaderIdentifier(AppHitGroups::NoPhongMaterial.c_str());
			if (id == nullptr) {
				throw std::logic_error("Not found ShaderIdentifier");
			}
			dst += WriteShaderIdentifier(dst, id);
			dst += WriteGPUDescriptor(dst, mesh.indexBuffer_.GetSrvGpuHandle());
			dst += WriteGPUDescriptor(dst, mesh.vertexBuffer_.GetSrvGpuHandle());

			dst = recordStart + hitgroupRecordSize;
			return dst;
		}
		template<class T>
		uint8_t* WriteHitgroupPhong(uint8_t* dst, const PolygonMesh<T>& mesh, D3D12_GPU_VIRTUAL_ADDRESS address, UINT hitgroupRecordSize) {
			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtsoProps;
			stateObject_.As(&rtsoProps);
			auto recordStart = dst;
			auto id = rtsoProps->GetShaderIdentifier(AppHitGroups::PhongMaterial.c_str());
			if (id == nullptr) {
				throw std::logic_error("Not found ShaderIdentifier");
			}
			dst += WriteShaderIdentifier(dst, id);
			dst += WriteGPUDescriptor(dst, mesh.indexBuffer_.GetSrvGpuHandle());
			dst += WriteGPUDescriptor(dst, mesh.vertexBuffer_.GetSrvGpuHandle());
			dst += WriteGpuResourceAddr(dst, address);

			dst = recordStart + hitgroupRecordSize;
			return dst;
		}

		void SetSphereVertex(
			VertexPN& vert,
			const Vector3& position, const Vector3& normal, const Vector2& uv) {
			vert.Position = position;
			vert.Normal = normal;
		}
		void SetSphereVertex(
			VertexPNT& vert,
			const Vector3& position, const Vector3& normal, const Vector2& uv) {
			vert.Position = position;
			vert.Normal = normal;
			vert.UV = uv;
		}

		template<class T>
		void CreateSphereVertices(std::vector<T>& vertices, float radius, int slices, int stacks) {
			vertices.clear();
			const auto SLICES = float(slices);
			const auto STACKS = float(stacks);
			for (int stack = 0; stack <= stacks; ++stack) {
				for (int slice = 0; slice <= slices; ++slice) {
					Vector3 p;
					p.y = 2.0f * stack / STACKS - 1.0f;
					float r = std::sqrtf(1 - p.y * p.y);
					float theta = 2.0f * std::numbers::pi_v<float> *slice / SLICES;
					p.x = r * std::sinf(theta);
					p.z = r * std::cosf(theta);

					Vector3 v = p * radius;
					Vector3 n = Normalize(v);
					Vector2 uv = {
						float(slice) / SLICES,
						1.0f - float(stack) / STACKS,
					};

					T vtx{};
					SetSphereVertex(vtx, v, n, uv);
					vertices.push_back(vtx);
				}
			}
		}
		void CreateSphereIndices(std::vector<UINT>& indices, int slices, int stacks);
		void GetSphere(std::vector<VertexPN>& vertices, std::vector<UINT>& indices, float radius, int slices, int stacks);
		void GetSphere(std::vector<VertexPNT>& vertices, std::vector<UINT>& indices, float radius, int slices, int stacks);
	};
}

