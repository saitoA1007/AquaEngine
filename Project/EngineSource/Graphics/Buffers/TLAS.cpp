#include "TLAS.h"
#include "CreateBufferResource.h"
#include "ResourceGarbageCollector.h"
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

void TLAS::Create(ID3D12GraphicsCommandList4* cmdList, const std::vector<TLASInstanceData>& instances, bool isUpdate) {
    if (instances.empty()) { return; }

    uint32_t instanceCount = static_cast<uint32_t>(instances.size());
    uint64_t instanceBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount;

    // GPUに送るためのリソースを作成
    instanceBuffer_ = CreateBufferResource(
        device_, instanceBufferSize,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_FLAG_NONE
    );

    // インスタンス情報をD3D12の構造体にマッピングして書き込む
    D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = nullptr;
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));

    for (uint32_t i = 0; i < instanceCount; ++i) {
        instanceDescs[i].InstanceID = instances[i].instanceID;
        instanceDescs[i].InstanceContributionToHitGroupIndex = instances[i].hitGroupIndexOffset;
        instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

        // Transform行列のコピー
        std::memcpy(instanceDescs[i].Transform, instances[i].transform, sizeof(float) * 12);

        // 対象となるBLASのGPUアドレスを指定
        instanceDescs[i].AccelerationStructure = instances[i].blas->GetGpuVirtualAddress();

        // レイキャスト時のフィルタリング
        instanceDescs[i].InstanceMask = 0xFF;
    }
    instanceBuffer_->Unmap(0, nullptr);

    // ビルドの入力設定
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = instanceCount;
    inputs.InstanceDescs = instanceBuffer_->GetGPUVirtualAddress();
    if (isUpdate) {
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    } else {
        inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    }

    // 必要なメモリ量を求める
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuild{};
    device_->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &tlasPrebuild);

    // TLASのデータリソースを作成
    resource_ = CreateBufferResource(
        device_, tlasPrebuild.ResultDataMaxSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    // 一時的な作業リソースを作成
    Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer = CreateBufferResource(
        device_, tlasPrebuild.ScratchDataSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    // アップデート用バッファを確保
    if (inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE) {

        tlasUpdate_ = CreateBufferResource(
            device_, tlasPrebuild.UpdateScratchDataSizeInBytes,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        );
    }

    // ビルド実行
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
    buildDesc.Inputs = inputs;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = resource_->GetGPUVirtualAddress();
    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // バリア生成
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource_.Get();
    cmdList->ResourceBarrier(1, &barrier);

    // リソースの破棄を登録する
    ResourceGarbageCollector::GetInstance().Add(scratchBuffer);

    // SRVを作成
    srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = resource_->GetGPUVirtualAddress();

    auto srvHandleCPU = srvManager_->GetCPUHandle(srvIndex_);
    device_->CreateShaderResourceView(nullptr, &srvDesc, srvHandleCPU);

    isCreated_ = true;
}

void TLAS::Update(ID3D12GraphicsCommandList4* cmdList, const std::vector<TLASInstanceData>& instances) {
    if (!isCreated_) { return; }

    uint32_t instanceCount = static_cast<uint32_t>(instances.size());
    uint64_t instanceBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount;

    // インスタンス情報をD3D12の構造体にマッピングして書き込む
    D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = nullptr;
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));

    for (uint32_t i = 0; i < instanceCount; ++i) {
        instanceDescs[i].InstanceID = instances[i].instanceID;
        instanceDescs[i].InstanceContributionToHitGroupIndex = instances[i].hitGroupIndexOffset;
        instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

        // Transform行列のコピー
        std::memcpy(instanceDescs[i].Transform, instances[i].transform, sizeof(float) * 12);

        // 対象となるBLASのGPUアドレスを指定
        instanceDescs[i].AccelerationStructure = instances[i].blas->GetGpuVirtualAddress();

        // レイキャスト時のフィルタリング
        instanceDescs[i].InstanceMask = 0xFF;
    }
    instanceBuffer_->Unmap(0, nullptr);

    // ビルドの入力設定
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = instanceCount;
    inputs.InstanceDescs = instanceBuffer_->GetGPUVirtualAddress();
    // TLAS の更新処理を行うためのフラグを設定する.
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
        | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

    // ビルド実行
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
    buildDesc.Inputs = inputs;
    buildDesc.ScratchAccelerationStructureData = tlasUpdate_->GetGPUVirtualAddress();
    buildDesc.SourceAccelerationStructureData = resource_->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = resource_->GetGPUVirtualAddress();
    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // バリア生成
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource_.Get();
    cmdList->ResourceBarrier(1, &barrier);
}