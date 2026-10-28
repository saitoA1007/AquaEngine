# シーンを作る

シーンは、タイトル画面やゲーム画面のような「場面」の単位です。
エンジンは常に1つのシーンを動かしていて、シーンが終わると次のシーンに切り替えます。

## 1. シーンクラスを作る

`GameEngine::IScene` を継承し、次の関数をすべて実装します。

| 関数 | 呼ばれるタイミング |
|---|---|
| `Initialize()` | シーンが作られた直後に1回 |
| `Update()` | 毎フレーム |
| `DebugUpdate()` | エディタで一時停止している間も毎フレーム（ImGui での調整用） |
| `Draw()` | 毎フレーム、描画のとき |
| `NextSceneName()` | シーンが終わったとき。次のシーンの名前を返す |
| `GetTransitionEffect()` | シーンが終わったとき。切り替えの演出（フェードなど）を返す |

```cpp
// Application/Scene/TitleScene.h
#pragma once
#include "IScene.h"
#include "Camera.h"
#include "Application/Scene/Transition/Fade.h"

class TitleScene : public GameEngine::IScene {
public:
	TitleScene();

	void Initialize() override;
	void Update() override;
	void DebugUpdate() override;
	void Draw() override;

	// 次は "Game" シーンへ進む
	std::string NextSceneName() override { return "Game"; }

	// フェードで切り替える
	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private:
	std::unique_ptr<GameEngine::Camera> mainCamera_;
};
```

## 2. シーンからエンジンの機能を使う

`IScene` を継承すると、次のメンバー変数がすぐに使えます。エンジンが起動時に設定してくれるので、自分で用意する必要はありません。

| 変数 | できること |
|---|---|
| `input_` | キーボード・マウス・パッドの入力を直接調べる |
| `inputCommand_` | 名前を付けた入力（コマンド）を調べる（[入力を受け取る](input.md)） |
| `textureManager_` / `modelManager_` / `animationManager_` / `fontManager_` | 読み込み済みの素材を取り出す（[リソースの読み込み](resources.md)） |
| `gameObjectManager_` | ゲームオブジェクトを生成する（[ゲームオブジェクトを作る](game_object.md)） |
| `renderQueue_` | 描画の命令を出す（[描画する](rendering.md)） |
| `effectsManager_` / `postEffectManager_` | エフェクトやポストエフェクトを使う |
| `debugRenderer_` | デバッグ用の線を描く |

```cpp
// Application/Scene/TitleScene.cpp
#include "pch.h"
#include "TitleScene.h"
using namespace GameEngine;

TitleScene::TitleScene() {
	// 決定ボタンを登録
	inputCommand_->RegisterCommand("Decision", { {InputState::KeyTrigger, DIK_SPACE} });

	// カメラを用意
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,1.0f,-15.0f} }, 1280, 720);
}

void TitleScene::Initialize() {}

void TitleScene::Update() {
	mainCamera_->Update();

	// スペースキーでシーンを終える
	if (inputCommand_->IsCommandActive("Decision")) {
		isFinished_ = true;
	}
}

void TitleScene::DebugUpdate() {}

void TitleScene::Draw() {
	// このカメラで描画する
	renderQueue_->SetCamera(mainCamera_.get());
}
```

## 3. シーンを登録する

作ったシーンは `Application/Scene/Register/SetUpScenes.h` の `SetupScenes()` に登録します。
ここで付けた名前が、`NextSceneName()` で使う名前になります。

```cpp
void SetupScenes(SceneRegistry& factory) {
    factory.RegisterScene<TitleScene>("Title");
    factory.RegisterScene<GameScene>("Game");

    // 起動したときに最初に動くシーン
    factory.SetDefaultScene("Title");
}
```

## シーンを切り替える

シーンの中で `isFinished_ = true;` にすると、次の順で切り替わります。

1. `GetTransitionEffect()` で返した演出が始まる（この間、今のシーンの `Update()` は止まります）
2. 演出の途中（`ITransitionEffect::IsMidTransition()` が `true` になったとき）で、`NextSceneName()` のシーンに入れ替わる
3. 前のシーンのゲームオブジェクトはすべて削除される

```{note}
シーンが切り替わると、`gameObjectManager_` に登録したオブジェクトは自動で削除されます。
シーンをまたいで残したいデータは、シーンの外で持つ必要があります。
```

### 切り替えの演出を自作する

`ITransitionEffect` を継承します。`GetMaxTime()` で演出の長さ（秒）を返すと、`Update(float timer)` には進み具合が 0〜1 の値で渡されます。
例は `Application/Scene/Transition/Fade.h` を見てください。
