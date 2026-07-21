#pragma once
#include <queue>
#include "Matrix4x4.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "StructuredBuffer.h"
#include "ConstantBuffer.h"
#include "PackedGeometryBuffer.h"

namespace GameEngine {

	// CPU側で各破片の状態を管理
	struct FractureChunkState {
		Transform transform;
	};

	struct FractureForGPU {
		Matrix4x4 World;

		uint32_t vertexOffset; // PackedGeometryBuffer内でのチャンクの頂点開始位置
		uint32_t indexOffset; // PackedGeometryBuffer内でのチャンクのインデックス開始位置
		uint32_t indexCount; // IndexCountPerInstance に相当
		uint32_t chunkId; // シェーダー側で gParticle を引くためのID
	};

	struct FractureIndirectCommand {
		uint32_t instanceIndex;
		D3D12_DRAW_INDEXED_ARGUMENTS drawArguments;
	};

	/// <summary>
	/// 複数描画用のワールド行列
	/// </summary>
	class FractureInstance {
	public:
		FractureInstance() = default;
		~FractureInstance();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="transform"></param>
		void Initialize(const std::vector<uint32_t>& chunkIds, const PackedGeometryBuffer& geometryBuffer);

		/// <summary>
		/// SRTを適応
		/// </summary>
		void Update();

		const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetInstancingSrvGPU() const { return buffer_.GetSrvGpuHandle(); }

		/// <summary>
		/// 描画するモデルの数
		/// </summary>
		/// <returns></returns>
		const uint32_t GetNumInstance() { return numInstance_; }

		// 外部からCPU側のトランスフォームを操作できるようにゲッターを用意
		std::vector<FractureChunkState>& GetTransformDatas() { return transformDatas_; }

		StructuredBuffer<FractureForGPU>& GetBuffer() { return buffer_; }

		ConstantBuffer<FractureIndirectCommand>& GetArgumentBuffer() { return argumentBuffer_; }
	private:
		// コピー禁止
		FractureInstance(const FractureInstance&) = delete;
		FractureInstance& operator=(const FractureInstance&) = delete;

		// インスタンスが持つsrvインデックス
		uint32_t srvIndex_ = 0;

		// transformData配列数
		uint32_t numInstance_ = 0;

		// リソース
		StructuredBuffer<FractureForGPU> buffer_;
		FractureForGPU* instancingData_ = nullptr;

		std::vector<FractureChunkState> transformDatas_;

		// ExecuteIndirect 用の間接描画引数バッファ
		ConstantBuffer<FractureIndirectCommand> argumentBuffer_;
		FractureIndirectCommand* argumentData_ = nullptr;
	};
}

