#include "PostEffectData.h"
#include "FPSCounter.h"
using namespace GameEngine;

ColorGrading::ColorGrading() {
    // 作成
    buffer_.Create();
    auto* data = buffer_.GetData();
    
    data->enableGrayscale = 0;
    data->enableSepia = 0;
    data->enableRandom = 0;
    data->enableVignetting = 1;

    data->vignettingIntensity = 16.0f;
    data->vignettingTime = 0.15f;

    data->randomIntensity = 0.05f;
    data->randomTime = 1.0f;

    isActive_ = true;
}

void ColorGrading::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

RadialBlur::RadialBlur() {
    // 作成
    buffer_.Create();
    buffer_.GetData()->centerPos = { 0.5f,0.5f };
    buffer_.GetData()->numSamles = 3;
    buffer_.GetData()->blurWidth = 0.01f;
}

void RadialBlur::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

HighLumMask::HighLumMask() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->highLumMask = 0.8f;

    isActive_ = true;
}

void HighLumMask::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

GaussVertical::GaussVertical() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->sd = 2.0f;

    isActive_ = true;
}

void GaussVertical::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

GaussHorizontal::GaussHorizontal() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->sd = 2.0f;

    isActive_ = true;
}

void GaussHorizontal::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

Bloom::Bloom() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->intensity = 1.0f;

    isActive_ = true;
}

void Bloom::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

Dissolve::Dissolve() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->threshold = 0.0f;

    isActive_ = true;
}

void Dissolve::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}