#pragma once
#include"Matrix4x4.h"
#include"Transform.h"
#include"TransformationMatrix.h"

#include"VertexData.h"
#include"Mesh.h"

#include"ComputePSO/ParticleCSPSO.h"
#include"TextureManager.h"
#include <d3d12.h>

namespace GameEngine {
	class Particles {
	public:

		struct ParticleCS {
			Vector3 translate;
			Vector3 scale;
			float lifeTime;
			Vector3 velocity;
			float currentTime;
			Vector4 color;
		};

		struct PerView {
			Matrix4x4 viewProjection;
			Matrix4x4 billboardMatrix;
		};

	public:

		void Initialize();

		void Update();

		void Draw(const Matrix4x4& VPMatrix,const uint32_t& textureHandle);

	private:

		static ID3D12Device* device_;
		static ID3D12GraphicsCommandList* commandList_;
		// テクスチャ
		static TextureManager* textureManager_;

		// PSO設定
		static ParticleCSPSO* particleCSPSO_;

		// リソースをカウント
		static uint32_t StaticSrvIndex_;

		// 複数メッシュに対応
		std::unique_ptr<Mesh> mesh_;

		// パーティクル用
		Microsoft::WRL::ComPtr<ID3D12Resource> computeResource_;

		ParticleCS* particleData_ = nullptr;

		// シェーダリソースビューのハンドル(CPU)
		CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandleCPU_;
		// シェーダリソースビューのハンドル(CPU)
		CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandleGPU_;

		// シェーダリソースビューのハンドル(CPU)
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_;
		// シェーダリソースビューのハンドル(CPU)
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_;

		// カメラ用
		Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;

		PerView* PerViewData_ = nullptr;
	};
}