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

    // 輝度マスク
    class HighLumMask : public IPostEffect {
        struct HighLumMaskData {
            uint32_t textureHandle;
            float highLumMask; // マスク範囲
            float padding[2];
        };

    public:
        HighLumMask();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

    private:
        ConstantBuffer<HighLumMaskData> buffer_;
    };

    // 縦のぼかし
    class GaussVertical : public IPostEffect {
        struct GaussianBlurData {
            uint32_t textureHandle;
            float sd; // 標準偏差
            float padding[2];
        };

    public:
        GaussVertical();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

    private:
        ConstantBuffer<GaussianBlurData> buffer_;
    };

    // 横のぼかし
    class GaussHorizontal : public IPostEffect {
        struct GaussianBlurData {
            uint32_t textureHandle;
            float sd; // 標準偏差
            float padding[2];
        };

    public:
        GaussHorizontal();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

    private:
        ConstantBuffer<GaussianBlurData> buffer_;
    };

    // ブルーム
    class Bloom : public IPostEffect {
        struct BloomData {
            uint32_t blurTextureHandle;
            uint32_t gameTextureHandle;
            float intensity;
            float pad;
        };

    public:
        Bloom();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->blurTextureHandle = index;
        }

        void SetGamePassIndex(uint32_t index) {
            buffer_.GetData()->gameTextureHandle = index;
        }

    private:
        ConstantBuffer<BloomData> buffer_;
    };
}
