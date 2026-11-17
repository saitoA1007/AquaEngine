#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <memory>
#include <span>
#include "DynamicBufferRegistry.h"
#include "FrameUploader.h"

namespace GameEngine {

	/// <summary>
	/// CPUが毎フレーム書き換えるバッファ
	/// </summary>
	class DynamicBuffer : private IUploadSource {
	public:
		DynamicBuffer(ID3D12Device5* device, std::weak_ptr<DynamicBufferRegistry> registry, size_t sizeInBytes);
		~DynamicBuffer();

		/// <summary>
		/// CPUが書き込むメモリをTの配列として取得する
		/// </summary>
		template <GpuUploadable T>
		std::span<T> GetData() {
			static_assert(alignof(T) <= kShadowAlignment, "シャドウメモリのアライメントが足りません");
			// バイト列をTの配列として扱う。Tはtrivially copyableなので、この変換はここだけで行う
			T* first = reinterpret_cast<T*>(shadow_.data());
			return std::span<T>(first, shadow_.size() / sizeof(T));
		}

		// GPUが読むリソース
		ID3D12Resource* GetResource() const { return resource_.Get(); }
		const Microsoft::WRL::ComPtr<ID3D12Resource>& GetResourcePtr() const { return resource_; }

		// バッファ全体のサイズ
		size_t GetSize() const { return shadow_.size(); }

		/// <summary>
		/// 比較,転送する範囲を先頭から指定バイトに制限する
		/// </summary>
		void SetUploadSize(size_t sizeInBytes) { uploadSize_ = (sizeInBytes < shadow_.size()) ? sizeInBytes : shadow_.size(); }

	private:
		DynamicBuffer(const DynamicBuffer&) = delete;
		DynamicBuffer& operator=(const DynamicBuffer&) = delete;

		// シャドウメモリのアライメント
		static constexpr size_t kShadowAlignment = 16;

		// GPUが読むDEFAULTヒープのリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
		// CPUが書き込むメモリ
		std::vector<std::byte> shadow_;
		// 最後にGPUへ送った内容
		std::vector<std::byte> uploaded_;

		// 比較、転送する範囲
		size_t uploadSize_ = 0;

		// まだ一度も送っていない
		bool needsFullUpload_ = true;

		// 登録先への登録。破棄されると自動で登録解除される
		DynamicBufferRegistry::Registration registration_;

	private:

		// 前回送った内容と比べて変化した範囲を求める
		std::optional<ByteRange> FindDirtyRange() const override;

		// 指定範囲の現在の内容
		std::span<const std::byte> GetBytes(ByteRange range) const override;

		// コピー先のGPUリソース
		ID3D12Resource* GetDestination() const override { return resource_.Get(); }

		// 指定範囲を送信済みとして記録する
		void MarkUploaded(ByteRange range) override;
	};
}
