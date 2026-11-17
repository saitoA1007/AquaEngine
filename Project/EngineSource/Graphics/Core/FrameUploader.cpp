#include "pch.h"
#include "FrameUploader.h"
#include <cassert>
#include <algorithm>
#include "CreateBufferResource.h"
#include "Debug/DebugName.h"
using namespace GameEngine;

namespace {
	size_t AlignUp(size_t value, size_t alignment) {
		return (value + alignment - 1) & ~(alignment - 1);
	}
}

FrameUploader::~FrameUploader() {
	for (auto& frame : frames_) {
		for (auto& page : frame.pages) {
			page.resource->Unmap(0, nullptr);
		}
	}
}

void FrameUploader::Initialize(ID3D12Device5* device) {
	device_ = device;

	// 最初のページを用意しておく
	for (auto& frame : frames_) {
		CreatePage(frame, kPageSize);
	}
}

FrameUploader::Page& FrameUploader::CreatePage(FrameData& frame, size_t sizeInBytes) {
	Page page;
	page.size = (sizeInBytes > kPageSize) ? AlignUp(sizeInBytes, 64 * 1024) : kPageSize;
	page.resource = CreateBufferResource(device_, page.size);
	// Mapの出力はD3D12の都合で型の無いポインタなので、ここでバイト列として受け取る
	void* mapped = nullptr;
	page.resource->Map(0, nullptr, &mapped);
	page.cpu = static_cast<std::byte*>(mapped);
	SetDebugName(page.resource.Get(), "FrameUploadPage", static_cast<uint32_t>(frame.pages.size()));
	frame.pages.push_back(std::move(page));
	return frame.pages.back();
}

UploadRegion FrameUploader::AllocateBytes(size_t sizeInBytes, size_t alignment) {
	assert(device_ != nullptr && "FrameUploaderが初期化されていません");

	FrameData& frame = frames_[FrameContext::GetFrameIndex()];

	// 現在のページから順に入るページを探す
	for (; frame.currentPage < frame.pages.size(); ++frame.currentPage) {
		Page& page = frame.pages[frame.currentPage];
		size_t offset = AlignUp(page.offset, alignment);
		if (offset + sizeInBytes <= page.size) {
			page.offset = offset + sizeInBytes;
			return { std::span<std::byte>(page.cpu + offset, sizeInBytes), page.resource.Get(), offset, page.resource->GetGPUVirtualAddress() + offset };
		}
	}

	// 入るページが無ければ追加する
	Page& page = CreatePage(frame, sizeInBytes);
	frame.currentPage = frame.pages.size() - 1;
	page.offset = sizeInBytes;
	return { std::span<std::byte>(page.cpu, sizeInBytes), page.resource.Get(), 0, page.resource->GetGPUVirtualAddress() };
}

UploadRegion FrameUploader::UploadBytes(std::span<const std::byte> source) {
	UploadRegion region = AllocateBytes(source.size(), 16);
	std::ranges::copy(source, region.bytes.begin());
	return region;
}

void FrameUploader::BeginFrame(uint32_t frameIndex) {
	// このスロットの前回のGPU実行は完了しているので、領域を最初から使い直す
	FrameData& frame = frames_[frameIndex];
	for (auto& page : frame.pages) {
		page.offset = 0;
	}
	frame.currentPage = 0;
}
