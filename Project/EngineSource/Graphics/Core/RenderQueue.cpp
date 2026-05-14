#include "RenderQueue.h"
#include "PSO/Core/PSOManager.h"
#include "ModelRenderer.h"
#include "DebugRenderer.h"
#include "SpriteRenderer.h"
#include "RaytracingPipeline.h"
#include "MyMath.h"
using namespace GameEngine;

RenderQueue::RenderQueue() {

}

void RenderQueue::Initialize(ID3D12GraphicsCommandList4* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController,
    RaytracingPipeline* raytracingPipeline, BufferRefManager* bufferRefManager) {
    commandList_ = commandList;
    renderPassController_ = renderPassController;
    raytracingPipeline_ = raytracingPipeline;
    srvManager_ = srvManager;
    bufferRefManager_ = bufferRefManager;

    // 影を描画するパス
    renderPassController_->AddPass("ShadowPass", RenderTextureMode::DsvOnly, 2048, 2048);
    // デフォルトで描画するパス
    renderPassController_->AddPass("DefaultPass");
    // レイトレーシングで描画するパス
    renderPassController_->AddPass("RaytracingPass", RenderTextureMode::RtvAndUav);
    // レイトレとラスタライズを合成する用のパス
    renderPassController_->AddPass("LightingCompositePass");

    // レイトレーシング描画の深度情報を記録する。描画に使用しない
    renderPassController_->AddPass("RaytracingPassDepth", RenderTextureMode::UavOnly,1280,720, DXGI_FORMAT_R32_FLOAT);

    // 最終的な描画先を設定
    finalPassName_ = "LightingCompositePass";
    renderPassController_->SetSceneFinalPass(finalPassName_);
    renderPassController_->SetPresentPass(finalPassName_);

    // 実行順序を設定
    RegisterPassOrder({ "ShadowPass", "DefaultPass","RaytracingPass" });

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

    // レイトレとラスタライズの合成用
    RegisterPSO("LightingComposite", psoManager);

    // bufferのsrvIndexのスタート位置を設定
    bufferStartSrvIndex_ = srvManager_->GetStartSrvIndex(SrvHeapType::Buffer);

    // tlasを作成
    tlas_.Create(commandList_, maxRayInstanceNum_);

    // カメラを設定
    mainCamera_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);

    // 平行光源ライト
    directionalData_.active = true;
    directionalData_.color = { 1.0f,1.0f,1.0f,1.0f };
    directionalData_.direction = { 0.0,-1.0f,0.0f };
    directionalData_.intensity = 1.0f;

    // ライトの設定
    lightManager_.Initialize(true, false, false);
    lightManager_.SetDirectionalData(directionalData_);
}

void RenderQueue::Begin() {
    // クリア
    Clear();
}

