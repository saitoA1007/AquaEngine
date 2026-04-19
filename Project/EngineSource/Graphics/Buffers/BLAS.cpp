#include "BLAS.h"
#include "CreateBufferResource.h"
using namespace GameEngine;

void BLAS::Create(ID3D12GraphicsCommandList4* cmdList,
    const D3D12_VERTEX_BUFFER_VIEW& vertexBufView, const D3D12_INDEX_BUFFER_VIEW& indexBufView,
    const uint32_t& totalVertices, const uint32_t& totalIndices) {

    // ジオメトリ情報を設定
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    // 頂点データ
    geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBufView.BufferLocation;
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexBufView.StrideInBytes;
    geometryDesc.Triangles.VertexCount = totalVertices;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    // インデックスデータ
    geometryDesc.Triangles.IndexBuffer = indexBufView.BufferLocation;
    geometryDesc.Triangles.IndexCount = totalIndices;
    geometryDesc.Triangles.IndexFormat = indexBufView.Format;

    // 1つのMeshの入力設定
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL; // BLASを指定
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.pGeometryDescs = &geometryDesc;
    inputs.NumDescs = 1;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    // 必要なバッファサイズをGPUに計算させる
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
    device_->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    // BLASのデータリソース
    resource_ = CreateBufferResource(
        device_,
        prebuildInfo.ResultDataMaxSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,                                       
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, // DXR専用ステート  
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS                    
    );

    // 構築に使用する一時的な作業メモリ
    scratchBuffer_ = CreateBufferResource(
        device_,
        prebuildInfo.ScratchDataSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,                                    
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // UAVステート              
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS                  
    );

    // ビルド実行
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
    buildDesc.Inputs = inputs;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer_->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = resource_->GetGPUVirtualAddress();
    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // ビルド完了を待つUAVバリアを張る
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource_.Get();
    cmdList->ResourceBarrier(1, &barrier);
}