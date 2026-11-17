#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <optional>
#include <span>
#include <cstddef>
#include <cstdint>
#include "FrameContext.h"

namespace GameEngine {

	// 前方宣言
	class FrameUploader;

	// バッファ内のバイト範囲
	struct ByteRange {
		size_t begin = 0;
		size_t end = 0;

		size_t Size() const { return end - begin; }
	};

	/// <summary>
	/// 転送元として見たときのバッファ。DynamicBufferRegistryだけが使う操作をまとめる
	/// </summary>
	class IUploadSource {
	public:
		// 前回送った内容から変化した範囲。変化が無ければnullopt
		virtual std::optional<ByteRange> FindDirtyRange() const = 0;
		// 指定範囲の現在の内容
		virtual std::span<const std::byte> GetBytes(ByteRange range) const = 0;
		// コピー先のGPUリソース
		virtual ID3D12Resource* GetDestination() const = 0;
		// 指定範囲を送信済みとして記録する
		virtual void MarkUploaded(ByteRange range) = 0;

	protected:
		IUploadSource() = default;
		// インターフェース経由で破棄させない
		~IUploadSource() = default;
		IUploadSource(const IUploadSource&) = default;
		IUploadSource& operator=(const IUploadSource&) = default;
	};

	/// <summary>
	/// 生成されたDynamicBufferの一覧を持ち、変化した内容をGPU側へ転送する
	/// </summary>
	class DynamicBufferRegistry : public std::enable_shared_from_this<DynamicBufferRegistry> {
	public:

		/// <summary>
		/// 登録を表すトークン。破棄すると自動で登録解除される
		/// </summary>
		class Registration {
		public:
			Registration() = default;
			Registration(std::weak_ptr<DynamicBufferRegistry> registry, uint32_t id);
			~Registration();

			Registration(Registration&& other) noexcept;
			Registration& operator=(Registration&& other) noexcept;
			Registration(const Registration&) = delete;
			Registration& operator=(const Registration&) = delete;

			// 登録を解除する
			void Reset();

		private:
			std::weak_ptr<DynamicBufferRegistry> registry_;
			uint32_t id_ = kInvalidId;
		};

	public:
		DynamicBufferRegistry() = default;
		~DynamicBufferRegistry() = default;

		// 初期化
		void Initialize(ID3D12Device5* device);

		/// <summary>
		/// 変化したバッファの転送コマンドを記録して閉じる
		/// </summary>
		/// <param name="uploader">転送元の領域を確保するアップローダー</param>
		/// <returns>実行するコマンドリスト。転送するものが無ければnullptr</returns>
		ID3D12CommandList* RecordUploads(FrameUploader& uploader);

		/// <summary>
		/// 転送元を登録する。返したトークンが破棄されるまで転送対象になる
		/// </summary>
		[[nodiscard]] Registration Register(IUploadSource& source);

	private:
		DynamicBufferRegistry(const DynamicBufferRegistry&) = delete;
		DynamicBufferRegistry& operator=(const DynamicBufferRegistry&) = delete;

		// 無効な登録番号
		static constexpr uint32_t kInvalidId = UINT32_MAX;

		// フレームスロットごとのコマンドアロケータ
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocators_[kFrameCount];

		// 転送用のコマンドリスト
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

		// 転送対象。登録番号が添字で、解除した枠はnullptr
		std::vector<IUploadSource*> slots_;
		// 空いている登録番号
		std::vector<uint32_t> freeIds_;

	private:

		// 登録を解除する
		void Unregister(uint32_t id);
	};
}
