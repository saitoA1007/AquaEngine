#pragma once
#include <string>

namespace GameEngine {

    class Model;
    class WorldTransform;
    class WorldTransforms;
    class GpuResource;

    // 描画レイヤー（実行優先順位。数値が小さいほど先に描画）
    enum class RenderLayer : uint32_t {
        Shadow = 0,
        Skybox = 10,
        Grid = 20,
        Opaque = 30,
        Animation = 40,
        Translucent = 100,
    };

    // モデルを描画するモード
    enum class DrawType {
        Default,        // 通常モデルを描画用
        DefaultAdd,
        Instancing,     // インスタンシング描画用
        InstancingAdd,
        Grid,           // グリッド描画用
        Animation,      // アニメーション描画用
        Skybox,         // スカイボックスの描画用
        ShadowMap,      // シャドウマップ用
    };

    // 描画に使用するリソース
    struct DrawRequest {
        DrawType type = DrawType::Default;
        RenderLayer layer = RenderLayer::Opaque;
        std::string passName = "DefaultPass";

        const Model* model = nullptr;
        WorldTransform* worldTransform = nullptr;
        // インスタンシング描画
        uint32_t numInstances = 1;
        WorldTransforms* worldTransforms = nullptr;

        // マテリアル
        const GpuResource* material = nullptr;
    };
}

 