#pragma once
#include <vector>
#include "SrvResource.h"
#include "BLAS.h"

namespace GameEngine {

	// TLASに登録する1つ分のインスタンス情報
	struct TLASInstanceData {
		BLAS* blas = nullptr;             // BLAS
		float transform[3][4];            // ワールド行列
		uint32_t instanceID = 0;          // シェーダー側で取得できる任意のID。重複しても問題ない
		uint32_t hitGroupIndexOffset = 0; // このモデルが使うHitGroupのインデックス
	};

	class TLAS :public SrvResource {
	public:
		TLAS() = default;
		~TLAS();

		/// <summary>
		/// インスタンス情報からTLASを構築する
		/// </summary>
		void Create(ID3D12GraphicsCommandList4* cmdList, const std::vector<TLASInstanceData>& instances, bool isUpdate = false);

		/// <summary>
		/// インスタンス情報の更新をおこなう
		/// </summary>
		void Update(ID3D12GraphicsCommandList4* cmdList, const std::vector<TLASInstanceData>& instances);

		// SRVインデックスの取得
		uint32_t GetSrvIndex() const { return srvIndex_; }

	private:
		// コピー禁止
		TLAS(const TLAS&) = delete;
		TLAS& operator=(const TLAS&) = delete;

		// インスタンス情報をGPUに送るためのバッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;

		// 更新用
		Microsoft::WRL::ComPtr<ID3D12Resource> tlasUpdate_;

		// SRVインデックス
		uint32_t srvIndex_ = 0;

		bool isCreated_ = false;
	};
}
