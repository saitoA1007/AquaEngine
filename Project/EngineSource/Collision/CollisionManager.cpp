#include"CollisionManager.h"
#include"CollisionVisitor.h"
using namespace GameEngine;

void CollisionManager::CheckAllCollisions() {

	// 現フレームの衝突リストをクリア
	currentCollisions_.clear();

	// 何も無ければ早期リターン
	if (colliders_.empty()) {
		// 前フレームの情報も更新
		preCollisions_.clear();
		return;
	}

	// リスト内のペアを総当たり
	std::list<Collider*>::iterator itrA = colliders_.begin();
	for (; itrA != colliders_.end(); ++itrA) {
		// コライダーAを取得
		Collider* colliderA = *itrA;

		// イテレーターBはイテレータAの次の要素から回す(重複判定を回避)
		std::list<Collider*>::iterator itrB = itrA;
		itrB++;

		for (; itrB != colliders_.end(); ++itrB) {
			// コライダーBを取得
			Collider* colliderB = *itrB;

			// ペアの当たり判定
			CheckCollisionPair(colliderA, colliderB);
		}
	}

	// 現在の当たり判定内容を保存
	preCollisions_ = currentCollisions_;
}

void  CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB) {

	// 衝突フィルタリング
	if (!IsActiveCollision(colliderA, colliderB)) {
		return;
	}

	// 形状を取得
	CollisionType typeA = colliderA->GetCollisionType();
	CollisionType typeB = colliderB->GetCollisionType();

	// 各形状に応じた当たり判定を取得
	CollisionResult result = CheckCollisionType(typeA, typeB);

	if (result.isHit) {

		// 現在の衝突ペアを記録
		auto collisionPair = MakeCollisionPair(colliderA, colliderB);
		currentCollisions_.insert(collisionPair);

		// 前フレームで衝突していたか確認
		bool wasHit = WasCollidingLastFrame(colliderA, colliderB);

		result.userData = colliderB->GetUserData();
		// 反対側の法線情報を渡す
		CollisionResult resultB = result;
		resultB.contactNormal = result.contactNormal * -1.0f;
		resultB.userData = colliderA->GetUserData();

		// コライダーの衝突時コールバックを呼び出す
		if (wasHit) {
			// 衝突している間に呼び出す
			colliderA->OnCollision(result);
			colliderB->OnCollision(resultB);
		} else {
			// 衝突した瞬間に呼び出す
			colliderA->OnCollisionEnter(result);
			colliderB->OnCollisionEnter(resultB);
		}
	}
}

bool CollisionManager::IsActiveCollision(Collider* a, Collider* b) {
	return (a->GetCollisionAttribute() & b->GetCollisionMask()) != 0 && (b->GetCollisionAttribute() & a->GetCollisionMask()) != 0;
}

CollisionResult CollisionManager::CheckCollisionType(const CollisionType& typeA, const CollisionType& typeB) {
	// 各形状の当たり判定を取得する
	return std::visit(CollisionVisitor{}, typeA.type, typeB.type);
}

std::pair<Collider*, Collider*> CollisionManager::MakeCollisionPair(Collider* a, Collider* b) {
	// ポインタの値で順序を固定する
	if (a < b) {
		return { a, b };
	} else {
		return { b, a };
	}
}

bool CollisionManager::WasCollidingLastFrame(Collider* a, Collider* b) {
	auto collisionPair = MakeCollisionPair(a, b);
	return preCollisions_.find(collisionPair) != preCollisions_.end();
}