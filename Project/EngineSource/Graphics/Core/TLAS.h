#pragma once
#include <vector>
#include "SrvResource.h"
#include "BLAS.h"
#include "Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

	// 前方宣言
	class FrameUploader;

	// レイのフィルタリングに使うインスタンスマスク
	enum class RayInstanceMask {
		kRayMaskOpaque = 0x01, // 不透明。影レイを完全に遮る
		kRayMaskIce = 0x02, // 氷などの透過物。影レイを一部だけ遮る
		kRayMaskAll = 0xFF, // 全てのインスタンス
	};

	// TLASに登録する1つ分のインスタンス情報
	struct TLASInstanceData {
		BLAS* blas = nullptr;             // BLAS
		float transform[3][4];            // ワールド変換行列
		uint32_t instanceID = 0;          // シェーダー側で取得できる任意のID
		uint32_t hitGroupIndexOffset = 0; // hitGroupのどのレコードを使用するか
		uint32_t instanceMask = static_cast<uint32_t>(RayInstanceMask::kRayMaskOpaque); // レイキャスト時のフィルタリング用マスク
	};

	class TLAS : public SrvResource {
	public:
		TLAS() = default;
		~TLAS();

		/// <summary>
		/// 初期容量分のバッファを確保してTLASを初期化する
		/// </summary>
		void Create(ID3D12GraphicsCommandList4* cmdList, const uint32_t& initialCapacity);

		/// <summary>
		/// 更新
		/// </summary>
		/// <param name="cmdList"></param>
		/// <param name="instances"></param>
		/// <param name="uploader"></param>
		void Update(ID3D12GraphicsCommandList4* cmdList, const std::vector<TLASInstanceData>& instances, FrameUploader& uploader);

		// SRVインデックスの取得
		uint32_t GetSrvIndex() const { return srvIndex_; }

		// このフレームのインスタンス情報のGPUアドレス
		D3D12_GPU_VIRTUAL_ADDRESS GetInstanceDescAddress() const { return instanceDescAddress_; }
		const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetSrvHandleGPU() const { return srvHandleGPU_; }
	
	private:
		// コピー禁止
		TLAS(const TLAS&) = delete;
		TLAS& operator=(const TLAS&) = delete;

		// 現在確保している容量
		uint32_t maxInstanceCount_ = 0;

		// 作業リソース
		Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer_;
		// ビルドの入力設定
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs_{};

		// 前フレームの有効インスタンス数
		uint32_t previousInstanceCount_ = UINT32_MAX;

		// 連続でリフィットした回数
		uint32_t consecutiveRefitCount_ = 0;

		// リフィットを連続で許可する上限
		uint32_t kMaxConsecutiveRefits_ = 4;

		// SRVインデックス
		uint32_t srvIndex_ = 0;
		// CPUのシェーダリソースビューのハンドル
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_;
		// GPUのシェーダリソースビューのハンドル
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_;

		bool isCreated_ = false;

		// このフレームのインスタンス情報のGPUアドレス
		D3D12_GPU_VIRTUAL_ADDRESS instanceDescAddress_ = 0;

	private:

		// 指定した容量でインスタンスバッファ、AS本体、スクラッチバッファを確保し直す
		void AllocateBuffers(uint32_t capacity);

		// SRVを作成し直す
		void CreateSrv();

		// 必要な数が現在の容量を超えていればバッファを拡張する
		void EnsureCapacity(uint32_t requiredCount);
	};
}
