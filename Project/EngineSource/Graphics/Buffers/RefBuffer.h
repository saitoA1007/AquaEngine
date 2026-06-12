#pragma once
#include "BufferRefResource.h"
#include "BufferRefManager.h"

namespace GameEngine {

	/// <summary>
	/// レイトレで使用する参照したいバッファのハンドルを持つ構造体を作成する
	/// </summary>
	class RefBuffer : public BufferRefResource {
	public:
		~RefBuffer() {
			// bufferRefの解放
			if (isCreated_) {
				bufferRefManager_->ReleaseIndex(refIndex_);
			}
		}

		/// <summary>
		/// データを作成
		/// </summary>
		/// <param name="type">バッファのタイプを設定</param>
		void Create() {
			// 参照用データを作成
			refIndex_ = bufferRefManager_->AllocateIndex();
			refData_ = bufferRefManager_->GetBufferRef(refIndex_);
			isCreated_ = true;
		}

		// アクセス用のsrvインデックス
		const uint32_t& GetRefIndex() const { return refIndex_; }
		BufferRef* GetRefData() const { return refData_; }

		// 使用するタイプを設定
		void SetType(const uint32_t& type) {
			refData_->type = type;
		}

		// バッファの使用タイプを設定
		void SetBufferMaterial(const uint32_t& type,const uint32_t& materialSrvIndex) {
			refData_->type = type;
			refData_->materialIndex = materialSrvIndex - bufferStartIndex_;
		}

		// モデル情報を設定する
		void SetModelData(const uint32_t& vertexHandle, const uint32_t& indexHandle) {
			refData_->indexHandle = indexHandle - bufferStartIndex_;
			refData_->vertexHandle = vertexHandle - bufferStartIndex_;
		}

		// 使用するヒットグループを設定
		void SetHitGroupIndex(uint32_t index) {
			useHitGroupIndex_ = index;
		}

		// 使用するヒットグループを取得
		uint32_t GetUseHitGroupIndex() const { return useHitGroupIndex_; }

	private:
		uint32_t refIndex_ = 0;
		BufferRef* refData_ = nullptr;

		// 使用するヒットグループ
		uint32_t useHitGroupIndex_ = 0;

		bool isCreated_ = false;
	};
}