void RenderQueue::Execute() {

    // カメラを設定する
    if (cameraPtr_ != nullptr) {
        mainCamera_.SetCamera(*cameraPtr_);
    }

    for (const auto& passName : passExecuteOrder_) {

        // 不透明、半透明ともにコマンドがなければ飛ばす
        bool hasOpaque = draw3dQueueList_.count(passName) > 0;
        bool hasTranslucent = translucentDrawQueueList_.count(passName) > 0;
        bool has2d = draw2dQueueList_.count(passName) > 0;
        bool hasRaytracing = raytracingDrawQueueList_.count(passName) > 0;
        if (!hasOpaque && !hasTranslucent && !has2d && !hasRaytracing) {
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

        // レイトレーシング描画コマンドを解放
        if (hasRaytracing) {
        
            // tlasを更新する
            tlas_.Update(commandList_, raytracingDrawQueueList_[passName]);
         
            // UAVに状態を変更
            renderPassController_->SwitchToUAV(passName);
            renderPassController_->SwitchToUAV("RaytracingPassDepth");
         
            // レイトレーシングの描画
            DrawRaytracing();
         
            // UAV書き込み完了
            renderPassController_->InsertUavBarrier(passName);
            renderPassController_->InsertUavBarrier("RaytracingPassDepth");
            renderPassController_->PostPass("RaytracingPassDepth");
        }

        renderPassController_->PostPass(passName);
    }

    // レイトレとラスタライズの内容を合成する
    renderPassController_->PrePass("LightingCompositePass");
    PreDraw("LightingComposite");
    commandList_->SetGraphicsRootDescriptorTable(0, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex("DefaultPass")));
    commandList_->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUHandle(renderPassController_->GetDepthSrvIndex("DefaultPass")));
    commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex("RaytracingPass")));
    commandList_->SetGraphicsRootDescriptorTable(3, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex("RaytracingPassDepth")));
    commandList_->DrawInstanced(3, 1, 0, 0);
    renderPassController_->PostPass("LightingCompositePass");

    // 最終的に画面に出すためのパスの設定
    renderPassController_->SetPresentPass(finalPassName_);
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
        ModelRenderer::SetCamera(lightManager_.GetResource());
    } else {
        if (useDebugCamera_) {
            // デバック用のカメラを設定
            ModelRenderer::SetCamera(debugCameraResource_->GetResource());
        } else {
            ModelRenderer::SetCamera(mainCamera_.GetResource());   
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

void RenderQueue::SubmitRaytracingModel(const Model* model, WorldTransform& worldTransform, const uint32_t* materialIndex, const std::string& passName) {
    // 登録
    auto& meshes = model->GetMeshes();
    for (auto& mesh : meshes) {
        TLASInstanceData data;

        data.blas = mesh->GetBLAS();
        data.hitGroupIndexOffset = mesh->GetHitGroupIndex();

        if (materialIndex == nullptr) {
            // マテリアルを設定
            const Material* drawMaterial = model->GetMaterial(mesh->GetMaterialName());

            data.instanceID = drawMaterial->GetMaterialRefIndex();
        } else {
            data.instanceID = *materialIndex;
        }

        if (model->IsLoad()) {
            worldTransform.SetWVPMatrix(model->GetLocalMatrix());
        }
        Matrix4x4 matrix = Transpose(worldTransform.GetWorldMatrix());
        std::memcpy(&data.transform, &matrix, sizeof(float) * 12);

        raytracingDrawQueueList_[passName].push_back(std::move(data));
    }
}

void RenderQueue::Execute3dRequest(const Draw3dRequest& request) {
    switch (request.type) {

    case Draw3dType::Default:
    case Draw3dType::DefaultAdd:
        ModelRenderer::DrawLight(lightManager_.GetConstantBuffer());
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

void RenderQueue::DrawRaytracing() {
    commandList_->SetComputeRootSignature(raytracingPipeline_->GetGlobalRootSignature());
    // TLASのセット
    commandList_->SetComputeRootDescriptorTable(0, tlas_.GetSrvHandleGPU());
    // テスクチャのセット
    commandList_->SetComputeRootDescriptorTable(1, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    // BufferRefのセット
    commandList_->SetComputeRootDescriptorTable(2, bufferRefManager_->GetSrvHandleGPU());
    // Bufferのセット
    commandList_->SetComputeRootDescriptorTable(3, srvManager_->GetGPUHandle(bufferStartSrvIndex_));
    // カメラのセット
    if (useDebugCamera_) {
        commandList_->SetComputeRootConstantBufferView(4, debugCameraResource_->GetGpuVirtualAddress());
    } else {
        commandList_->SetComputeRootConstantBufferView(4, mainCamera_.GetConstantBuffer()->GetGpuVirtualAddress());
    }
    // ライトのセット
    commandList_->SetComputeRootConstantBufferView(5, lightManager_.GetConstantBuffer()->GetGpuVirtualAddress());
    // 出力画像を設定
    commandList_->SetComputeRootDescriptorTable(6, srvManager_->GetGPUHandle(renderPassController_->GetUavIndex("RaytracingPass")));
    commandList_->SetComputeRootDescriptorTable(7, srvManager_->GetGPUHandle(renderPassController_->GetUavIndex("RaytracingPassDepth")));
    // 背景画像
    commandList_->SetComputeRootDescriptorTable(8, srvManager_->GetGPUHandle(skyboxTextureIndex_));

    // レイトレーシングを開始
    commandList_->SetPipelineState1(raytracingPipeline_->GetStateObject());
    commandList_->DispatchRays(&raytracingPipeline_->GetDispatchRayDesc());
}