#pragma once
#include<array>
#include <wrl.h>
#include <d3d12.h>

#include"Externals/DirectXTex/d3dx12.h"

#include"PostProcess/BloomPSO.h"
#include"PostProcess/OutLinePSO.h"

#include"PSO/Core/PSOManager.h"
#include"PSO/Core/DrawPSOData.h"

#include"PostEffectData.h"
#include"ParameterResource.h"

#include"SrvManager.h"

namespace GameEngine {

    class PostEffectManager {
    public:

        // ポストエフェクトの描画モード
        enum class DrawMode {
            Default, // 通常の描画(ブルームとヴィネット)
            ScanLine, // ラインの描画
            RadialBlur, // 中心に集中するぼかし
        };

        // PSOのデータ
        enum class PSOType {
            Vignetting,
            RadialBlur,
            ScanLine,

            MaxCount
        };

        // ポストエフェクトデータ
        struct EffectData {
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
            CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle{};
            uint32_t srvIndex;

            DrawPsoData* psoData; // 描画前処理のデータ
        };

    public:

        /// <summary>
        /// 静的初期化
        /// </summary>
        /// <param name="bloomPSO"></param>
        /// <param name="logManager"></param>
        static void StaticInitialize(BloomPSO* bloomPSO, OutLinePSO* outLinePSO, PSOManager* psoManager);

        /// <summary>
        /// 初期化
        /// </summary>
        /// <param name="device"></param>
        /// <param name="clearColor_"></param>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <param name="descriptorSizeRTV"></param>
        /// <param name="descriptorSizeSRV"></param>
        /// <param name="srvHeap_"></param>
        void Initialize(ID3D12Device* device, float clearColor_[4], uint32_t width, uint32_t height, uint32_t descriptorSizeRTV, SrvManager* srvManager);

        /// <summary>
        /// 描画前処理
        /// </summary>
        /// <param name="commandList"></param>
        /// <param name="viewport"></param>
        /// <param name="scissorRect"></param>
        /// <param name="clearColor"></param>
        /// <param name="dsvHeap"></param>
        void PreDraw(ID3D12GraphicsCommandList* commandList, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect, float clearColor[4], ID3D12DescriptorHeap* dsvHeap);

        /// <summary>
        /// 描画後処理
        /// </summary>
        /// <param name="commandList"></param>
        /// <param name="viewport"></param>
        /// <param name="scissorRect"></param>
        void PostDraw(ID3D12GraphicsCommandList* commandList, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect, D3D12_GPU_DESCRIPTOR_HANDLE depthSRV);

        /// <summary>
        /// SRVを取得
        /// </summary>
        /// <returns></returns>
        CD3DX12_GPU_DESCRIPTOR_HANDLE& GetSRVHandle();

        /// <summary>
        /// 描画モードを設定する
        /// </summary>
        /// <param name="drawMode"></param>
        void SetDrawMode(DrawMode drawMode) { drawMode_ = drawMode; }

        /// <summary>
        /// 現在の描画形態を取得
        /// </summary>
        /// <returns></returns>
        DrawMode GetDrawMode() { return drawMode_; }

    public:

        // ブルーム用PSO
        static BloomPSO* bloomPSO_;
        uint32_t bloomIndex_ = 0;

        // アウトライン用のPSO
        static OutLinePSO* outLinePSO_;

        // psoデータのリスト
        static std::array<DrawPsoData, static_cast<size_t>(PSOType::MaxCount)> psoList_;

        ParameterResource<RadialBlurData> radialBlurResource_;

    private:

        // デバイス
        ID3D12Device* device_ = nullptr;

        // ポストエフェクト用RTVの総数
        static inline const uint32_t kRTVNum = 16;

        // Clearの最適値
        D3D12_CLEAR_VALUE clearValue_{};

        // ポストエフェクトのためのRTVを計測
        uint32_t rtvIndex_ = 0;

        // コピー描画用リソース
        Microsoft::WRL::ComPtr<ID3D12Resource> DrawObjectResource_;

