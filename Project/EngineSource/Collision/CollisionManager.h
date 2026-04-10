#pragma once
#include<list>
#include<unordered_set>

#include"Collider.h"
#include"CollisionResult.h"

namespace GameEngine {

	// ペアのハッシュ関数
	struct PairHash {
		std::size_t operator()(const std::pair<Collider*, Collider*>& p) const {
			// ポインタのアドレスをハッシュ化
			auto h1 = std::hash<Collider*>{}(p.first);
			auto h2 = std::hash<Collider*>{}(p.second);
			// 2つのハッシュ値を組み合わせる
			return h1 ^ (h2 << 1);
		}
	};

	class CollisionManager {
	public:

		/// <summary>
		/// 全ての当たり判定を行う
		/// </summary>
		void CheckAllCollisions();

		/// <summary>
		/// コライダーリストをクリアする
		/// </summary>
		void ClearList() { colliders_.clear(); }

		/// <summary>
		/// コライダーをリストに追加する
		/// </summary>
		/// <param name="collider"></param>
		void AddCollider(Collider* collider) { colliders_.push_back(collider); }

	private:
		// コライダーリスト
		std::list<Collider*> colliders_;

		// 衝突ペアの履歴
		std::unordered_set<std::pair<Collider*, Collider*>, PairHash> preCollisions_;
		std::unordered_set<std::pair<Collider*, Collider*>, PairHash> currentCollisions_;

	private:

		/// <summary>
		/// コライダー2つの衝突判定と応答
		/// </summary>
		/// <param name="colliderA">コライダーA</param>
		/// <param name="colliderB">コライダーB</param>
		void CheckCollisionPair(Collider* colliderA, Collider* colliderB);

		/// <summary>
		/// 衝突出来るか判定する
		/// </summary>
		/// <param name="a"></param>
		/// <param name="b"></param>
		/// <returns></returns>
		bool IsActiveCollision(Collider* a, Collider* b);

		/// <summary>
		/// 形状に応じた当たり判定を実行する
		/// </summary>
		/// <param name="typeA"></param>
		/// <param name="typeB"></param>
		/// <returns></returns>
		CollisionResult CheckCollisionType(const CollisionType& typeA, const CollisionType& typeB);

		/// <summary>
		/// 衝突ペアを作成
		/// </summary>
		std::pair<Collider*, Collider*> MakeCollisionPair(Collider* a, Collider* b);

		/// <summary>
		/// 前フレームで衝突していたか確認
		/// </summary>
		bool WasCollidingLastFrame(Collider* a, Collider* b);
	};
}