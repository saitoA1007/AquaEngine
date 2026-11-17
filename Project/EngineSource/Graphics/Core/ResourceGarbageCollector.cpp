#include "pch.h"
#include "ResourceGarbageCollector.h"
#include "DXFence.h"
using namespace GameEngine;

void ResourceGarbageCollector::Add(Microsoft::WRL::ComPtr<ID3D12Resource> resource) {
    if (!resource) { return; }

    // フェンスが無い(初期化前・終了後)場合は参照を外すだけで即時破棄
    if (fence_ == nullptr) { return; }

    // 現在記録中のフレームの完了をもって破棄する
    releaseQueue_.push({ std::move(resource), nullptr, fence_->GetNextFenceValue() });
}

void ResourceGarbageCollector::AddDeferred(std::function<void()> action) {
    if (!action) { return; }

    if (fence_ == nullptr) {
        action();
        return;
    }

    releaseQueue_.push({ nullptr, std::move(action), fence_->GetNextFenceValue() });
}

void ResourceGarbageCollector::ProcessCompletedResources() {
    if (fence_ == nullptr) { return; }

    uint64_t completedValue = fence_->GetCompletedValue();

    // キューの先頭から、完了したフェンス値以下か確認する
    while (!releaseQueue_.empty()) {
        auto& item = releaseQueue_.front();

        if (item.fenceValue <= completedValue) {
            // 破棄する
            Release(item);
            releaseQueue_.pop();
        } else {
            // まだ実行中のリソースに当たったらそれ以降も実行中なので抜ける
            break;
        }
    }
}

void ResourceGarbageCollector::FlushAll() {
    while (!releaseQueue_.empty()) {
        Release(releaseQueue_.front());
        releaseQueue_.pop();
    }
}

void ResourceGarbageCollector::Release(ReleaseItem& item) {
    item.resource.Reset();
    if (item.action) {
        // 処理の中で再登録される可能性があるため、取り出してから実行する
        auto action = std::move(item.action);
        item.action = nullptr;
        action();
    }
}
