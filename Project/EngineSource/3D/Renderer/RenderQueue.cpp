#include "RenderQueue.h"
#include "PSO/Core/PSOManager.h"
#include "ModelRenderer.h"

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

    // 文字列キーでそのまま登録。PSOManager側の名前と一致させる
    RegisterPSO("Default3D", psoManager);
    RegisterPSO("Additive3D", psoManager);
    RegisterPSO("Instancing3D", psoManager);
    RegisterPSO("AdditiveInstancing3D", psoManager);
    RegisterPSO("Grid", psoManager);
    RegisterPSO("Animation", psoManager);
    RegisterPSO("Skybox", psoManager);
    RegisterPSO("ShadowMap", psoManager);
}

void RenderQueue::Begin() {
    // 描画コマンドのクリア
    Clear();
}

void RenderQueue::Execute() {

    // 通常描画
    renderPassController_->PrePass("DefaultPass");

    // 通常描画コマンドを解放
    for (auto& [layer, psoMap] : drawQueueList_) {
        for (auto& [psoName, requests] : psoMap) {
            if (requests.empty()) { continue; }
            // 描画前処理
            PreDraw(psoName.c_str());
            // 描画コマンド解放
            for (const auto& request : requests) {
                ExecuteRequest(request);
            }
        }
    }

    // 半透明描画コマンドを解放
    if (!translucentDrawQueueList_.empty()) {
       // カメラの距離でソートをおこなう
       // std::sort(translucentDrawQueueList_.begin(), translucentDrawQueueList_.end(),
       //     [](const DrawRequest& a, const DrawRequest& b) {
       //         return a.sortKey > b.sortKey;
       //     });

        for (const auto& request : translucentDrawQueueList_) {
            // 描画前処理
            PreDraw(GetPsoName(request.type));
            // 描画コマンド解放
            ExecuteRequest(request);
        }
    }

    renderPassController_->PostPass("DefaultPass");
}

const char* RenderQueue::GetPsoName(DrawType type) {
    switch (type) {
        case DrawType::Default:         { return "Default3D"; }
        case DrawType::DefaultAdd:      { return "Default3D"; }
        case DrawType::Instancing:      { return "Instancing3D"; }
        case DrawType::InstancingAdd:   { return "AdditiveInstancing3D"; }
        case DrawType::Animation:       { return "Animation"; }
        case DrawType::Skybox:          { return "Skybox"; }
        case DrawType::ShadowMap:       { return "ShadowMap"; }
        case DrawType::Grid:            { return "Grid"; }
        default:                        { return "Default3D"; }
    }
}

void RenderQueue::RegisterPSO(const std::string& name, PSOManager* psoManager) {
    psoList_[name] = psoManager->GetDrawPsoData(name);
}

// 戻り値で登録済みかどうかを返す
void RenderQueue::PreDraw(const std::string& psoName) {
    // 前回と同じPSOなら切り替え不要
    if (currentPsoName_ == psoName) { return; }

    auto it = psoList_.find(psoName);
    assert(it != psoList_.end() && "未登録のPSO名です");

    commandList_->SetGraphicsRootSignature(it->second.rootSignature);
    commandList_->SetPipelineState(it->second.graphicsPipelineState);
    currentPsoName_ = psoName;

    if (psoName == "ShadowMap") {
        ModelRenderer::SetCamera(lightCameraResource_);
    } else {
        ModelRenderer::SetCamera(cameraResource_->GetResource());
    } 
}

void RenderQueue::SubmitModel(const Model* model, WorldTransform& worldTransform, const float& alpha, const GpuResource* material) {
    DrawRequest request;
    request.type = DrawType::Default;
    request.layer = RenderLayer::Opaque;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        drawQueueList_[request.layer][GetPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_.push_back(request);
    }
}

void RenderQueue::SubmitInstancing(const Model* model, uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha, const GpuResource* material) {
    DrawRequest request;
    request.type = DrawType::Instancing;
    request.layer = RenderLayer::Opaque;
    request.model = model;
    request.numInstances = numInstances;
    request.worldTransforms = &worldTransforms;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        drawQueueList_[request.layer][GetPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_.push_back(request);
    }
}

void RenderQueue::SubmitAnimation(const Model* model, WorldTransform& worldTransform, const float& alpha, const GpuResource* material) {
    DrawRequest request;
    request.type = DrawType::Animation;
    request.layer = RenderLayer::Animation;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        drawQueueList_[request.layer][GetPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_.push_back(request);
    }
}

void RenderQueue::SubmitSkybox(const Model* model, WorldTransform& worldTransform, const GpuResource* material) {
    DrawRequest request;
    request.type = DrawType::Skybox;
    request.layer = RenderLayer::Skybox;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    // 不透明描画に登録
    drawQueueList_[request.layer][GetPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitShadowMap(const Model* model, WorldTransform& worldTransform) {
    DrawRequest request;
    request.type = DrawType::ShadowMap;
    request.layer = RenderLayer::Shadow;
    request.model = model;
    request.worldTransform = &worldTransform;

    // 不透明描画に登録
    drawQueueList_[request.layer][GetPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitGrid(const Model* model, WorldTransform& worldTransform) {
    DrawRequest request;
    request.type = DrawType::Grid;
    request.layer = RenderLayer::Grid;
    request.model = model;
    request.worldTransform = &worldTransform;

    // 不透明描画に登録
    drawQueueList_[request.layer][GetPsoName(request.type)].push_back(request);
}

void RenderQueue::ExecuteRequest(const DrawRequest& request) {
    switch (request.type) {

    case DrawType::Default:
    case DrawType::DefaultAdd:
        ModelRenderer::DrawLight(lightResource_);
        ModelRenderer::Draw(request.model, *request.worldTransform, request.material);
        break;

    case DrawType::Instancing:
    case DrawType::InstancingAdd:
        ModelRenderer::DrawInstancing(
            request.model, request.numInstances, *request.worldTransforms, request.material);
        break;

    case DrawType::Animation:
        ModelRenderer::DrawAnimation(request.model, *request.worldTransform, request.material);
        break;

    case DrawType::Skybox:
        ModelRenderer::DrawSkybox(request.model, *request.worldTransform, request.material);
        break;

    case DrawType::ShadowMap:
        ModelRenderer::DrawShadowMap(request.model, *request.worldTransform);
        break;

    case DrawType::Grid:
        ModelRenderer::DrawGrid(request.model, *request.worldTransform);
        break;

    default:
        assert(false && "未対応のDrawTypeです");
        break;
    }
}