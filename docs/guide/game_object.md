# ゲームオブジェクトを作る

プレイヤーや敵のように、シーンの中で動くものはゲームオブジェクトとして作ります。
ゲームオブジェクトは `GameObjectManager` に登録しておくと、エンジンが毎フレーム `Update()` と `Draw()` を呼んでくれます。

## 1. クラスを作る

`GameEngine::IGameObject` を継承し、必要な関数だけをオーバーライドします（すべて任意です）。

| 関数 | 呼ばれるタイミング |
|---|---|
| `Initialize()` | 登録された直後に1回 |
| `Update()` | 毎フレーム |
| `Draw()` | 毎フレーム、描画のとき |
| `DebugUpdate()` | エディタで一時停止している間も毎フレーム |

`IGameObject` を継承すると、描画用の `renderQueue_` がそのまま使えます。

```cpp
// Application/Demo/Test.h
#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"

class Test : public GameEngine::IGameObject {
public:
	Test(GameEngine::Model* model) : modelComponent_(model) {}

	void Update() override {
		// 位置などを反映する
		modelComponent_.Update();
	}

	void Draw() override {
		modelComponent_.DrawRaytracing(renderQueue_);
	}

private:
	// モデルと、その位置・回転・大きさをまとめたもの
	GameEngine::ModelComponent modelComponent_;
};
```

## 2. シーンで生成する

シーンの中で `gameObjectManager_->AddObject<型>(コンストラクタの引数...)` を呼びます。
生成したオブジェクトのポインタが返ってくるので、あとで操作したいときは保存しておきます。

```cpp
auto* model = modelManager_->GetNameByModel("sphere.obj");
Test* test = gameObjectManager_->AddObject<Test>(model);
```

`Update()` の途中で `AddObject` を呼んでも問題ありません。その場合、追加されたオブジェクトは次のフレームから動きます。

## 3. 削除する

オブジェクト自身の中で `Destroy()` を呼ぶと、そのフレームの更新が終わったあとに削除されます。

```cpp
void Bullet::Update() {
	if (lifeTime_ <= 0.0f) {
		Destroy();
	}
}
```

外から削除するときは `gameObjectManager_->RemoveObject(ptr)` を使います。
削除したオブジェクトのポインタは使えなくなるので、保存していた場合は `nullptr` にしてください。

```{note}
シーンが切り替わると、登録されたゲームオブジェクトはすべて削除されます。
```

## その他の機能

| 関数 | 説明 |
|---|---|
| `SetActive(false)` | 一時的に止める。止めている間は `Update()` / `Draw()` が呼ばれません。 |
| `SetUpdateOrder(n)` | 更新する順番を決める。値が小さいものから先に更新されます（初期値は 1）。並べ替えはオブジェクトが追加されたときにだけ行われるので、コンストラクタか `Initialize()` の中で設定してください。 |
