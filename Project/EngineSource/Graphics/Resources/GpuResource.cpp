#include "pch.h"
#include "GpuResource.h"
#include "DynamicBuffer.h"

using namespace GameEngine;

ID3D12Device5* GpuResource::device_ = nullptr;
std::weak_ptr<DynamicBufferRegistry> GpuResource::dynamicBufferRegistry_;

std::shared_ptr<DynamicBuffer> GpuResource::CreateDynamicBuffer(size_t sizeInBytes) {
	return std::make_shared<DynamicBuffer>(device_, dynamicBufferRegistry_, sizeInBytes);
}