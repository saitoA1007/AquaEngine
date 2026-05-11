#pragma once
#include <vector>
#include <map>
#include <unordered_map>

#include "PSO/Core/DrawPSOData.h"
#include "DrawRequest.h"
#include "RenderPass/RenderPassController.h"
#include "TLAS.h"
#include "Camera.h"
#include "LightManager.h"
#include "DirectionalLight.h"

namespace GameEngine {

    // 前方宣言
    class PSOManager;
    class BufferRefManager;
    class RaytracingPipeline;

    /// <summary>
    /// 溜めた描画コマンドを解放する機能
    /// </summary>
    class RenderQueue {
    public:
        RenderQueue();
        ~RenderQueue() = default;

        // 初期化処理
        void Initialize(ID3D12GraphicsCommandList4* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController,
            RaytracingPipeline* raytracingPipeline, BufferRefManager* bufferRefManager);

        // フレーム開始前処理
        void Begin();

        // 描画コマンドを解放する
        void Execute();

    public:

        void SetCamera(Camera* camera) {
            cameraPtr_ = camera;
        }

        void SetDebugCamera(GpuResource* cameraResource) {
            debugCameraResource_ = cameraResource;  
        }

        void SetUseDebugCamera(const bool& useDebugCamera) {
            useDebugCamera_ = useDebugCamera;
        }

        // tlasを取得
        TLAS* GetTLAS() { return &tlas_; }

        // カメラリソースを取得
        GpuResource* GetCameraResource() { return mainCamera_.GetConstantBuffer(); }

        // ライトリソースを取得
        GpuResource* GetLightResource() { return lightManager_.GetConstantBuffer(); }

    public:

        // 画像描画
        void SubmitSprite(const Sprite* sprite, const std::string& passName = "DefaultPass");

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

        // レイトレーシングでのモデル
        void SubmitRaytracingModel(const Model* model, WorldTransform& worldTransform,const uint32_t* materialIndex = nullptr, const std::string& passName = "RaytracingPass");

    private:
        ID3D12GraphicsCommandList4* commandList_ = nullptr;
        RenderPassController* renderPassController_ = nullptr;
        RaytracingPipeline* raytracingPipeline_ = nullptr;
        SrvManager* srvManager_ = nullptr;
        BufferRefManager* bufferRefManager_ = nullptr;

        // 2D描画コマンドのスタックメモリ [描画パス]->[PSO]->[描画コマンド]
        std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw2dRequest>>>> draw2dQueueList_;
        // 3D描画コマンドのスタックメモリ [描画パス]->[PSO]->[描画コマンド]
        std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw3dRequest>>>> draw3dQueueList_;
        // 半透明の描画コマンドのスタックメモリ
        std::map<std::string, std::vector<Draw3dRequest>> translucentDrawQueueList_;
        // レイトレーシングの描画コマンド
        std::map<std::string,std::vector<TLASInstanceData>> raytracingDrawQueueList_;

        // カメラリソース
        Camera mainCamera_;
        Camera* cameraPtr_ = nullptr;
        GpuResource* debugCameraResource_ = nullptr;
        // ライトリソース
        LightManager lightManager_;
        // 平行光源
        DirectionalLight::DirectionalLightData directionalData_;

        // デバックカメラを使用するか
        bool useDebugCamera_ = false;

        // 描画パスの実行順
        std::vector<std::string> passExecuteOrder_;

        // 現在のpso
        std::string currentPsoName_;

        // psoのリスト
        std::unordered_map<std::string, DrawPsoData> psoList_;

        // レイトレーシング用の描画モデル管理
        TLAS tlas_;

        // レイトレーシングでの最大描画数
        uint32_t maxRayInstanceNum_ = 200;

        // bufferが存在しているsrvのスタート位置
        uint32_t bufferStartSrvIndex_ = 0;

        // 最終的に画面に描画させるパスの名前
        std::string finalPassName_ = "";

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
            draw3dQueueList_.clear();
            translucentDrawQueueList_.clear();
            currentPsoName_.clear();
            raytracingDrawQueueList_.clear();
        }

        // psoの名前を取得
        const char* Get3dPsoName(Draw3dType type);
        const char* Get2dPsoName(Draw2dType type);

        // 描画コマンドを解放
        void Execute3dRequest(const Draw3dRequest& request);
        void Execute2dRequest(const Draw2dRequest& request);

        // レイトレーシングの描画
        void DrawRaytracing();
    };
}