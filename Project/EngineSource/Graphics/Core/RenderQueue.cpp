#include "RenderQueue.h"
#include "PSO/Core/PSOManager.h"
#include "ModelRenderer.h"
#include "DebugRenderer.h"
#include "SpriteRenderer.h"
using namespace GameEngine;

RenderQueue::RenderQueue() {

}

void RenderQueue::Initialize(ID3D12GraphicsCommandList* commandList, PSOManager* psoManager, RenderPassController* renderPassController) {
    commandList_ = commandList;
    renderPassController_ = renderPassController;

    // 影を描画するパス
    renderPassController_->AddPass("ShadowPass", RenderTextureMode::DsvOnly, 2048, 2048);
    // デフォルトで描画するパス
    renderPassController_->AddPass("DefaultPass");
    // 最終的な描画先を設定
    renderPassController_->SetEndPass("DefaultPass");

    // 実行順序を設定
    RegisterPassOrder({ "ShadowPass", "DefaultPass" });

    // PSOを登録
    RegisterPSO("Default3D", psoManager);
    RegisterPSO("Additive3D", psoManager);
    RegisterPSO("Instancing3D", psoManager);
    RegisterPSO("AdditiveInstancing3D", psoManager);
    RegisterPSO("Animation", psoManager);
    RegisterPSO("Skybox", psoManager);
    RegisterPSO("ShadowMap", psoManager);
    RegisterPSO("Grid", psoManager);
    RegisterPSO("Line", psoManager);

    RegisterPSO("DefaultSprite", psoManager);
    RegisterPSO("AdditiveSprite", psoManager);
}

void RenderQueue::Begin() {
    // クリア
    Clear();
}

void RenderQueue::Execute() {

    for (const auto& passName : passExecuteOrder_) {

        // 不透明、半透明ともにコマンドがなければ飛ばす
        bool hasOpaque = draw3dQueueList_.count(passName) > 0;
        bool hasTranslucent = translucentDrawQueueList_.count(passName) > 0;
        bool has2d = draw2dQueueList_.count(passName) > 0;
        if (!hasOpaque && !hasTranslucent && !has2d) {
            renderPassController_->PrePass(passName);
            renderPassController_->PostPass(passName);
            continue;
        }

        renderPassController_->PrePass(passName);
        currentPsoName_.clear();

        // 不透明描画コマンドを解放
        if (hasOpaque) {
            for (auto& [layer, psoMap] : draw3dQueueList_[passName]) {
                for (auto& [psoName, requests] : psoMap) {
                    if (requests.empty()) { continue; }
                    // 描画前処理
                    PreDraw(psoName);
                    for (const auto& request : requests) {
                        // 描画コマンド解放
                        Execute3dRequest(request);
                    }
                }
            }
        }

        // 半透明描画コマンドを解放
        if (hasTranslucent) {
            auto& translucentList = translucentDrawQueueList_[passName];

            // カメラの距離でソートをおこなう
            // std::sort(translucentList.begin(), translucentList.end(),
            //     [](const DrawRequest& a, const DrawRequest& b) {
            //         return a.sortKey > b.sortKey;
            //     });

            for (const auto& request : translucentList) {
                // 描画前処理
                PreDraw(Get3dPsoName(request.type));
                // 描画コマンド解放
                Execute3dRequest(request);
            }
        }

        // 2D描画コマンドを解放
        if (has2d) {
            for (auto& [layer, psoMap] : draw2dQueueList_[passName]) {
                for (auto& [psoName, requests] : psoMap) {
                    if (requests.empty()) { continue; }
                    // 描画前処理
                    PreDraw(psoName);
                    for (const auto& request : requests) {
                        // 描画コマンド解放
                        Execute2dRequest(request);
                    }
                }
            }
        }

        renderPassController_->PostPass(passName);
    }
}

const char* RenderQueue::Get3dPsoName(Draw3dType type) {
    switch (type) {
        case Draw3dType::Default:         { return "Default3D"; }
        case Draw3dType::DefaultAdd:      { return "Default3D"; }
        case Draw3dType::Instancing:      { return "Instancing3D"; }
        case Draw3dType::InstancingAdd:   { return "AdditiveInstancing3D"; }
        case Draw3dType::Animation:       { return "Animation"; }
        case Draw3dType::Skybox:          { return "Skybox"; }
        case Draw3dType::ShadowMap:       { return "ShadowMap"; }
        case Draw3dType::Grid:            { return "Grid"; }
        case Draw3dType::DebugLine:       { return "Line"; }
        default:                          { return "Default3D"; }
    }
}