        // ポストエフェクト用のRTVヒープ
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> postProcessRTVHeap_;

        // ハンドル
        uint32_t drawObjectIndex_ = 0;

        // オブジェクトを描画する用のRTV
        D3D12_CPU_DESCRIPTOR_HANDLE drawObjectRTVHandle_;

        // オブジェクトを描画する用のSRV
        CD3DX12_GPU_DESCRIPTOR_HANDLE drawObjectSRVHandle_;

        // 描画するモード
        DrawMode drawMode_ = DrawMode::Default;

        // srvを管理する
        SrvManager* srvManager_;

        // 最終的に出力するsrvHandle
        CD3DX12_GPU_DESCRIPTOR_HANDLE resultSRVHandle_;

    private:

        // ブルーム用のRTVハンドル
        D3D12_CPU_DESCRIPTOR_HANDLE bloomRTVHandle_[4]{};

        // SRVハンドル
        CD3DX12_GPU_DESCRIPTOR_HANDLE bloomSRVHandle_[4];

        Microsoft::WRL::ComPtr<ID3D12Resource> bloomBrightResource_;     // 明るい部分抽出用
        Microsoft::WRL::ComPtr<ID3D12Resource> bloomBlurShrinkResource_; // 縮小させながらブラーをする
        Microsoft::WRL::ComPtr<ID3D12Resource> bloomResultResource_;     // 最終敵なもの
        Microsoft::WRL::ComPtr<ID3D12Resource> bloomCompositeResource_;  // 合成用

        // スキャンラインのデータ
        EffectData scanLineData_;
        ParameterResource<ScanLineData> scanLineResource_;

        // ヴィネットのデータ
        EffectData vignettingData_;
        ParameterResource<VignettingData> vignettingResource_;

        // ラジアルブラーのデータ
        EffectData radialBlurData_;

        // アウトラインのデータ
        EffectData outLineData_;

    private:

        /// <summary>
        /// ブルーム処理をするためのRTVを設定
        /// </summary>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <param name="descriptorSizeSRV"></param>
        /// <param name="descriptorSizeRTV"></param>
        void InitializeBloom(uint32_t width, uint32_t height, uint32_t descriptorSizeRTV);

        /// <summary>
        /// ブルームの描画処理
        /// </summary>
        /// <param name="commandList"></param>
        /// <param name="baseViewport"></param>
        /// <param name="baseScissorRect"></param>
        void DrawBloom(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE currentSrv, const D3D12_VIEWPORT& baseViewport, const D3D12_RECT& baseScissorRect);

        /// <summary>
        /// ポストエフェクトのデータを初期化する
        /// </summary>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <param name="descriptorSizeRTV"></param>
        void InitializePostEffectData(uint32_t width, uint32_t height, uint32_t descriptorSizeRTV);

        /// <summary>
        /// アウトラインの描画
        /// </summary>
        /// <param name="commandList"></param>
        /// <param name="depthSRV"></param>
        void DrawOutLine(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE depthSRV, D3D12_GPU_DESCRIPTOR_HANDLE currentSrv);

        /// <summary>
        /// エフェクトを描画する
        /// </summary>
        /// <param name="commandList"></param>
        /// <param name="data"></param>
        /// <param name="resource"></param>
        void DrawEffect(ID3D12GraphicsCommandList* commandList, EffectData data, ID3D12Resource* resource);

        // ポストエフェクトを仕様するためのリソースを作成する
        void CreatePostEffectResources(
            ID3D12DescriptorHeap* rtvHeap,
            uint32_t& rtvIndex,
            uint32_t descriptorSizeRTV,
            uint32_t width,
            uint32_t height,
            Microsoft::WRL::ComPtr<ID3D12Resource>& resource,   // 作成するリソース
            D3D12_CPU_DESCRIPTOR_HANDLE& rtvHandle,             // 作成するrtvHandle
            D3D12_GPU_DESCRIPTOR_HANDLE& srvGpuHandle,           // 作成するsrvHandle
            uint32_t& srvIndex  // ヒープ上に存在するインデックス
        );
    };
}