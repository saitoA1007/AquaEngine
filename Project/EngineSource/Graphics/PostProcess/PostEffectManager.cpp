#include"PostEffectManager.h"
#include"DescriptorHeap.h"
#include"DescriptorHandle.h"
#include"LogManager.h"
using namespace GameEngine;

BloomPSO* PostEffectManager::bloomPSO_ = nullptr;
OutLinePSO* PostEffectManager::outLinePSO_ = nullptr;
std::array<DrawPsoData, static_cast<size_t>(PostEffectManager::PSOType::MaxCount)> PostEffectManager::psoList_;

void PostEffectManager::StaticInitialize(BloomPSO* bloomPSO, OutLinePSO* outLinePSO, PSOManager* psoManager) {
    bloomPSO_ = bloomPSO;
    outLinePSO_ = outLinePSO;

    psoList_[static_cast<size_t>(PSOType::Vignetting)] = psoManager->GetDrawPsoData("Vignetting");
    psoList_[static_cast<size_t>(PSOType::ScanLine)] = psoManager->GetDrawPsoData("ScanLine");
    psoList_[static_cast<size_t>(PSOType::RadialBlur)] = psoManager->GetDrawPsoData("RadialBlur");
}

void PostEffectManager::Initialize(ID3D12Device* device, float clearColor_[4], uint32_t width, uint32_t height, uint32_t descriptorSizeRTV, SrvManager* srvManager) {

    // デバイスを取得
    device_ = device;

    srvManager_ = srvManager;

    // ポストエフェクトのRTV用ヒープ作成
    postProcessRTVHeap_ = CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kRTVNum, false);

    // テクスチャリソース作成
    D3D12_RESOURCE_DESC desc{};
    desc.Width = width;   // テクスチャの幅
    desc.Height = height; // テクスチャの高さ
    desc.MipLevels = 1;  // mipMapの数
    desc.DepthOrArraySize = 1; //  奥行 or 配列Textureの配列数
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // TextureのFormat
    desc.SampleDesc.Count = 1; // ダンプリングのカウント
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // Textureの次元数
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    // Clearの最適値
    clearValue_.Format = desc.Format;
    clearValue_.Color[0] = clearColor_[0]; clearValue_.Color[1] = clearColor_[1]; clearValue_.Color[2] = clearColor_[2]; clearValue_.Color[3] = clearColor_[3];

    // 利用するHeapの設定。
    CD3DX12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr;

    // オブジェクトを描画する用のリソースを作成
    hr = device_->CreateCommittedResource(
        &heapProps, // Heapの設定
        D3D12_HEAP_FLAG_NONE,  // Heapの特殊な設定。特になし
        &desc,  // Resourceの設定
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // データの転送される設定
        &clearValue_, // Clearの最適値
        IID_PPV_ARGS(&DrawObjectResource_));  // 作成するResourceポインタへのポインタ
    assert(SUCCEEDED(hr));

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // オブジェクト描画用RTVを作成
    drawObjectRTVHandle_ = GetCPUDescriptorHandle(postProcessRTVHeap_.Get(), descriptorSizeRTV, rtvIndex_);
    device_->CreateRenderTargetView(DrawObjectResource_.Get(), &rtvDesc, drawObjectRTVHandle_);

    // SRV作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    // SRVハンドル取得
    uint32_t index = srvManager_->AllocateSrvIndex(SrvHeapType::System);
    D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle = srvManager_->GetCPUHandle(index);
    drawObjectSRVHandle_ = srvManager_->GetGPUHandle(index);
    drawObjectIndex_ = index;

    // オブジェクト描画用SRV
    device_->CreateShaderResourceView(DrawObjectResource_.Get(), &srvDesc, srvCPUHandle);

    // ブルームの初期化
    InitializeBloom(width, height, descriptorSizeRTV);

    // ポストエフェクトのデータを初期化
    InitializePostEffectData(width, height, descriptorSizeRTV);
}

void PostEffectManager::PreDraw(ID3D12GraphicsCommandList* commandList, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect, float clearColor[4], ID3D12DescriptorHeap* dsvHeap) {

    // バリア: オフスクリーンをレンダーターゲットに
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = DrawObjectResource_.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // ヒープを設定する
#ifdef USE_IMGUI
    // ImGuiの方で普段は設定されている
#else
    ID3D12DescriptorHeap* descriptorHeaps[] = { srvManager_->GetSRVHeap() };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);
