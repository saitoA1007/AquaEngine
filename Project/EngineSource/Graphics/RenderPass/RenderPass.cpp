#include"RenderPass.h"

using namespace GameEngine;

RenderPass::RenderPass(const std::string& name, ID3D12GraphicsCommandList* commandList, RenderTexture* renderTexture) {
	name_ = name;
	renderTexture_ = renderTexture;

	commandList_ = commandList;

	// 画面サイズを取得
	uint32_t width = renderTexture_->GetWidth();
	uint32_t height = renderTexture_->GetHeight();

	// Viewportを作成
	viewport_.Width = static_cast<FLOAT>(width);
	viewport_.Height = static_cast<FLOAT>(height);
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
	// Scissorを作成
	scissorRect_.left = 0;
	scissorRect_.right = static_cast<LONG>(viewport_.Width);
	scissorRect_.top = 0;
	scissorRect_.bottom = static_cast<LONG>(viewport_.Height);

	// モードを取得する
	mode_ = renderTexture_->GetMode();
}

void RenderPass::PrePass() {
	// 書き込み状態に遷移
	renderTexture_->TransitionToRenderTarget(commandList_);

	switch (mode_)
	{
	case GameEngine::RenderTextureMode::RtvOnly: {
		// RTVのみセットする
		commandList_->OMSetRenderTargets(1, &renderTexture_->GetRtvHandle(), false, nullptr);

		// 指定した色で画面全体をクリアする
		float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
		commandList_->ClearRenderTargetView(renderTexture_->GetRtvHandle(), clearColor, 0, nullptr);
		break;
	}

	case GameEngine::RenderTextureMode::DsvOnly: {
		// DSVのみセットする
		commandList_->OMSetRenderTargets(0, nullptr, false, &renderTexture_->GetDsvHandle());

		// 深度クリア
		commandList_->ClearDepthStencilView(renderTexture_->GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		break;
	}

	case GameEngine::RenderTextureMode::RtvAndDsv: {
		// RTVとDSVをセットする
		commandList_->OMSetRenderTargets(1, &renderTexture_->GetRtvHandle(), false, &renderTexture_->GetDsvHandle());

		float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
		// 指定した色で画面全体をクリアする
		commandList_->ClearRenderTargetView(renderTexture_->GetRtvHandle(), clearColor, 0, nullptr);
		// 指定した深度で画面全体をクリアする
		commandList_->ClearDepthStencilView(renderTexture_->GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		break;
	}

	case RenderTextureMode::UavOnly: {
		// UAV状態へ遷移
		renderTexture_->TransitionToUnorderedAccess(commandList_);
		break;
	}

	case RenderTextureMode::RtvAndUav: {
		// まずRTV状態で描画パスを開始する
		renderTexture_->TransitionToRenderTarget(commandList_);

		// RTVのみセットする
		commandList_->OMSetRenderTargets(1, &renderTexture_->GetRtvHandle(), false, nullptr);

		// 指定した色で画面全体をクリアする
		float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
		commandList_->ClearRenderTargetView(renderTexture_->GetRtvHandle(), clearColor, 0, nullptr);
		break;
	}
	}

	if (mode_ != RenderTextureMode::UavOnly) {
		// Viewportを設定
		commandList_->RSSetViewports(1, &viewport_);
		// Scirssorを設定
		commandList_->RSSetScissorRects(1, &scissorRect_);
	}	
}

void RenderPass::PostPass() {
	// 読み込み状態に遷移
	renderTexture_->TransitionToShaderResource(commandList_);
}

void RenderPass::SwitchToUnorderedAccess() {
	commandList_->OMSetRenderTargets(0, nullptr, false, nullptr);
	renderTexture_->TransitionToUnorderedAccess(commandList_);
}

void RenderPass::InsertUavBarrier() {
	renderTexture_->InsertUavBarrier(commandList_);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE RenderPass::GetSrvHandle() {
	return renderTexture_->GetSrvGpuHandle();
}