#pragma once
#include "StructuredBuffer.h"

namespace GameEngine {

	// マテリアル情報へのアクセスデータ
	struct MaterialRef {
		uint32_t type;  // マテリアルデータのタイプ
		uint32_t MaterialIndex = 0; // マテリアルデータの参照するハンドル
	};

	// マテリアルタイプ
	enum class MaterialType {
		kDefalut,

		kMaxCount
	};

	/// <summary>
	/// マテリアルを作成する構造体
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template <typename T>
	class MaterialBuffer {
	public:
		~MaterialBuffer() {}

		/// <summary>
		/// マテリアルデータを作成
		/// </summary>
		/// <param name="type">マテリアルのタイプを設定</param>
		void Create(const uint32_t& type) {

			// マテリアルへのアクセスデータを作成
			materialRefBuffer_.Create(1,SrvHeapType::AccessData);

			// マテリアルデータを作成
			materialDataBuffer_.Create();

			// マテリアルのsrv番号を設定
			MaterialRef* materialRef = materialRefBuffer_.GetData();
			materialRef->type = type; // タイプを設定
			materialRef->MaterialIndex = materialDataBuffer_.GetSrvIndex(); // srvの番号を設定
		}

	public:
		// マテリアルのデータ用
		StructuredBuffer<T> materialDataBuffer_;

	private:

		// マテリアルにアクセスするためのデータ保存用
		StructuredBuffer<MaterialRef> materialRefBuffer_;
	};
}