#endif

    // 描画先をオフスクリーンRTVに
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    commandList->OMSetRenderTargets(1, &drawObjectRTVHandle_, false, &dsvHandle);

    // 指定した色で画面全体をクリアする
    commandList->ClearRenderTargetView(drawObjectRTVHandle_, clearColor, 0, nullptr);
    // 指定した深度で画面全体をクリアする
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    commandList->RSSetViewports(1, &viewport); // Viewportを設定
    commandList->RSSetScissorRects(1, &scissorRect); // Scirssorを設定
}

void PostEffectManager::PostDraw(ID3D12GraphicsCommandList* commandList, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect, D3D12_GPU_DESCRIPTOR_HANDLE depthSRV) {
    D3D12_RESOURCE_BARRIER PostProcessBarrier{};
    PostProcessBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    PostProcessBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    PostProcessBarrier.Transition.pResource = DrawObjectResource_.Get();
    PostProcessBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    PostProcessBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &PostProcessBarrier);

    /// ここにポストエフェクトの描画処理を記入===================

    D3D12_GPU_DESCRIPTOR_HANDLE currentInputSRV = drawObjectSRVHandle_;

    // アウトラインを描画
   // DrawOutLine(commandList, depthSRV, currentInputSRV);
    //currentInputSRV = outLineData_.srvHandle;
    depthSRV;

    // ブルームを描画
    DrawBloom(commandList, currentInputSRV, viewport, scissorRect);
    currentInputSRV = bloomSRVHandle_[3];

    // ヴィネットの描画
    DrawEffect(commandList, vignettingData_, vignettingResource_.GetResource());
    currentInputSRV = vignettingData_.srvHandle;

    switch (drawMode_)
    {
    case GameEngine::PostEffectManager::DrawMode::Default:
        break;

    case GameEngine::PostEffectManager::DrawMode::ScanLine:
        // scanLineを描画
        DrawEffect(commandList, scanLineData_, scanLineResource_.GetResource());
        currentInputSRV = scanLineData_.srvHandle;
        break;

    case GameEngine::PostEffectManager::DrawMode::RadialBlur:
        // ラジアルブルーを描画
        DrawEffect(commandList, radialBlurData_, radialBlurResource_.GetResource());
        currentInputSRV = radialBlurData_.srvHandle;
        break;
    }

    resultSRVHandle_ = currentInputSRV;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE& PostEffectManager::GetSRVHandle() {
    return resultSRVHandle_;
}

