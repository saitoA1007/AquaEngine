#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <queue>
#include <functional>

namespace GameEngine {

    // 前方宣言
    class DXFence;

	/// <summary>
	/// 一時的に作成したGPUリソースなどをGPUの実行が完了してから破棄するクラス
	/// </summary>
	class ResourceGarbageCollector {
	public:
		static ResourceGarbageCollector& GetInstance() {
			static ResourceGarbageCollector instance;
			return instance;
		}

        // フェンス機能を取得。nullptrを設定すると以降は即時破棄になる
        void SetFence(DXFence* fence) {
            fence_ = fence;
        }

        // 遅延破棄が有効か
        bool IsActive() const { return fence_ != nullptr; }

        /// <summary>
        /// 破棄したいリソースを登録する
        /// </summary>
        /// <param name="resource">破棄するリソース</param>
        void Add(Microsoft::WRL::ComPtr<ID3D12Resource> resource);

        /// <summary>
        /// GPUの実行完了後に行いたい処理を登録する
        /// </summary>
        void AddDeferred(std::function<void()> action);

        /// <summary>
        /// 毎フレーム呼び出し、安全になったリソースを解放する
        /// </summary>
        void ProcessCompletedResources();

        /// <summary>
        /// 登録されているものをすべて解放する。GPUの完了を待ってから呼ぶこと
        /// </summary>
        void FlushAll();

    private:
        ResourceGarbageCollector() = default;
        ~ResourceGarbageCollector() = default;

        DXFence* fence_ = nullptr;

        struct ReleaseItem {
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            std::function<void()> action;
            uint64_t fenceValue;
        };

        std::queue<ReleaseItem> releaseQueue_;

    private:
        // 1件分を解放する
        static void Release(ReleaseItem& item);
	};
}
