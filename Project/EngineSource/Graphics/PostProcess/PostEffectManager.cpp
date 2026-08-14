#include "PostEffectManager.h"
#include "LogManager.h"
#include "PostEffectData.h"
using namespace GameEngine;

void PostEffectManager::Initialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController) {
    commandList_ = commandList;
    srvManager_ = srvManager;
    renderPassController_ = renderPassController;

    // ヴィネットで描画するパス
    renderPassController_->AddPass("ColorGradingPass");
    // ぼかし
    renderPassController_->AddPass("GaussVerticalPass");
    renderPassController_->AddPass("GaussHorizontalPass");
    // 輝度マスク
    renderPassController_->AddPass("HighLumMaskPass");
    // ブルーム
    renderPassController_->AddPass("BloomPass");
    // ディゾルブ
    renderPassController_->AddPass("DissolvePass");

    // 実行順序を設定
    RegisterPassOrder({"HighLumMaskPass", "GaussVerticalPass", "GaussHorizontalPass", "BloomPass", "ColorGradingPass", "DissolvePass"});

    // psoを登録
    RegisterPSO("ColorGrading", psoManager);
    RegisterPSO("HighLumMask", psoManager);
    RegisterPSO("GaussVertical", psoManager);
    RegisterPSO("GaussHorizontal", psoManager);
    RegisterPSO("Bloom", psoManager);
    RegisterPSO("Dissolve", psoManager);

    // エフェクトを追加
    AddPostEffect<ColorGrading>("ColorGradingPass", "ColorGrading");
    AddPostEffect<HighLumMask>("HighLumMaskPass", "HighLumMask");
    AddPostEffect<GaussVertical>("GaussVerticalPass", "GaussVertical");
    AddPostEffect<GaussHorizontal>("GaussHorizontalPass", "GaussHorizontal");
    auto* bloom = AddPostEffect<Bloom>("BloomPass", "Bloom");
    bloom->SetGamePassIndex(renderPassController_->GetSrvIndex(renderPassController_->GetSceneFinalPass()));
    AddPostEffect<Dissolve>("DissolvePass", "Dissolve");
}

void PostEffectManager::Execute() {

    for (uint32_t i = 0; i < passExecuteOrder_.size(); ++i) {
        // 設定する
        if (i == 0) {
            currentPassName_ = renderPassController_->GetSceneFinalPass();
            currentPassIndex_ = renderPassController_->GetSrvIndex(currentPassName_);
        }
        const std::string passName = passExecuteOrder_[i];
        auto it = drawQueueList_.find(passName);
        if (it == drawQueueList_.end()) { assert(false && "Not found PostEffectPass"); }
        IPostEffect* postEffect = it->second;
        if (!postEffect->IsActive()) { continue; }

        // ポストエフェクトを掛ける画面を設定
        postEffect->SetPassIndex(currentPassIndex_);
        // 更新する
        postEffect->Update();

        renderPassController_->PrePass(passName);
        renderPassController_->ClearRenderPass(passName);

        // pso設定
        PreDraw(postEffect->GetPsoName());
        // 描画
        postEffect->Draw(commandList_, srvManager_);

        renderPassController_->PostPass(passName);

        // 現在の状態を登録
        currentPassName_ = passName;
        currentPassIndex_ = postEffect->GetPassIndex();
    }

    // 最終的な描画先を設定
    renderPassController_->SetPostProcessFinalPass(currentPassName_);
    renderPassController_->SetPresentPass(currentPassName_);
}

void PostEffectManager::PreDraw(const std::string& psoName) {
    auto it = psoList_.find(psoName);
    assert(it != psoList_.end() && "未登録のPSO名です");

    commandList_->SetGraphicsRootSignature(it->second.rootSignature);
    commandList_->SetPipelineState(it->second.graphicsPipelineState);
}