void PostEffectManager::InitializeBloom(uint32_t width, uint32_t height, uint32_t descriptorSizeRTV) {
    // ブルームの生成
    LogManager::GetInstance().Log("Start Create BloomRenderTargets");

    // テクスチャリソース作成
    D3D12_RESOURCE_DESC desc{};
    desc.Width = width;   // テクスチャの幅
    desc.Height = height; // テクスチャの高さ
    desc.MipLevels = 1;  // mipMapの数
    desc.DepthOrArraySize = 1; //  奥行 or 配列Textureの配列数
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // TextureのFormat
    desc.SampleDesc.Count = 1; // ダンプリングのカウント
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // Textureの次元数
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    // 利用するHeapの設定。
    CD3DX12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    // 5つのブルーム用レンダーターゲットを作成
    HRESULT hr;

    // 明るい部分抽出用のリソースを作成 D3D12_RESOURCE_STATE_RENDER_TARGET
    hr = device_->CreateCommittedResource(
        &heapProps, // Heapの設定
        D3D12_HEAP_FLAG_NONE,  // Heapの特殊な設定。特になし
        &desc,  // Resourceの設定
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // データの転送される設定
        &clearValue_, // Clearの最適値
        IID_PPV_ARGS(&bloomBrightResource_));  // 作成するResourceポインタへのポインタ
    assert(SUCCEEDED(hr));

    // 縮小しながらブラーする用のリソースを作成
    hr = device_->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue_, IID_PPV_ARGS(&bloomBlurShrinkResource_));
    assert(SUCCEEDED(hr));

    // ブラーした結果を描画のリソースを作成
    hr = device_->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue_, IID_PPV_ARGS(&bloomResultResource_));
    assert(SUCCEEDED(hr));

    // 合成したものを描画のリソースを作成
    hr = device_->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue_, IID_PPV_ARGS(&bloomCompositeResource_));
    assert(SUCCEEDED(hr));

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // 2つ目
    bloomRTVHandle_[0] = GetCPUDescriptorHandle(postProcessRTVHeap_.Get(), descriptorSizeRTV, ++rtvIndex_);
    device_->CreateRenderTargetView(bloomBrightResource_.Get(), &rtvDesc, bloomRTVHandle_[0]);
    // 3つ目
    bloomRTVHandle_[1] = GetCPUDescriptorHandle(postProcessRTVHeap_.Get(), descriptorSizeRTV, ++rtvIndex_);
    device_->CreateRenderTargetView(bloomBlurShrinkResource_.Get(), &rtvDesc, bloomRTVHandle_[1]);
    // 4つ目
    bloomRTVHandle_[2] = GetCPUDescriptorHandle(postProcessRTVHeap_.Get(), descriptorSizeRTV, ++rtvIndex_);
    device_->CreateRenderTargetView(bloomResultResource_.Get(), &rtvDesc, bloomRTVHandle_[2]);
    // 5つ目
    bloomRTVHandle_[3] = GetCPUDescriptorHandle(postProcessRTVHeap_.Get(), descriptorSizeRTV, ++rtvIndex_);
    device_->CreateRenderTargetView(bloomCompositeResource_.Get(), &rtvDesc, bloomRTVHandle_[3]);

    // SRV作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    uint32_t index[4] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle[4];

    index[0] = srvManager_->AllocateSrvIndex(SrvHeapType::System);
    srvCPUHandle[0] = srvManager_->GetCPUHandle(index[0]);
    bloomSRVHandle_[0] = srvManager_->GetGPUHandle(index[0]);
    device_->CreateShaderResourceView(bloomBrightResource_.Get(), &srvDesc, srvCPUHandle[0]);

    // ブラー2回目用SRV
    index[1] = srvManager_->AllocateSrvIndex(SrvHeapType::System);
    srvCPUHandle[1] = srvManager_->GetCPUHandle(index[1]);
    bloomSRVHandle_[1] = srvManager_->GetGPUHandle(index[1]);
    device_->CreateShaderResourceView(bloomBlurShrinkResource_.Get(), &srvDesc, srvCPUHandle[1]);

    // ブラー3回目用SRV
    index[2] = srvManager_->AllocateSrvIndex(SrvHeapType::System);
    srvCPUHandle[2] = srvManager_->GetCPUHandle(index[2]);
    bloomSRVHandle_[2] = srvManager_->GetGPUHandle(index[2]);
    device_->CreateShaderResourceView(bloomResultResource_.Get(), &srvDesc, srvCPUHandle[2]);

    // ブラー4回目用SRV
    index[3] = srvManager_->AllocateSrvIndex(SrvHeapType::System);
    srvCPUHandle[3] = srvManager_->GetCPUHandle(index[3]);
    bloomSRVHandle_[3] = srvManager_->GetGPUHandle(index[3]);
    device_->CreateShaderResourceView(bloomCompositeResource_.Get(), &srvDesc, srvCPUHandle[3]);
    bloomIndex_ = index[3];

    LogManager::GetInstance().Log("End Create BloomRenderTargets\n");
}

