#pragma once
#include <string>
#include "SrvManager.h"
#include "ConstantBuffer.h"

namespace GameEngine {

	class IPostEffect {
	public:
		virtual ~IPostEffect() = default;

		// 更新処理
		virtual void Update() {}

		// 描画処理
		virtual void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) = 0;

	public:

		// ポストエフェクトを掛ける描画パスを取得
		virtual void SetPassIndex(const uint32_t& index) = 0;

		// 登録
		void Register(const uint32_t& passIndex, const std::string& psoName) {
			passIndex_ = passIndex;
			psoName_ = psoName;
		}

		// グラフ用
		// 入力ピンの数
		virtual uint32_t GetInputCount() const { return 1; }
		// 入力ピンの名前
		virtual const char* GetInputName(uint32_t slot) const { return "Input"; }
		// スロット指定で入力SRVを設定。slot0は既存のSetPassIndexに流す
		virtual void SetInputIndex(uint32_t slot, uint32_t srvIndex) {
			if (slot == 0) { SetPassIndex(srvIndex); }
		}
		// ノードのタイトル
		virtual const char* GetDisplayName() const { return psoName_.c_str(); }
		// ノード内に出すパラメータUI
		virtual void DrawParamUI() {}

		// 使用するpassのインデックス
		const uint32_t& GetPassIndex() const { return passIndex_; }
		// 使用するPSOの名前
		const std::string& GetPsoName() const { return psoName_; }
		// 有効状態
		void SetIsActive(const bool& isActive) { isActive_ = isActive; }
		const bool& IsActive()const { return isActive_; }
	protected:
		// 使用するpass名前
		uint32_t passIndex_ = 0;
		// 使用するpso
		std::string psoName_;

		// 有効状態
		bool isActive_ = false;
	};
}

