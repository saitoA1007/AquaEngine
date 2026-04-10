#include"BlendBuilder.h"

using namespace GameEngine;

void BlendBuilder::Initialize() {

	for (uint32_t i = 0; i < BlendMode::kCountOfBlendMode; ++i) {
		// すべての色要素を書き込む
		blendDesc_[i].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// ブレンドモードの有効化
		if (i != kBlendModeNone) {
			blendDesc_[i].RenderTarget[0].BlendEnable = TRUE; // ブレンドを有効化
			blendDesc_[i].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO; // アルファ値のソース
			blendDesc_[i].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファ値の加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE; // アルファ値のデスティネーション
		}

		switch (i) {

		case kBlendModeNormal:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // (1-SrcA)
			break;

		case kBlendModeAdd:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;

		case kBlendModeSubtract:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;

		case kBlendModeMultily:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // (1-SrcA)
			break;

		case kBlendModeScreen:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;
		}
	}
}

D3D12_BLEND_DESC BlendBuilder::GetBlendDesc(BlendMode blendMode) {
	return blendDesc_[blendMode];
}