void PostEffectManager::DrawBloom(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE currentSrv, const D3D12_VIEWPORT& baseViewport, const D3D12_RECT& baseScissorRect) {
    // SRVヒープをセット
    ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // ルート
    commandList->SetGraphicsRootSignature(bloomPSO_->GetRootSignature());

    // 明るい部分を抽出
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    // バリアを張る対象のリソース
    barrier.Transition.pResource = bloomBrightResource_.Get();
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // レンダーターゲット設定
    commandList->SetPipelineState(bloomPSO_->GetBrightPipelineState());
    commandList->OMSetRenderTargets(1, &bloomRTVHandle_[0], false, nullptr);

    commandList->SetGraphicsRootConstantBufferView(1, bloomPSO_->GetBloomParameterResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, currentSrv);

    // 明るい部分抽出の描画
    bloomPSO_->Draw(commandList, bloomSRVHandle_[0]);

    // bloomBrightResourceをSRVに戻す
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移

    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);

    ///=============================

    // 縮小させながらブラー
    barrier.Transition.pResource = bloomBlurShrinkResource_.Get();
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    auto desc = bloomBrightResource_->GetDesc();
    D3D12_VIEWPORT viewport = {};
    D3D12_RECT rect = {};
    // 描画サイズ
    viewport.Height = static_cast<FLOAT>(desc.Height / 2);
    viewport.Width = static_cast<FLOAT>(desc.Width / 2);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    // rect
    rect.left = 0;
    rect.right = static_cast<LONG>(viewport.Width);
    rect.top = 0;
    rect.bottom = static_cast<LONG>(viewport.Height);

    // レンダーターゲット設定
    commandList->SetPipelineState(bloomPSO_->GetBlurPipelineState());
    commandList->OMSetRenderTargets(1, &bloomRTVHandle_[1], false, nullptr);

    // 縮小させながらブラーをかけたものを描画
    for (uint32_t i = 0; i < bloomPSO_->GetBloomIteration(); ++i) {
        commandList->RSSetViewports(1, &viewport); // Viewportを設定
        commandList->RSSetScissorRects(1, &rect); // Scirssorを設定
        commandList->SetGraphicsRootDescriptorTable(2, drawObjectSRVHandle_);
        bloomPSO_->Draw(commandList, bloomSRVHandle_[0]);

        rect.top += static_cast<LONG>(viewport.Height);
        viewport.TopLeftX = 0;
        viewport.TopLeftY = static_cast<FLOAT>(rect.top);

        viewport.Width /= 2;
        viewport.Height /= 2;
        rect.bottom = rect.top + static_cast<LONG>(viewport.Height);
    }

    // bloomBlurShrinkResourceをSRVに戻す
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);

    //=================================

    // ブラーの最終結果
    barrier.Transition.pResource = bloomResultResource_.Get();
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // ブラーの最終結果を描画
    commandList->SetPipelineState(bloomPSO_->GetResultPipelineState());
    commandList->OMSetRenderTargets(1, &bloomRTVHandle_[2], false, nullptr);
    // ビューポートを元に戻す
    commandList->RSSetViewports(1, &baseViewport);
    commandList->RSSetScissorRects(1, &baseScissorRect);

    commandList->SetGraphicsRootConstantBufferView(1, bloomPSO_->GetBloomParameterResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, drawObjectSRVHandle_);
    bloomPSO_->Draw(commandList, bloomSRVHandle_[0]);

    // bloomResultResourceをSRVに戻す
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);

    //====================================-

    // 合成用のリソースがある場合はバリアを設定
    barrier.Transition.pResource = bloomCompositeResource_.Get();
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // 合成したのを描画
    commandList->SetPipelineState(bloomPSO_->GetBlurCompositePipelineState());
    commandList->OMSetRenderTargets(1, &bloomRTVHandle_[3], false, nullptr);
    commandList->SetGraphicsRootConstantBufferView(1, bloomPSO_->GetBloomParameterResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, drawObjectSRVHandle_);
    bloomPSO_->Draw(commandList, bloomSRVHandle_[0]);

    // 最終的にすべてのブルームリソースをSRVに戻す
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
}

void PostEffectManager::InitializePostEffectData(uint32_t width, uint32_t height, uint32_t descriptorSizeRTV) {

    // ヴィネットの生成
    LogManager::GetInstance().Log("Start Create VignettingRenderTargets");

    // ヴィネットのポストエフェクトのリソースを作成する
    CreatePostEffectResources(
        postProcessRTVHeap_.Get(), rtvIndex_, descriptorSizeRTV,
        width, height,
        vignettingData_.resource,
        vignettingData_.rtvHandle,
        vignettingData_.srvHandle,
        vignettingData_.srvIndex
    );

    // psoデータを取得する
    vignettingData_.psoData = &psoList_[static_cast<size_t>(PSOType::Vignetting)];

    // パラメータリソースを作成
    vignettingResource_.CreateResource(device_);
    vignettingResource_.GetData()->intensity = 16.0f;
    vignettingResource_.GetData()->time = 0.15f;
    vignettingResource_.GetData()->textureHandle = bloomIndex_;

    LogManager::GetInstance().Log("End Create VignettingRenderTargets\n");

    // スキャンラインの生成
    LogManager::GetInstance().Log("Start Create ScanLineRenderTargets");

    // スキャンラインのポストエフェクトのリソースを作成する
    CreatePostEffectResources(
        postProcessRTVHeap_.Get(), rtvIndex_, descriptorSizeRTV,
        width, height,
        scanLineData_.resource,
        scanLineData_.rtvHandle,
        scanLineData_.srvHandle,
        scanLineData_.srvIndex
    );

    // psoデータを取得する
    scanLineData_.psoData = &psoList_[static_cast<size_t>(PSOType::ScanLine)];

    // パラメータリソースを作成
    scanLineResource_.CreateResource(device_);
    scanLineResource_.GetData()->interval = 96.0f;
    scanLineResource_.GetData()->speed = -2.0f;
    scanLineResource_.GetData()->time = 0.0f;
    scanLineResource_.GetData()->lineColor = { 0.3f,0.3f,0.3f };
    scanLineResource_.GetData()->textureHandle = vignettingData_.srvIndex;

    LogManager::GetInstance().Log("End Create ScanLineRenderTargets\n");

    // ラジアルブラーの生成
    LogManager::GetInstance().Log("Start Create RadialBlurRenderTargets");

    // ラジアルブラーのポストエフェクトのリソースを作成する
    CreatePostEffectResources(
        postProcessRTVHeap_.Get(), rtvIndex_, descriptorSizeRTV,
        width, height,
        radialBlurData_.resource,
        radialBlurData_.rtvHandle,
        radialBlurData_.srvHandle,
        radialBlurData_.srvIndex
    );

    // psoデータを取得する
    radialBlurData_.psoData = &psoList_[static_cast<size_t>(PSOType::RadialBlur)];

    // パラメータリソースを作成
    radialBlurResource_.CreateResource(device_);
    radialBlurResource_.GetData()->centerPos = { 0.5f,0.5f };
    radialBlurResource_.GetData()->numSamles = 2;
    radialBlurResource_.GetData()->blurWidth = 0.01f;
    radialBlurResource_.GetData()->textureHandle = vignettingData_.srvIndex;

    LogManager::GetInstance().Log("End Create RadialBlurRenderTargets");

    // アウトラインの生成
    LogManager::GetInstance().Log("Start Create OutLineRenderTargets");

    // アウトラインのポストエフェクトのリソースを作成する
    CreatePostEffectResources(
        postProcessRTVHeap_.Get(), rtvIndex_, descriptorSizeRTV,
        width, height,
        outLineData_.resource,
        outLineData_.rtvHandle,
        outLineData_.srvHandle,
        outLineData_.srvIndex
    );
    LogManager::GetInstance().Log("End Create OutLineRenderTargets");
}

