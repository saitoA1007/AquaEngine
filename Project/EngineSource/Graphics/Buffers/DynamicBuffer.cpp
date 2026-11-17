#include "pch.h"
#include "DynamicBuffer.h"
#include <algorithm>
#include <cassert>
#include "CreateBufferResource.h"
#include "ResourceGarbageCollector.h"
using namespace GameEngine;

namespace {
	// 差分を探すときの比較単位
	constexpr size_t kCompareBlock = 256;

	// 2つのバイト列が等しいか
	bool IsSame(std::span<const std::byte> a, std::span<const std::byte> b) {
		return std::ranges::equal(a, b);
	}
}

DynamicBuffer::DynamicBuffer(ID3D12Device5* device, std::weak_ptr<DynamicBufferRegistry> registry, size_t sizeInBytes) {
	const size_t size = (sizeInBytes == 0) ? 1 : sizeInBytes;
	uploadSize_ = size;

	// GPUが読む本体。COMMONで作成し、使用時の暗黙的な状態昇格に任せる
	resource_ = CreateBufferResource(device, size,
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_FLAG_NONE);

	// CPUが書き込むメモリ。従来のアップロードヒープと同様に0で初期化しておく
	shadow_.assign(size, std::byte{ 0 });
	uploaded_.assign(size, std::byte{ 0 });
	assert(reinterpret_cast<uintptr_t>(shadow_.data()) % kShadowAlignment == 0 && "シャドウメモリのアライメントが足りません");

	// 登録
	auto registryPtr = registry.lock();
	assert(registryPtr && "DynamicBufferRegistryが設定されていません");
	if (registryPtr) {
		registration_ = registryPtr->Register(static_cast<IUploadSource&>(*this));
	}
}

DynamicBuffer::~DynamicBuffer() {
	// 先に転送対象から外す
	registration_.Reset();

	// GPUが参照し終わってから破棄する
	ResourceGarbageCollector::GetInstance().Add(std::move(resource_));
}

std::optional<ByteRange> DynamicBuffer::FindDirtyRange() const {
	// 初回はGPU側の中身が未定義なので全体を送る
	if (needsFullUpload_) {
		return ByteRange{ 0, shadow_.size() };
	}

	const std::span<const std::byte> current(shadow_.data(), uploadSize_);
	const std::span<const std::byte> previous(uploaded_.data(), uploadSize_);
	const size_t size = uploadSize_;

	// 先頭から変化した位置を探す
	size_t first = 0;
	while (first < size) {
		size_t n = (std::min)(kCompareBlock, size - first);
		if (!IsSame(current.subspan(first, n), previous.subspan(first, n))) { break; }
		first += n;
	}
	// 変化なし
	if (first >= size) { return std::nullopt; }

	// 末尾から変化した位置を探す
	size_t last = size;
	while (last > first) {
		size_t n = (std::min)(kCompareBlock, last - first);
		if (!IsSame(current.subspan(last - n, n), previous.subspan(last - n, n))) { break; }
		last -= n;
	}

	return ByteRange{ first, last };
}

std::span<const std::byte> DynamicBuffer::GetBytes(ByteRange range) const {
	return std::span<const std::byte>(shadow_).subspan(range.begin, range.Size());
}

void DynamicBuffer::MarkUploaded(ByteRange range) {
	std::ranges::copy(GetBytes(range), uploaded_.begin() + range.begin);
	needsFullUpload_ = false;
}
