#pragma once
#include <vector>
#include "SrvResource.h"
#include "BLAS.h"

namespace GameEngine {

	// TLASに登録する1つ分のインスタンス情報
	struct TLASInstanceData {
		BLAS* blas = nullptr;             // BLAS
		float transform[3][4];            // ワールド変換行列
		uint32_t instanceID = 0;          // シェーダー側で取得できる任意のID
		uint32_t hitGroupIndexOffset = 0; // このモデルが使うマテリアル(HitGroup)のインデックス
	};

	class TLAS :public SrvResource {
	public:
		TLAS() = default;
		~TLAS();

		/// <summary>
		/// 複数のインスタンス情報からTLASを構築する
		/// </summary>
		void Create(ID3D12GraphicsCommandList4* cmdList, const std::vector<TLASInstanceData>& instances);

		// SRVインデックスの取得
		uint32_t GetSrvIndex() const { return srvIndex_; }

	private:
		// コピー禁止
		TLAS(const TLAS&) = delete;
		TLAS& operator=(const TLAS&) = delete;

		// インスタンス情報をGPUに送るためのバッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;

		// SRVインデックス
		uint32_t srvIndex_ = 0;
	};
}
