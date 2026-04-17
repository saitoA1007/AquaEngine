#include "PostEffectData.h"
using namespace GameEngine;

Vignetting::Vignetting() {
    // 作成
    buffer_.Create();
    buffer_.GetData()->intensity = 16.0f;
    buffer_.GetData()->time = 0.15f;
}

void Vignetting::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

RadialBlur::RadialBlur() {
    // 作成
    buffer_.Create();
    buffer_.GetData()->centerPos = { 0.5f,0.5f };
    buffer_.GetData()->numSamles = 2;
    buffer_.GetData()->blurWidth = 0.01f;
}

void RadialBlur::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

ScanLine::ScanLine() {
    // 作成
    buffer_.Create();
    buffer_.GetData()->interval = 96.0f;
    buffer_.GetData()->speed = -2.0f;
    buffer_.GetData()->time = 0.0f;
    buffer_.GetData()->lineColor = { 0.3f,0.3f,0.3f };
}

void ScanLine::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}