#pragma once
#include <cstdint>

namespace GameEngine {

	/// <summary>
	/// ゲームオブジェクトの基底クラス
	/// </summary>
	class IGameObject {
	public:
		virtual ~IGameObject() = default;

		virtual void Initialize() = 0;
		virtual void Update() = 0;
		virtual void Draw() = 0;

		virtual void DebugUpdate() = 0;

	public:

		// 更新処理順
		int32_t GetUpdateOrder() const { return updateOrder_; }
		void SetUpdateOrder(int32_t order) { updateOrder_ = order; }

		// 有効状態
		bool IsActive() const { return isActive_; }
		void SetActive(bool active) { isActive_ = active; }

		// 削除管理
		bool IsDead() const { return isDead_; }
		void Destroy() { isDead_ = true; }

	protected:
		// 更新処理順
		int32_t updateOrder_ = 0;
		// 有効状態
		bool isActive_ = true;
		// 削除管理
		bool isDead_ = false;
	};
}