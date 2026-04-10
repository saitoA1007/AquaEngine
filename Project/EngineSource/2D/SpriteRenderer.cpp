#include"SpriteRenderer.h"
#include<cassert>

using namespace GameEngine;

ID3D12GraphicsCommandList* SpriteRenderer::commandList_ = nullptr;
TextureManager* SpriteRenderer::textureManager_ = nullptr;
std::unordered_map<RenderMode2D, DrawPsoData> SpriteRenderer::psoList_;

void SpriteRenderer::StaticInitialize(ID3D12GraphicsCommandList* commandList, TextureManager* textureManager, PSOManager* psoManager) {
	commandList_ = commandList;
	textureManager_ = textureManager;

	// 通常描画のpsoデータを取得する
	psoList_[RenderMode2D::Normal] = psoManager->GetDrawPsoData("DefaultSprite");
	// 加算合成描画用のデータを取得する
	psoList_[RenderMode2D::Add] = psoManager->GetDrawPsoData("AdditiveSprite");
}

void SpriteRenderer::PreDraw(RenderMode2D mode) {
	auto pso = psoList_.find(mode);
	if (pso == psoList_.end()) {
		assert(0);
		return;
	}

	commandList_->SetGraphicsRootSignature(pso->second.rootSignature);
	commandList_->SetPipelineState(pso->second.graphicsPipelineState);
}

void SpriteRenderer::Draw(const Sprite* sprite, const uint32_t& textureHandle) {
	// Spriteの描画。
	commandList_->IASetVertexBuffers(0, 1, &sprite->GetVertexBufferView());
	commandList_->IASetIndexBuffer(&sprite->GetIndexBufferView());// IBVを設定
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// CBufferの場所を設定
	commandList_->SetGraphicsRootConstantBufferView(0, sprite->GetResource()->GetGPUVirtualAddress());
	if (textureHandle != 1024) {
		// 指定した画像を描画する
		commandList_->SetGraphicsRootDescriptorTable(1, textureManager_->GetTextureSrvHandlesGPU(textureHandle));
	} else {
		commandList_->SetGraphicsRootDescriptorTable(1, textureManager_->GetTextureSrvHandlesGPU(0));
	}
	// 描画
	commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
}