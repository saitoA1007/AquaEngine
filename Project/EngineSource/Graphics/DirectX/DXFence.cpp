#include "pch.h"
#include"DXFence.h"
#include<cassert>
#include"LogManager.h"
using namespace GameEngine;

DXFence::~DXFence() {
    if (fenceEvent_ != nullptr) {
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }
}

void DXFence::Initialize(ID3D12Device* device) {

    LogManager::GetInstance().Log("Start　Create Fence");

    // 初期値0でFenceを作る
    HRESULT hr = device->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    assert(SUCCEEDED(hr));

    // FenceのSignalを待つためのイベントを作成する
    fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent_ != nullptr);

    LogManager::GetInstance().Log("End　Create Fence");
}

uint64_t DXFence::Signal(ID3D12CommandQueue* commandQueue) {
    // Fenceの値を更新
    fenceValue_++;
    // GPUがここまでたどり着いた時に、Fenceの値を指定した値を代入するようにSignalを送る
    HRESULT hr = commandQueue->Signal(fence_.Get(), fenceValue_);
    assert(SUCCEEDED(hr));
    return fenceValue_;
}

void DXFence::WaitForValue(uint64_t value) {
    // Fenceの値が指定した値にたどり着いているかを確認する
    if (fence_->GetCompletedValue() < value) {
        // 指定したSignalにたどりついていないので、たどり着くまで待つようにイベントを設定する
        fence_->SetEventOnCompletion(value, fenceEvent_);
        // イベントを待つ
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
}

void DXFence::WaitForGPU(ID3D12CommandQueue* commandQueue) {
    // これまでに積んだすべてのコマンドの完了を待つ
    WaitForValue(Signal(commandQueue));
}
