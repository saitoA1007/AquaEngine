#pragma once
#include <cstdint>
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "IPostEffect.h"

namespace GameEngine {

    /// <summary>
    /// ヴィネット
    /// </summary>
    class Vignetting : public IPostEffect {
    public:
        struct alignas(16) VignettingData {
            float intensity; // ぼかさない円の範囲
            float time; // ぼかしぐわい
            uint32_t textureHandle; // 加工する画像
            float padding;
        };
    public:
        Vignetting();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

    private:
        ConstantBuffer<VignettingData> buffer_;
    };

    /// <summary>
    /// ラジアルブラー
    /// </summary>
    class RadialBlur : public IPostEffect {
    public:
        struct alignas(16) RadialBlurData {
            Vector2 centerPos; // 中心点
            int32_t numSamles; // サンプリング数。大きい程滑らか
            float blurWidth; // ぼかしの幅
            uint32_t textureHandle; // 加工する画像
            float padding[3];
        };
    public:
        RadialBlur();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

    private:
        ConstantBuffer<RadialBlurData> buffer_;
    };

    /// <summary>
    /// スキャンライン
    /// </summary>
    class ScanLine : public IPostEffect {
    public:
        struct alignas(16) ScanLineData {
            float interval; // 間隔
            float time; // 時間
            float speed; // 速度
            float pad;
            Vector3 lineColor; // 線の色
            uint32_t textureHandle; // 加工する画像
        };
    public:
        ScanLine();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

    private:
        ConstantBuffer<ScanLineData> buffer_;
    };
}

