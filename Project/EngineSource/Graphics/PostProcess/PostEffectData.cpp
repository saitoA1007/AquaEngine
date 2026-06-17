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

Grayscale::Grayscale() {
    // 作成
    buffer_.Create();
}

void Grayscale::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

GaussianBlur::GaussianBlur() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->sd = 2.0f;
}

void GaussianBlur::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

OutLine::OutLine() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->diff = 6.0f;

    //isActive_ = true;
}

void OutLine::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}