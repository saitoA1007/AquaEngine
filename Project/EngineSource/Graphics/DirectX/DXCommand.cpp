#include "pch.h"
#include"DXCommand.h"
#include<cassert>
#include<string>
#include"LogManager.h"
#include"Debug/DebugName.h"

using namespace GameEngine;

void DXCommand::Initialize(ID3D12Device* device) {

    LogManager::GetInstance().Log("Start　Create CommandList");

    // コマンドキューを生成する
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    HRESULT hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
    // コマンドキューの生成がうまくいかなかったので起動できない
    assert(SUCCEEDED(hr));

    // コマンドアロケータをフレームスロットの数だけ生成する
    for (uint32_t i = 0; i < kFrameCount; ++i) {
        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators_[i]));
        // コマンドアロケータの生成がうまくいかなかったので起動できない
        assert(SUCCEEDED(hr));
        SetDebugName(commandAllocators_[i].Get(), ("MainCommandAllocator[" + std::to_string(i) + "]").c_str());
    }

    // コマンドリストを生成する。最初のフレームはスロット0のアロケータで記録する
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators_[0].Get(), nullptr, IID_PPV_ARGS(&commandList_));
    // コマンドリストを生成がうまくいかなかったので起動できない
    assert(SUCCEEDED(hr));

    // PIX上で識別できるように名前を付ける
    SetDebugName(commandQueue_.Get(), "MainCommandQueue");
    SetDebugName(commandList_.Get(), "MainCommandList");

    LogManager::GetInstance().Log("End　Create CommandList");
}

void DXCommand::Close() {
    // コマンドリストの内容を確定させる。すべてのコマンドを積んでからcloseにすること
    HRESULT hr = commandList_->Close();
    assert(SUCCEEDED(hr));
}

void DXCommand::Execute(ID3D12CommandList* preCommandList) {
    // 先に実行するコマンドがあれば別のExecuteで実行する
    if (preCommandList != nullptr) {
        commandQueue_->ExecuteCommandLists(1, &preCommandList);
    }

    // GPUにコマンドリストの実行を行わせる
    ID3D12CommandList* commandLists[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(1, commandLists);
}

void DXCommand::Reset(uint32_t frameIndex) {
    // 次のフレーム用にコマンドリストを準備
    ID3D12CommandAllocator* allocator = commandAllocators_[frameIndex].Get();
    HRESULT hr = allocator->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList_->Reset(allocator, nullptr);
    assert(SUCCEEDED(hr));
}
