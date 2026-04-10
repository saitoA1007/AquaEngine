#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include"DXC.h"
#include"Vector4.h"
#include"Vector2.h"

#include"LogManager.h"

namespace GameEngine {

    class ParticleCSPSO {
    public:
        ParticleCSPSO() = default;
        ~ParticleCSPSO() = default;

        void Initialize(ID3D12Device* device, DXC* dxc, LogManager* logManager);

        ID3D12RootSignature* GetComputeRootSignature() const { return computeRootSignature_.Get(); }
        ID3D12PipelineState* GetComputePipelineState() const { return computePipelineState_.Get(); }

        ID3D12RootSignature* GetRootSignature() const { return computeRootSignature_.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return computePipelineState_.Get(); }

    private:
        ParticleCSPSO(const ParticleCSPSO&) = delete;
        ParticleCSPSO& operator=(const ParticleCSPSO&) = delete;

        // コンピュート
        Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> computePipelineState_;

        // グラフィック
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

    private:

        void CreateCS(ID3D12Device* device, DXC* dxc, LogManager* logManager);

        void CreateVSPS(ID3D12Device* device, DXC* dxc, LogManager* logManager);
    };
}