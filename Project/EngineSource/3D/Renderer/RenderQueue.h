#pragma once
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

#include "PSO/Core/DrawPSOData.h"
#include "DrawRequest.h"
#include "RenderPass/RenderPassController.h"

namespace GameEngine {

    class PSOManager;

    /// <summary>
    /// 溜めた描画コマンドを解放する機能
    /// </summary>
    class RenderQueue {
    public:
        RenderQueue();
        ~RenderQueue() = default;

        // 初期化処理
        void Initialize(ID3D12GraphicsCommandList* commandList, PSOManager* psoManager, RenderPassController* renderPassController);

        // フレーム開始前処理
        void Begin();

        // 描画コマンドを解放する
        void Execute();

        // 更新処理
        void Update();

    public:

        void SetCamera(GpuResource* cameraResource) {
            cameraResource_ = cameraResource;
        }

        void SetLight(GpuResource* lightResource) {
            lightResource_ = lightResource;
        }

        void SetLightCamera(ID3D12Resource* resource) {
            lightCameraResource_ = resource;
        }

    public:

        /// 通常モデル（ライトあり）
        void SubmitModel(const Model* model,WorldTransform& worldTransform,const float& alpha = 1.0f, const GpuResource* material = nullptr);

        /// インスタンシング描画
        void SubmitInstancing(const Model* model,uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha = 1.0f, const GpuResource* material = nullptr);

        /// スケルタルアニメーション
        void SubmitAnimation(const Model* model, WorldTransform& worldTransform, const float& alpha = 1.0f, const GpuResource* material = nullptr);

        /// スカイボックス
        void SubmitSkybox(const Model* model, WorldTransform& worldTransform, const GpuResource* material = nullptr);

        /// シャドウマップ
        void SubmitShadowMap(const Model* model, WorldTransform& worldTransform);

        /// グリッド
        void SubmitGrid(const Model* model,WorldTransform& worldTransform);

    private:
        ID3D12GraphicsCommandList* commandList_ = nullptr;
        RenderPassController* renderPassController_ = nullptr;

        // 描画コマンドのスタックメモリ
        std::map<RenderLayer, std::unordered_map<std::string, std::vector<DrawRequest>>> drawQueueList_;
        // 半透明の描画コマンドのスタックメモリ
        std::vector<DrawRequest> translucentDrawQueueList_;

        // psoのリスト
        std::unordered_map<std::string, DrawPsoData> psoList_;

        GpuResource* cameraResource_ = nullptr;
        GpuResource* lightResource_ = nullptr;
        ID3D12Resource* lightCameraResource_ = nullptr;

        // 現在のpso
        std::string currentPsoName_;
    private:
       /* /// <summary>
        /// PSOを動的に登録する。
        /// </summary>
        void RegisterPSO(const std::string& name, const DrawPsoData& psoData) {
            psoList_[name] = psoData;
        }*/

        /// <summary>
        /// PSOManagerから名前を指定して動的に登録する。
        /// </summary>
        void RegisterPSO(const std::string& name, PSOManager* psoManager);

        // 文字列キーでPSOをセット
        void PreDraw(const std::string& psoName);

        // 描画コマンドのクリア
        void Clear() {
            drawQueueList_.clear();
            translucentDrawQueueList_.clear();
            currentPsoName_.clear();
        }

        // psoの名前を取得
        const char* GetPsoName(DrawType type);

        // 描画
        void ExecuteRequest(const DrawRequest& request);
    };
}