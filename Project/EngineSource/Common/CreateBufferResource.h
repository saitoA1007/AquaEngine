#pragma once
#include <d3d12.h>
#include <wrl.h>

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device5* device, size_t sizeInBytes,
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD,
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ,
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);