#pragma once
#include <vector>
#include <map>
#include <unordered_map>

#include "DrawRequest.h"
#include "RenderPass/RenderPassController.h"

#include "RefBuffer.h"
#include "Camera.h"
#include "LightManager.h"
#include "DirectionalLight.h"
#include "TLAS.h"

#include "PSO/Core/BlendMode.h"

namespace GameEngine {

    /// <summary>
    /// 溜めた描画コマンドを解放する機能
    /// </summary>
    class RenderQueue {
    public:
        struct WBOITData {
            float nearPlane; // ニアプレーン距離
            float farPlane; // ファープレーン距離
            float alphaThreshold; // 棄却アルファ閾値
            float depthPow; // 深度感度指数

            float weightMin; // 重みの下限
            float weightMax; // 重みの上限
            float pad[2];
        };
    public:
        RenderQueue();
        ~RenderQueue() = default;

        // 初期化処理
        void Initialize();

        void Update();
     
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

        const bool& GetUseDebugCamera() const { return useDebugCamera_; }

        // カメラリソースを取得
        GpuResource* GetCameraResource() { return mainCamera_.GetConstantBuffer(); }

        // カメラを取得
        Camera& GetMainCamera() { return mainCamera_; }

        // デバックカメラリソースを取得
        GpuResource* GetDebugCameraResource() { return debugCameraResource_; }

        // ライトリソースを取得
        GpuResource* GetLightResource() { return lightManager_.GetConstantBuffer(); }

        // ライト管理機能を取得
        LightManager* GetLightManager() { return &lightManager_; }

        // wboitリソースを取得
        GpuResource* GetWboitResource() { return &wboitData_; }

        // 背景画像ハンドルを設定する
        void SetSkyboxTexture(const uint32_t& texture) {
            skyboxTextureIndex_ = texture;
            lightManager_.SetEnvironmentTexture(texture);
        }

        const uint32_t& GetSkyboxTexture() const { return skyboxTextureIndex_; }

    public:

        // 画像描画
        void SubmitSprite(const Sprite* sprite, const std::string& passName = "DefaultPass");

        /// 通常モデル（ライトあり）
        void SubmitModel(const Model* model,WorldTransform& worldTransform,const float& alpha = 1.0f, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        /// インスタンシング描画
        void SubmitInstancing(const Model* model,uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha = 1.0f, BlendMode blendMode = BlendMode::kBlendModeNormal, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        void SubmitInstancingWboit(const Model* model,uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha = 1.0f, BlendMode blendMode = BlendMode::kBlendModeNormal, const GpuResource* material = nullptr, const std::string& passName = "WBOITAccumulatePass");

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
        void SubmitRaytracingModel(Model* model, WorldTransform& worldTransform, RefBuffer* customRefBuffer = nullptr);

        // psoの名前を取得
        const char* Get3dPsoName(Draw3dType type);
        const char* Get2dPsoName(Draw2dType type);

        // 描画コマンドのクリア
        void Clear() {
            draw2dQueueList_.clear();
            draw3dQueueList_.clear();
            translucentDrawQueueList_.clear();
            raytracingDrawQueueList_.clear();
        }

        const std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw3dRequest>>>>& GetDraw3dQueue() const { return draw3dQueueList_; }
        const std::map<std::string, std::vector<Draw3dRequest>>& GetTranslucentQueue() const { return translucentDrawQueueList_; }
        const std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw2dRequest>>>>& GetDraw2dQueue() const { return draw2dQueueList_; }
        const std::vector<TLASInstanceData>& GetRaytracingQueue()  const { return raytracingDrawQueueList_; }

        /// レイトレーシングに積まれた描画コマンドが存在するか
        bool HasRaytracingDrawCalls() const { return !raytracingDrawQueueList_.empty(); }

    private:
        // コピー、ムーブは禁止
        RenderQueue(const RenderQueue&) = delete;
        RenderQueue& operator=(const RenderQueue&) = delete;
        RenderQueue(RenderQueue&&) = default;
        RenderQueue& operator=(RenderQueue&&) = default;

        // 2D描画コマンドのスタックメモリ [描画パス]->[PSO]->[描画コマンド]
        std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw2dRequest>>>> draw2dQueueList_;
        // 3D描画コマンドのスタックメモリ [描画パス]->[PSO]->[描画コマンド]
        std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw3dRequest>>>> draw3dQueueList_;
        // 半透明の描画コマンドのスタックメモリ
        std::map<std::string, std::vector<Draw3dRequest>> translucentDrawQueueList_;
        // レイトレーシングの描画コマンド
        std::vector<TLASInstanceData> raytracingDrawQueueList_;

        // カメラリソース
        Camera mainCamera_;
        Camera* cameraPtr_ = nullptr;
        GpuResource* debugCameraResource_ = nullptr;
        // ライトリソース
        LightManager lightManager_;
        // 平行光源
        DirectionalLight::DirectionalLightData directionalData_;
        // 背景画像ハンドル
        uint32_t skyboxTextureIndex_ = 0;

        // wboitデータ
        ConstantBuffer<WBOITData> wboitData_;

        // デバックカメラを使用するか
        bool useDebugCamera_ = false;
    };
}