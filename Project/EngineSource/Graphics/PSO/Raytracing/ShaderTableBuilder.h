#pragma once
#include "ShaderTable.h"
#include <d3d12.h>
#include <wrl.h>
#include <stdexcept>

namespace GameEngine {

    class ShaderTableBuilder {
    public:

        // 各テーブルへのアクセス（外から AddRecord を呼んで積み上げる）
        ShaderTable& RayGen() { return raygenTable_; }
        ShaderTable& Miss() { return missTable_; }
        ShaderTable& HitGroup() { return hitgroupTable_; }

        // GPU バッファを生成してテーブルデータを書き込む
        void Build(ID3D12Device* device) {
            // RayGen は必ず 1 レコードのみ
            if (raygenTable_.RecordCount() != 1) {
                throw std::logic_error(
                    "ShaderTableBuilder: RayGen table must have exactly 1 record.");
            }

            // 各テーブルのサイズを D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT に揃える
            UINT raygenSize = Align(raygenTable_.TableSize(), TABLE_ALIGN);
            UINT missSize = Align(missTable_.TableSize(), TABLE_ALIGN);
            UINT hitgroupSize = Align(hitgroupTable_.TableSize(), TABLE_ALIGN);
            UINT totalSize = raygenSize + missSize + hitgroupSize;

            // UPLOAD ヒープに GPU バッファを作成する
            buffer_ = CreateUploadBuffer(device, totalSize);

            // バッファにマップして書き込む
            uint8_t* mapped = nullptr;
            buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

            raygenTable_.WriteAll(mapped);
            missTable_.WriteAll(mapped + raygenSize);
            hitgroupTable_.WriteAll(mapped + raygenSize + missSize);

            buffer_->Unmap(0, nullptr);

            // GPU アドレスを記録しておく（DispatchRaysDesc 用）
            D3D12_GPU_VIRTUAL_ADDRESS base = buffer_->GetGPUVirtualAddress();
            raygenAddr_ = base;
            missAddr_ = base + raygenSize;
            hitgroupAddr_ = base + raygenSize + missSize;
        }

        // Build() 後に DispatchRaysDesc を生成する
        D3D12_DISPATCH_RAYS_DESC CreateDispatchRaysDesc(UINT width, UINT height) const {
            D3D12_DISPATCH_RAYS_DESC desc{};

            // RayGen: ストライドは不使用（常に SizeInBytes と同じでよい）
            desc.RayGenerationShaderRecord.StartAddress = raygenAddr_;
            desc.RayGenerationShaderRecord.SizeInBytes = raygenTable_.RecordStride();

            // Miss
            desc.MissShaderTable.StartAddress = missAddr_;
            desc.MissShaderTable.SizeInBytes = missTable_.TableSize();
            desc.MissShaderTable.StrideInBytes = missTable_.RecordStride();

            // HitGroup
            desc.HitGroupTable.StartAddress = hitgroupAddr_;
            desc.HitGroupTable.SizeInBytes = hitgroupTable_.TableSize();
            desc.HitGroupTable.StrideInBytes = hitgroupTable_.RecordStride();

            desc.Width = width;
            desc.Height = height;
            desc.Depth = 1;

            return desc;
        }

        ID3D12Resource* GetBuffer() const { return buffer_.Get(); }

    private:
        ShaderTable raygenTable_;
        ShaderTable missTable_;
        ShaderTable hitgroupTable_;

        Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
        D3D12_GPU_VIRTUAL_ADDRESS raygenAddr_ = 0;
        D3D12_GPU_VIRTUAL_ADDRESS missAddr_ = 0;
        D3D12_GPU_VIRTUAL_ADDRESS hitgroupAddr_ = 0;

        static constexpr UINT TABLE_ALIGN =
            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

        static UINT Align(UINT size, UINT alignment) {
            return (size + alignment - 1) & ~(alignment - 1);
        }

        static Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(
            ID3D12Device* device, UINT size)
        {
            D3D12_HEAP_PROPERTIES heapProps{};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC resDesc{};
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resDesc.Width = size;
            resDesc.Height = 1;
            resDesc.DepthOrArraySize = 1;
            resDesc.MipLevels = 1;
            resDesc.SampleDesc.Count = 1;
            resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
            HRESULT hr = device->CreateCommittedResource(
                &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                IID_PPV_ARGS(&buffer));

            assert(SUCCEEDED(hr) && "Failed to create ShaderTable buffer");
            return buffer;
        }
    };

}