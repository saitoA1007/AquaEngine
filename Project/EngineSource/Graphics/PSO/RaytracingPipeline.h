#pragma once
#include <d3d12.h>
#include "Externals/DirectXTex/d3dx12.h"
#include "DXC.h"
#include <wrl.h>
#include <cassert>

namespace GameEngine {

    class RaytracingPipeline {
    public:
        void Create(ID3D12Device5* device, ID3D12RootSignature* globalRootSig, IDxcBlob* compiledShader) {

            // 1. State Object の記述子（ヘルパーを使用）
            CD3DX12_STATE_OBJECT_DESC rtPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

            // ====================================================================
            // 2. DXIL ライブラリ (シェーダーの登録)
            // ====================================================================
            auto lib = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            D3D12_SHADER_BYTECODE libdxil = { compiledShader->GetBufferPointer(), compiledShader->GetBufferSize() };
            lib->SetDXILLibrary(&libdxil);

            // 使用する関数名（HLSL側の関数名）をエクスポートする
            lib->DefineExport(L"RayGen");
            lib->DefineExport(L"Miss");
            lib->DefineExport(L"ClosestHit");

            // ====================================================================
            // 3. ヒットグループの設定
            // ====================================================================
            auto hitGroup = rtPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitGroup->SetHitGroupExport(L"HitGroup"); // C++側(ShaderTable)で指定する名前
            hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitGroup->SetClosestHitShaderImport(L"ClosestHit"); // 結びつけるシェーダー名

            // ====================================================================
            // 4. シェーダー設定 (Payload と Attribute のサイズ)
            // ====================================================================
            auto shaderConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
            // ペイロード(色情報などレイが運ぶデータ)と、アトリビュート(重心座標など)の最大バイト数
            // ※必要最小限のサイズにしないとパフォーマンスが落ちます
            uint32_t payloadSize = 4 * sizeof(float);   // 例: float4 color
            uint32_t attributeSize = 2 * sizeof(float); // 例: float2 barycentrics
            shaderConfig->Config(payloadSize, attributeSize);

            // ====================================================================
            // 5. グローバルルートシグネチャの登録
            // ====================================================================
            auto globalRootSignature = rtPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            globalRootSignature->SetRootSignature(globalRootSig);

            // ====================================================================
            // 6. パイプライン設定 (レイの再帰深度)
            // ====================================================================
            auto pipelineConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
            // 1にすると「カメラからのレイ」のみ。反射（鏡など）を作るなら 2 や 3 にする
            pipelineConfig->Config(1);

            // ====================================================================
            // 7. State Object の生成
            // ====================================================================
            HRESULT hr = device->CreateStateObject(rtPipeline, IID_PPV_ARGS(&stateObject_));
            assert(SUCCEEDED(hr));
        }

        ID3D12StateObject* GetStateObject() const { return stateObject_.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject_;
    };
}