const char* RenderQueue::Get2dPsoName(Draw2dType type) {
    switch (type)
    {
    case GameEngine::Draw2dType::Normal: { return "DefaultSprite"; }
    case GameEngine::Draw2dType::Add:    { return "AdditiveSprite"; }
    default:                             { return "DefaultSprite"; }
    }
}

void RenderQueue::RegisterPSO(const std::string& name, PSOManager* psoManager) {
    psoList_[name] = psoManager->GetDrawPsoData(name);
}

// 戻り値で登録済みかどうかを返す
void RenderQueue::PreDraw(const std::string& psoName) {

    if (psoName == "ShadowMap") {
        ModelRenderer::SetCamera(lightCameraResource_);
    } else {
        if (useDebugCamera_) {
            // デバック用のカメラを設定
            ModelRenderer::SetCamera(debugCameraResource_->GetResource());
        } else {
            ModelRenderer::SetCamera(cameraResource_->GetResource());
        }
    }

    // 前回と同じPSOなら切り替え不要
    if (currentPsoName_ == psoName) { return; }

    auto it = psoList_.find(psoName);
    assert(it != psoList_.end() && "未登録のPSO名です");

    commandList_->SetGraphicsRootSignature(it->second.rootSignature);
    commandList_->SetPipelineState(it->second.graphicsPipelineState);
    currentPsoName_ = psoName;
}

void RenderQueue::SubmitSprite(const Sprite* sprite, const std::string& passName) {

    Draw2dRequest request;
    request.type = Draw2dType::Normal;
    request.layer = RenderLayer::Sprite;
    request.sprite = sprite;
    // 登録
    draw2dQueueList_[passName][request.layer][Get2dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitModel(const Model* model, WorldTransform& worldTransform, const float& alpha, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::Default;
    request.layer = RenderLayer::Opaque;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitInstancing(const Model* model, uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::Instancing;
    request.layer = RenderLayer::Opaque;
    request.passName = passName;
    request.model = model;
    request.numInstances = numInstances;
    request.worldTransforms = &worldTransforms;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitAnimation(const Model* model, WorldTransform& worldTransform, const float& alpha, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::Animation;
    request.layer = RenderLayer::Animation;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitSkybox(const Model* model, WorldTransform& worldTransform, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::Skybox;
    request.layer = RenderLayer::Skybox;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    // 不透明描画に登録
    draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitShadowMap(const Model* model, WorldTransform& worldTransform, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::ShadowMap;
    request.layer = RenderLayer::Shadow;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;

    // 不透明描画に登録
    draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitGrid(const Model* model, WorldTransform& worldTransform, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::Grid;
    request.layer = RenderLayer::Grid;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;

    // 不透明描画に登録
    draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitDebugLine(const DebugRenderer* debugRenderer, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::DebugLine;
    request.layer = RenderLayer::Debug;
    request.passName = passName;
    request.debugRenderer_ = debugRenderer;

    // 不透明描画に登録
    draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
}

void RenderQueue::Execute3dRequest(const Draw3dRequest& request) {
    switch (request.type) {

    case Draw3dType::Default:
    case Draw3dType::DefaultAdd:
        ModelRenderer::DrawLight(lightResource_);
        ModelRenderer::Draw(request.model, *request.worldTransform, request.material);
        break;

    case Draw3dType::Instancing:
    case Draw3dType::InstancingAdd:
        ModelRenderer::DrawInstancing(
            request.model, request.numInstances, *request.worldTransforms, request.material);
        break;

    case Draw3dType::Animation:
        ModelRenderer::DrawAnimation(request.model, *request.worldTransform, request.material);
        break;
        
    case Draw3dType::Skybox:
        ModelRenderer::DrawSkybox(request.model, *request.worldTransform, request.material);
        break;

    case Draw3dType::ShadowMap:
        ModelRenderer::DrawShadowMap(request.model, *request.worldTransform);
        break;

    case Draw3dType::Grid:
        ModelRenderer::DrawGrid(request.model, *request.worldTransform);
        break;

    case Draw3dType::DebugLine:
        ModelRenderer::DrawDebugLine(request.debugRenderer_->GetVertexBufferView(), request.debugRenderer_->GetTotalVertices());
        break;

    default:
        assert(false && "未対応のDraw3dTypeです");
        break;
    }
}

void RenderQueue::Execute2dRequest(const Draw2dRequest& request) {
   
    switch (request.type)
    {
    case Draw2dType::Normal:
        SpriteRenderer::Draw(request.sprite);
        break;

    case Draw2dType::Add:
        SpriteRenderer::Draw(request.sprite);
        break;

    default:
        assert(false && "未対応のDraw2dTypeです");
        break;
    }
}