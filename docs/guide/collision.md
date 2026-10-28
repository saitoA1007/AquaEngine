# 当たり判定

当たり判定は、コライダー（当たりの形）をメンバー変数として持つだけで使えます。
コライダーは**作られたときに自動で登録され、消えたときに自動で解除**されます。
判定はエンジンが毎フレーム行い、ぶつかったときに登録しておいた関数（コールバック）を呼びます。

## コライダーの種類

| クラス | 形 | 主な設定 |
|---|---|---|
| `SphereCollider` | 球 | `SetRadius()` |
| `AABBCollider` | 回転しない箱 | `SetSize()`、`SetAnchorPoint()` |
| `OBBCollider` | 回転する箱 | `SetSize()`、`UpdateOrientationsFromRotate()` |
| `SegmentCollider` | 線分 | `SetDiff()`（始点からの向きと長さ） |

## 使い方

```cpp
// Player.h
#include "Collider.h"

class Player : public GameEngine::IGameObject {
	// ...
private:
	GameEngine::SphereCollider collider_;
	void OnCollisionEnter(const GameEngine::CollisionResult& result);
};
```

```cpp
// Player.cpp
Player::Player() {
	collider_.SetRadius(1.0f);

	// 自分が「プレイヤー」で、「敵」と「地形」にだけ当たる
	collider_.SetCollisionAttribute(kCollisionAttributePlayer);
	collider_.SetCollisionMask(kCollisionAttributeEnemy | kCollisionAttributeTerrain);

	// 相手に渡す自分の情報
	GameEngine::UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kPlayer);
	userData.object = this;
	collider_.SetUserData(userData);

	// ぶつかった瞬間に呼ばれる関数
	collider_.SetOnCollisionEnterCallback([this](const GameEngine::CollisionResult& result) {
		OnCollisionEnter(result);
	});
}

void Player::Update() {
	// 位置は毎フレーム自分で更新する
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());
}
```

```{important}
コライダーの位置はモデルの位置に自動で付いていきません。`Update()` で毎フレーム `SetWorldPosition()` を呼んでください。
```

## ぶつかったときの処理

コールバックは2種類あります。

| 登録する関数 | 呼ばれるタイミング |
|---|---|
| `SetOnCollisionEnterCallback` | ぶつかった瞬間に1回 |
| `SetOnCollisionCallback` | ぶつかっている間、毎フレーム |

コールバックに渡される `CollisionResult` には、次の情報が入っています。

| メンバー | 内容 |
|---|---|
| `userData` | **相手**が `SetUserData()` で設定した情報 |
| `contactPosition` | ぶつかった位置 |
| `contactNormal` | 相手から自分へ押し返す向き |
| `penetrationDepth` | めり込んだ深さ |

相手が何かは `userData.typeID` で調べ、相手のオブジェクトは `userData.As<型>()` で取り出せます（型が違うと `nullptr` になります）。

```cpp
void Player::OnCollisionEnter(const GameEngine::CollisionResult& result) {
	if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kBoss)) {
		if (auto* boss = result.userData.As<BossEnemy>()) {
			// ボスに当たったときの処理
		}
	}
}
```

## 当たる相手を絞る（属性とマスク）

コライダーには「自分が何か」を表す**属性**と、「何と当たるか」を表す**マスク**があります。
A の属性が B のマスクに含まれていて、**かつ** B の属性が A のマスクに含まれているときだけ判定されます。
初期値はどちらもすべてのビットが立っているので、設定しなければ何とでも当たります。

属性と、相手を見分ける ID は `Application/CollisionConfig.h` で決めています。新しい種類を作るときはここに追加してください。

```cpp
static inline const uint32_t kCollisionAttributePlayer  = 0b1;
static inline const uint32_t kCollisionAttributeEnemy   = 0b1 << 1;
static inline const uint32_t kCollisionAttributeTerrain = 0b1 << 2;
```

一時的に判定を止めたいときは `collider_.SetActive(false)` を使います。