void  PostEffectManager::DrawOutLine(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE depthSRV, D3D12_GPU_DESCRIPTOR_HANDLE currentSrv) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    // バリアを張る対象のリソース
    barrier.Transition.pResource = outLineData_.resource.Get();
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // レンダーターゲット設定
    commandList->SetGraphicsRootSignature(outLinePSO_->GetRootSignature());
    commandList->SetPipelineState(outLinePSO_->GetPipelineState());
    commandList->OMSetRenderTargets(1, &outLineData_.rtvHandle, false, nullptr);

    // アウトラインを描画
    outLinePSO_->Draw(commandList, currentSrv, depthSRV);

    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
}

void PostEffectManager::DrawEffect(ID3D12GraphicsCommandList* commandList, EffectData data, ID3D12Resource* resource) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    // バリアを張る対象のリソース
    barrier.Transition.pResource = data.resource.Get();
    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // レンダーターゲット設定
    commandList->OMSetRenderTargets(1, &data.rtvHandle, false, nullptr);
    commandList->SetGraphicsRootSignature(data.psoData->rootSignature);
    commandList->SetPipelineState(data.psoData->graphicsPipelineState);
    commandList->SetGraphicsRootDescriptorTable(0, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, resource->GetGPUVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);

    // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
}

void PostEffectManager::CreatePostEffectResources(
    ID3D12DescriptorHeap* rtvHeap,
    uint32_t& rtvIndex,
    uint32_t descriptorSizeRTV,
    uint32_t width,
    uint32_t height,
    Microsoft::WRL::ComPtr<ID3D12Resource>& resource,
    D3D12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE& srvGpuHandle,
    uint32_t& srvIndex
) {

    // テクスチャリソース作成
    D3D12_RESOURCE_DESC desc{};
    desc.Width = width;// テクスチャの幅
    desc.Height = height;// テクスチャの高さ
    desc.MipLevels = 1;// mipMapの数
    desc.DepthOrArraySize = 1;//  奥行 or 配列Textureの配列数
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // TextureのFormat
    desc.SampleDesc.Count = 1;// ダンプリングのカウント
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    // 利用するHeapの設定。
    CD3DX12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr;
    hr = device_->CreateCommittedResource(
        &heapProps, // Heapの設定
        D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし
        &desc, // Resourceの設定
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // データの転送される設定
        &clearValue_, // Clearの最適値
        IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
    assert(SUCCEEDED(hr));

    // RTV作成
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // オブジェクト描画用RTVを作成
    rtvHandle = GetCPUDescriptorHandle(rtvHeap, descriptorSizeRTV, ++rtvIndex);
    device_->CreateRenderTargetView(resource.Get(), &rtvDesc, rtvHandle);

    // SRV作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    // SRVハンドル取得
    srvIndex = srvManager_->AllocateSrvIndex(SrvHeapType::System);
    D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle = srvManager_->GetCPUHandle(srvIndex);
    srvGpuHandle = srvManager_->GetGPUHandle(srvIndex);

    // オブジェクト描画用SRV
    device_->CreateShaderResourceView(resource.Get(), &srvDesc, srvCPUHandle);
}