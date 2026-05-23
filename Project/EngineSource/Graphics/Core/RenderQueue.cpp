#include "RenderQueue.h"
#include "DebugRenderer.h"
#include "Sprite.h"
#include "Material.h"
#include "Model.h"
#include "WorldTransform.h"
#include "WorldTransforms.h"
#include "DrawRequest.h"
#include "MyMath.h"
using namespace GameEngine;

RenderQueue::RenderQueue() {

}

void RenderQueue::Initialize() {
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

void RenderQueue::Update() {
    // カメラを設定する
    if (cameraPtr_ != nullptr) {
        mainCamera_.SetCamera(*cameraPtr_);
    }

    // ライトの更新
    lightManager_.Update();
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

void RenderQueue::SubmitRaytracingModel(const Model* model, WorldTransform& worldTransform, const uint32_t* materialIndex) {
    // 登録
    auto& meshes = model->GetMeshes();
    for (auto& mesh : meshes) {
        TLASInstanceData data;

        data.blas = mesh->GetBLAS();

        // 屈折するかや透明かなどの情報から使用するシェーダーを判断するようにする
        data.hitGroupIndexOffset = 0;

        if (materialIndex == nullptr) {
            // マテリアルを設定
            Material* drawMaterial = model->GetMaterial(mesh->GetMaterialName());
            auto& refBuffer = drawMaterial->GetMaterialBuffer();
            auto* refData = refBuffer.GetRefData();
            refData->indexHandle = mesh->GetIndexBufferSrvIndex() - refBuffer.GetBufferStartIndex();
            // アニメーションの有無で参照する頂点データを変更
            if (model->IsSkeleton()) {
                auto* skeleton = model->GetSkeleton();
                refData->vertexHandle = skeleton->GetOutputVertexBufferSrvIndex() - refBuffer.GetBufferStartIndex();
            } else {
                refData->vertexHandle = mesh->GetVertexBufferSrvIndex() - refBuffer.GetBufferStartIndex();
            }

            data.instanceID = drawMaterial->GetMaterialRefIndex();
        } else {
            data.instanceID = *materialIndex;
        }
        
        if (model->IsLoad()) {
            worldTransform.SetWVPMatrix(model->GetLocalMatrix());
        }
        Matrix4x4 matrix = Math::Transpose(worldTransform.GetWorldMatrix());
        std::memcpy(&data.transform, &matrix, sizeof(float) * 12);

        raytracingDrawQueueList_.push_back(std::move(data));
    }
}

const char* RenderQueue::Get3dPsoName(Draw3dType type) {
    switch (type) {
    case Draw3dType::Default: { return "Default3D"; }
    case Draw3dType::DefaultAdd: { return "Default3D"; }
    case Draw3dType::Instancing: { return "Instancing3D"; }
    case Draw3dType::InstancingAdd: { return "AdditiveInstancing3D"; }
    case Draw3dType::Animation: { return "Animation"; }
    case Draw3dType::Skybox: { return "Skybox"; }
    case Draw3dType::ShadowMap: { return "ShadowMap"; }
    case Draw3dType::Grid: { return "Grid"; }
    case Draw3dType::DebugLine: { return "Line"; }
    default: { return "Default3D"; }
    }
}

const char* RenderQueue::Get2dPsoName(Draw2dType type) {
    switch (type)
    {
    case GameEngine::Draw2dType::Normal: { return "DefaultSprite"; }
    case GameEngine::Draw2dType::Add: { return "AdditiveSprite"; }
    default: { return "DefaultSprite"; }
    }
}