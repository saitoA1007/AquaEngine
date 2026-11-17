#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <span>
#include <type_traits>
#include <cassert>
#include "FrameContext.h"

namespace GameEngine {

	// GPUへバイト列のまま送れる型
	template <typename T>
	concept GpuUploadable = std::is_trivially_copyable_v<T>;

	// フレーム用アップロード領域から確保したバイト列
	struct UploadRegion {
		std::span<std::byte> bytes;               // 書き込み先
		ID3D12Resource* resource = nullptr;       // アップロードヒープのリソース
		uint64_t offset = 0;                      // リソース内のオフセット
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0; // GPUアドレス
	};

	// フレーム用アップロード領域から確保した型付きの配列
	template <typename T>
	struct UploadArray {
		std::span<T> data;                        // Tとして書き込める範囲
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0; // GPUアドレス
	};

	/// <summary>
	/// フレームスロットごとのアップロード領域を管理する
	/// </summary>
	class FrameUploader {
	public:
		FrameUploader() = default;
		~FrameUploader();

		// 初期化
		void Initialize(ID3D12Device5* device);

		/// <summary>
		/// フレームスロットの使用開始。そのスロットの前回のGPU実行が完了してから呼ぶ
		/// </summary>
		void BeginFrame(uint32_t frameIndex);

	public:

		/// <summary>
		/// 現在のフレームスロットのアップロード領域から、Tをcount個ぶん確保する
		/// </summary>
		/// <param name="count">要素数</param>
		/// <param name="alignment">先頭のアライメント。alignof(T)の倍数であること</param>
		template <GpuUploadable T>
		UploadArray<T> AllocateArray(size_t count, size_t alignment = alignof(T)) {
			assert(alignment % alignof(T) == 0 && "アライメントが型の要件を満たしていません");
			UploadRegion region = AllocateBytes(sizeof(T) * count, alignment);
			// バイト列をTの配列として扱う。Tはtrivially copyableなので、この変換はここだけで行う
			T* first = reinterpret_cast<T*>(region.bytes.data());
			return { std::span<T>(first, count), region.gpuAddress };
		}

		/// <summary>
		/// 定数バッファ用に確保してデータを書き込み、GPUアドレスを返す。
		/// </summary>
		template <GpuUploadable T>
		D3D12_GPU_VIRTUAL_ADDRESS UploadConstant(const T& data) {
			UploadArray<T> upload = AllocateArray<T>(1, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
			upload.data[0] = data;
			return upload.gpuAddress;
		}

		/// <summary>
		/// バイト列をアップロード領域へコピーし、コピー先の範囲を返す
		/// </summary>
		UploadRegion UploadBytes(std::span<const std::byte> source);

	private:

		// アップロード用のページ
		struct Page {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			std::byte* cpu = nullptr;
			size_t size = 0;
			size_t offset = 0;
		};

		// フレームスロットごとのページ
		struct FrameData {
			std::vector<Page> pages;
			size_t currentPage = 0;
		};

	private:
		FrameUploader(const FrameUploader&) = delete;
		FrameUploader& operator=(const FrameUploader&) = delete;

		// 1ページの基本サイズ
		static constexpr size_t kPageSize = 4 * 1024 * 1024;

		ID3D12Device5* device_ = nullptr;

		FrameData frames_[kFrameCount];

	private:

		// ページを追加する
		Page& CreatePage(FrameData& frame, size_t sizeInBytes);

		// 現在のフレームスロットのアップロード領域からバイト列を確保する
		UploadRegion AllocateBytes(size_t sizeInBytes, size_t alignment);
	};
}
