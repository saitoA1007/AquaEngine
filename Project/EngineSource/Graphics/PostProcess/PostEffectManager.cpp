#include "pch.h"
#include "PostEffectManager.h"
#include "LogManager.h"
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
    bloom_ = AddPostEffect<Bloom>("BloomPass", "Bloom");
    bloom_->SetGamePassIndex(renderPassController_->GetSrvIndex(renderPassController_->GetSceneFinalPass()));
    AddPostEffect<Dissolve>("DissolvePass", "Dissolve");

    // デフォルトのノードが繋がっている状態
    BuildDefaultGraph();
}

void PostEffectManager::Execute() {

    // ブルームを載せる最終パス
    bloom_->SetGamePassIndex(renderPassController_->GetSrvIndex(renderPassController_->GetSceneFinalPass()));

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

void PostEffectManager::BuildDefaultGraph() {
    graph_ = PostEffectGraph{};  

    auto* scene = graph_.AddSceneNode();
    auto* output = graph_.AddOutputNode();

    auto add = [&](const char* passName) {
        return graph_.AddEffectNode(passName, effects_.at(passName).get());
        };
    auto* highLum = add("HighLumMaskPass");
    auto* gaussV = add("GaussVerticalPass");
    auto* gaussH = add("GaussHorizontalPass");
    auto* bloom = add("BloomPass");
    auto* grading = add("ColorGradingPass");
    auto* dissolve = add("DissolvePass");

    // Scene → HighLum → GaussV → GaussH → Bloom(Blur) → Grading → Dissolve → Output
    graph_.AddLink(scene->output.id, highLum->inputs[0].id);
    graph_.AddLink(highLum->output.id, gaussV->inputs[0].id);
    graph_.AddLink(gaussV->output.id, gaussH->inputs[0].id);
    graph_.AddLink(gaussH->output.id, bloom->inputs[0].id);   // Blur
    graph_.AddLink(scene->output.id, bloom->inputs[1].id);   // 元の画像
    graph_.AddLink(bloom->output.id, grading->inputs[0].id);
    graph_.AddLink(grading->output.id, dissolve->inputs[0].id);
    graph_.AddLink(dissolve->output.id, output->inputs[0].id);
}

void PostEffectManager::RebuildOrder() {
    std::vector<int> order;
    if (graph_.BuildOrder(order)) {
        executeOrder_ = std::move(order);
    } else {
        // 循環など。前回の有効な順序を使い続ける
        LogManager::GetInstance().Log("PostEffectGraph: 実行順の構築に失敗しました\n");
    }
    graph_.dirty = false;
}