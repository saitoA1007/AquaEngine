#pragma once
#include "BufferRefResource.h"
#include "StructuredBuffer.h"
#include "BufferRefManager.h"

namespace GameEngine {

	/// <summary>
	/// マテリアルを作成する構造体
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template <typename T>
	class MaterialBuffer : public BufferRefResource {
	public:
		~MaterialBuffer() {
			if (isCreated_) {
				bufferRefManager_->ReleseIndex(index_);
			}
		}

		/// <summary>
		/// マテリアルデータを作成
		/// </summary>
		/// <param name="type">マテリアルのタイプを設定</param>
		void Create(const uint32_t& type) {

			// マテリアルデータを作成
			materialDataBuffer_.Create();

			index_ = bufferRefManager_->AllocateIndex();
			auto* data = bufferRefManager_->GetBufferRef(index_);

			// マテリアルのsrv番号を設定
			data->type = type; // タイプを設定
			data->index = materialDataBuffer_.GetSrvIndex(); // srvの番号を設定

			isCreated_ = true;
		}

		// アクセス用のsrvインデックス
		const uint32_t& GetRefIndex() const { return index_; }

	public:
		// マテリアルのデータ用
		StructuredBuffer<T> materialDataBuffer_;

	private:
		uint32_t index_ = 0;

		bool isCreated_ = false;
	};
}