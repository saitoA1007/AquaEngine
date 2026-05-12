#include "TLAS.h"
#include <cassert>
#include "CreateBufferResource.h"
using namespace GameEngine;

TLAS::~TLAS() {
    if (isCreated_) {
        resource_.Reset();
        instanceBuffer_.Reset();
        if (srvManager_) {
            srvManager_->ReleseIndex(srvIndex_);
        }
    }  
}

void TLAS::Create(ID3D12GraphicsCommandList4* cmdList, const uint32_t& maxInstanceNum) {
    maxInstanceCount_ = maxInstanceNum;
    uint64_t instanceBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * maxInstanceCount_;

    // GPUに送るためのリソースを作成
    instanceBuffer_ = CreateBufferResource(
        device_, instanceBufferSize,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_FLAG_NONE
    );

    // インスタンス情報をD3D12の構造体にマッピングして書き込む
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs_));

    // ビルドの入力設定
    inputs_.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs_.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs_.InstanceDescs = instanceBuffer_->GetGPUVirtualAddress();
    inputs_.NumDescs = maxInstanceCount_;
    inputs_.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    // バッファを生成
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
    device_->GetRaytracingAccelerationStructurePrebuildInfo(&inputs_, &prebuildInfo);

    // TLASのデータリソースを作成
    resource_ = CreateBufferResource(
        device_, prebuildInfo.ResultDataMaxSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    // 作業リソースを作成
    scratchBuffer_ = CreateBufferResource(
        device_, prebuildInfo.ScratchDataSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    // ビルド実行
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
    buildDesc.Inputs = inputs_;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer_->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = resource_->GetGPUVirtualAddress();
    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // バリア生成
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource_.Get();
    cmdList->ResourceBarrier(1, &barrier);

    // SRVを作成
    srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::AccessData);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = resource_->GetGPUVirtualAddress();

    srvHandleCPU_ = srvManager_->GetCPUHandle(srvIndex_);
    srvHandleGPU_ = srvManager_->GetGPUHandle(srvIndex_);
    device_->CreateShaderResourceView(nullptr, &srvDesc, srvHandleCPU_);

    isCreated_ = true;
}

void TLAS::Update(ID3D12GraphicsCommandList4* cmdList, const std::vector<TLASInstanceData>& instances) {
    assert(instances.size() <= maxInstanceCount_);

    uint32_t activeCount = static_cast<uint32_t>(instances.size());

    // 有効なインスタンスの状態を書き込む
    for (uint32_t i = 0; i < instances.size(); ++i) {
        instanceDescs_[i].InstanceID = instances[i].instanceID;
        instanceDescs_[i].InstanceContributionToHitGroupIndex = instances[i].hitGroupIndexOffset;
        instanceDescs_[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

        // Transform行列のコピー
        std::memcpy(instanceDescs_[i].Transform, instances[i].transform, sizeof(float) * 12);

        // 対象となるBLASのGPUアドレスを指定
        instanceDescs_[i].AccelerationStructure = instances[i].blas->GetGpuVirtualAddress();

        // レイキャスト時のフィルタリング
        instanceDescs_[i].InstanceMask = 0xFF;
    }

    // 使用していないスロットを無効
    for (uint32_t i = activeCount; i < maxInstanceCount_; ++i) {
        // レイキャストで無視される
        instanceDescs_[i].InstanceMask = 0x00; 
    }

    // TLASの再ビルドを実行する
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
    buildDesc.Inputs = inputs_;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer_->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = resource_->GetGPUVirtualAddress();
    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // バリア生成
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource_.Get();
    cmdList->ResourceBarrier(1, &barrier);
}