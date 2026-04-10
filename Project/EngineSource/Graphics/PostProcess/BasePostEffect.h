#pragma once
#include <d3d12.h>
#include"EngineSource/Core/DXC.h"

class BasePostEffect {
public:

    enum class PostEffectMode {
        GrayScale,
        Bloom,
        GaussianBlur,
    };

public:
    virtual ~BasePostEffect() = default;

    // 初期化処理（PSO、ルートシグネチャ、リソース作成など）
    virtual void Initialize(ID3D12Device* device) = 0;

    virtual void Resize(UINT width, UINT height) = 0;

    // エフェクトの適用
    // source: 入力となるレンダーターゲット
    // commandList: コマンドリスト
    // Returns: 出力となるレンダーターゲットのリソース
    virtual ID3D12Resource* Apply(ID3D12GraphicsCommandList* commandList, ID3D12Resource* source) = 0;

    // GUIでのパラメータ設定など
    virtual void OnGUI() {};

    // 解放処理
    virtual void Release() = 0;

    virtual PostEffectMode GetName() const = 0;
};