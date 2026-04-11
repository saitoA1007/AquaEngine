#pragma once
#include "GpuResource.h"
#include "SrvManager.h"

namespace GameEngine {

    /// <summary>
    /// GpuResourceクラスにsrvの作成を拡張した汎用クラス
    /// </summary>
    class SrvResource : public GpuResource {
    public:
        virtual ~SrvResource() = default;

        /// <summary>
        /// 静的初期化
        /// </summary>
        static void StaticInitialize(SrvManager* srvManager) {
            srvManager_ = srvManager;
        }

    protected:
        // srvManager
        static SrvManager* srvManager_;
    };
}