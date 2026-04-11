#pragma once
#include<cstdint>

namespace GameEngine {

	// 使用するヒープのタイプ
	enum class SrvHeapType {
		Texture,      // 画像データ
		System,       // オフスクリーンなど
		Buffer,       // インスタンシング、パーティクルのStructuredBuffer
		Other,        // 他で使用する(現在はImGuiでのみ使用している)

		Count         // 総数
	};

	// 使用するヒープの数
	enum class SrvHeapTypeCount : uint32_t {
		TextureMaxCount = 2048,
		SystemMaxCount = 18,
		BufferMaxCount = 1024,
		OtherMaxCount = 1
	};

	// 最大のsrvインデックス
	static inline const uint32_t kMaxSrvIndex_ = 4096; 
}