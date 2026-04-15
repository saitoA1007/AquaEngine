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
        void SubmitModel(const Model* model,WorldTransform& worldTransform,const float& alpha = 1.0f, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        /// インスタンシング描画
        void SubmitInstancing(const Model* model,uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha = 1.0f, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        /// スケルタルアニメーション
        void SubmitAnimation(const Model* model, WorldTransform& worldTransform, const float& alpha = 1.0f, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        /// スカイボックス
        void SubmitSkybox(const Model* model, WorldTransform& worldTransform, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        /// シャドウマップ
        void SubmitShadowMap(const Model* model, WorldTransform& worldTransform, const std::string& passName = "ShadowPass");

        /// グリッド
        void SubmitGrid(const Model* model,WorldTransform& worldTransform, const std::string& passName = "DefaultPass");

        // デバック用ライン
        void SubmitDebugLine(const DebugRenderer* debugRenderer, const std::string& passName = "DefaultPass");

    private:
        ID3D12GraphicsCommandList* commandList_ = nullptr;
        RenderPassController* renderPassController_ = nullptr;

        // 描画コマンドのスタックメモリ [描画パス]->[PSO]->[描画コマンド]
        std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<DrawRequest>>>> drawQueueList_;
        // 半透明の描画コマンドのスタックメモリ
        std::map<std::string, std::vector<DrawRequest>> translucentDrawQueueList_;

        // psoのリスト
        std::unordered_map<std::string, DrawPsoData> psoList_;

        // カメラリソース
        GpuResource* cameraResource_ = nullptr;
        // ライトリソース
        GpuResource* lightResource_ = nullptr;
        ID3D12Resource* lightCameraResource_ = nullptr;

        // 現在のpso
        std::string currentPsoName_;

        // 描画パスの実行順
        std::vector<std::string> passExecuteOrder_;

    private:
        /// <summary>
        /// PSOManagerから名前を指定して動的に登録する。
        /// </summary>
        void RegisterPSO(const std::string& name, PSOManager* psoManager);

        // 文字列キーでPSOをセット
        void PreDraw(const std::string& psoName);

        // パスの実行順を登録（Initialize時に呼ぶ）
        void RegisterPassOrder(const std::vector<std::string>& order) {
            passExecuteOrder_ = order;
        }

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