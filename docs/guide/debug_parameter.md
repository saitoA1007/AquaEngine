# デバッグパラメータ

`DebugParameter` を使うと、変数をエディタ（ImGui）から調整でき、調整した値を JSON に保存できます。
移動速度や配置などの調整値を、ビルドし直さずに変えたいときに使います。

## 使い方

```cpp
// Test.h
#include "DebugParameter.h"

class Test : public GameEngine::IGameObject {
	// ...
private:
	// エディタに表示されるグループ名を指定する
	GameEngine::DebugParameter debugParame_{ "Test" };

	float speed_ = 0.0f;
	Vector3 startPos_ = {};
	GameEngine::ModelComponent modelComponent_;
};
```

```cpp
// Test.cpp
Test::Test(GameEngine::Model* model) : modelComponent_(model) {
	// 調整したい変数を登録する
	debugParame_.Register("Speed", speed_);
	debugParame_.Register("StartPos", startPos_);
	// WorldTransform の大きさ・回転・位置をまとめて登録する
	debugParame_.RegisterWorld("", modelComponent_.worldTransform_);

	// 保存されている値を変数に読み込む
	debugParame_.Apply();
}

void Test::Update() {
	// エディタで値が変更されたときだけ反映する
	debugParame_.ApplyIfDirty();

	modelComponent_.Update();
}
```

## 登録できるもの

| 関数 | 内容 |
|---|---|
| `Register(名前, 変数)` | 1つの変数（`float`、`Vector3`、`Vector4` など） |
| `RegisterWorld(名前, WorldTransform)` | 大きさ・回転・位置 |
| `RegisterSprite(名前, Sprite)` | スプライトの位置・大きさなど |
| `RegisterText(名前, Text)` | テキストの位置・大きさなど |

`Register` の3つ目の引数で表示する順番、4つ目の引数でサブグループ（グループ内の見出し）を指定できます。

```cpp
int index = 0;
parame_->Register("ModelScale", scale_, index++, "Wall");
parame_->Register("MaxHp", maxHp_, index++, "Wall");
```

## 保存場所

エディタで保存すると、値は `Resources/Json/GameData/<グループ名>.json` に書き込まれます。
次に起動したときは、`Apply()` を呼んだ時点でこのファイルの値が変数に入ります。
