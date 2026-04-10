#include"RtvManager.h"
#include"LogManager.h"
#include"DescriptorHeap.h"
#include"DescriptorHandle.h"
using namespace GameEngine;

void RtvManager::Initialize(ID3D12Device* device) {
    LogManager::GetInstance().Log("RtvManager start Initialize");

    device_ = device;

    // RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_ = {};
    rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // RTV用のヒープ
    rtvHeap_ = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, maxRtvCount_, false);

    // メモリを確保
    resources_.reserve(maxRtvCount_);

    // サイズを取得
    descriptorSizeRTV_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 色の設定する
    clearValue_.Color[0] = clearColor[0]; clearValue_.Color[1] = clearColor[1]; clearValue_.Color[2] = clearColor[2]; clearValue_.Color[3] = clearColor[3];

    LogManager::GetInstance().Log("RtvManager end Initialize\n");
}

uint32_t RtvManager::CreateRenderTargetResource(RtvContext context) {
    assert(currentRtvIndex_ < maxRtvCount_ && "RTV descriptor heap is full");

    // リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;

    // インデックスを取得
    uint32_t index = 0;
    // 空きを確認
    if (!freeIndices_.empty()) {
        index = freeIndices_.front();
        freeIndices_.pop_front();
    } else {
        assert(index < maxRtvCount_ && "RTV index out of range");
        // 空きがなければ新規インデックスを使用
        index = currentRtvIndex_++;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(rtvHeap_.Get(), descriptorSizeRTV_, index);

    D3D12_RESOURCE_DESC desc{};
    desc.Width = context.width;// テクスチャの幅
    desc.Height = context.height;// テクスチャの高さ
    desc.MipLevels = static_cast<UINT16>(context.mipLevels);// mipMapの数
    desc.DepthOrArraySize = 1;//  奥行 or 配列Textureの配列数
    desc.Format = context.format; // TextureのFormat
    desc.SampleDesc.Count = 1;// ダンプリングのカウント
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = context.flags;

    if (context.allowUAV) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    // クリアカラーのフォーマットを設定
    clearValue_.Format = desc.Format;

    CD3DX12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr;
    hr = device_->CreateCommittedResource(
        &heapProps, // Heapの設定
        D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。
        &desc, // Resourceの設定
        context.initialState, // データの転送される設定
        &clearValue_, // Clearの最適値
        IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
    assert(SUCCEEDED(hr));

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = context.format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // オブジェクト描画用RTVを作成
    device_->CreateRenderTargetView(resource.Get(), &rtvDesc, handle);

    // リソースを登録
    resources_.push_back(std::move(resource));

    return static_cast<uint32_t>(resources_.size() - 1);
}

void RtvManager::ReleseIndex(const uint32_t& index) {
    assert(index < maxRtvCount_ && "RTV index out of range");

    // リソースを解放
    if (resources_[index]) {
        resources_[index].Reset();

        // 解放されたインデックスを再利用リストに追加
        freeIndices_.push_back(index);
    }
}

ID3D12Resource* RtvManager::GetResource(const uint32_t& index) {
    assert(index < maxRtvCount_ && "RTV index out of range");
    return resources_[index].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE RtvManager::GetCPUHandle(const uint32_t& index) const {
    assert(index < maxRtvCount_ && "RTV index out of range");
    return GetCPUDescriptorHandle(rtvHeap_.Get(), descriptorSizeRTV_, index);
}