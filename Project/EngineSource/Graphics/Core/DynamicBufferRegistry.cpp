#include "pch.h"
#include "DynamicBufferRegistry.h"
#include <cassert>
#include <utility>
#include "FrameUploader.h"
#include "Debug/DebugName.h"
using namespace GameEngine;

DynamicBufferRegistry::Registration::Registration(std::weak_ptr<DynamicBufferRegistry> registry, uint32_t id)
	: registry_(std::move(registry)), id_(id) {
}

DynamicBufferRegistry::Registration::~Registration() {
	Reset();
}

DynamicBufferRegistry::Registration::Registration(Registration&& other) noexcept
	: registry_(std::move(other.registry_)), id_(std::exchange(other.id_, kInvalidId)) {
}

DynamicBufferRegistry::Registration& DynamicBufferRegistry::Registration::operator=(Registration&& other) noexcept {
	if (this != &other) {
		Reset();
		registry_ = std::move(other.registry_);
		id_ = std::exchange(other.id_, kInvalidId);
	}
	return *this;
}

void DynamicBufferRegistry::Registration::Reset() {
	if (id_ == kInvalidId) { return; }

	// 登録先が既に破棄されていれば解除は不要
	if (auto registry = registry_.lock()) {
		registry->Unregister(id_);
	}
	registry_.reset();
	id_ = kInvalidId;
}

void DynamicBufferRegistry::Initialize(ID3D12Device5* device) {
	for (uint32_t i = 0; i < kFrameCount; ++i) {
		HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocators_[i]));
		assert(SUCCEEDED(hr));
		SetDebugName(allocators_[i].Get(), "UploadCommandAllocator", i);
	}

	HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocators_[0].Get(), nullptr, IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));
	SetDebugName(commandList_.Get(), "UploadCommandList");
	// 記録状態で作られるので閉じておく
	commandList_->Close();
}

ID3D12CommandList* DynamicBufferRegistry::RecordUploads(FrameUploader& uploader) {
	ID3D12CommandAllocator* allocator = allocators_[FrameContext::GetFrameIndex()].Get();
	bool isRecording = false;

	for (IUploadSource* source : slots_) {
		// 解除済みの枠
		if (source == nullptr) { continue; }

		std::optional<ByteRange> dirty = source->FindDirtyRange();
		if (!dirty) { continue; }

		// 最初に転送が必要になった時点で記録を開始する
		if (!isRecording) {
			HRESULT hr = allocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList_->Reset(allocator, nullptr);
			assert(SUCCEEDED(hr));
			isRecording = true;
		}

		// 変化した範囲をアップロード領域へ書き込み、DEFAULTバッファへコピーする
		UploadRegion upload = uploader.UploadBytes(source->GetBytes(*dirty));
		commandList_->CopyBufferRegion(source->GetDestination(), dirty->begin, upload.resource, upload.offset, dirty->Size());

		source->MarkUploaded(*dirty);
	}

	if (!isRecording) { return nullptr; }

	// バッファはExecuteCommandListsの終わりにCOMMONへ戻るため、状態遷移は不要
	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));
	return commandList_.Get();
}

DynamicBufferRegistry::Registration DynamicBufferRegistry::Register(IUploadSource& source) {
	// 空いている番号があれば使い直す
	uint32_t id = 0;
	if (!freeIds_.empty()) {
		id = freeIds_.back();
		freeIds_.pop_back();
		slots_[id] = &source;
	} else {
		id = static_cast<uint32_t>(slots_.size());
		slots_.push_back(&source);
	}
	return Registration(weak_from_this(), id);
}

void DynamicBufferRegistry::Unregister(uint32_t id) {
	assert(id < slots_.size() && slots_[id] != nullptr && "登録されていない番号です");
	slots_[id] = nullptr;
	freeIds_.push_back(id);
}
