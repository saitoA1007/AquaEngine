# 入力を受け取る

入力の受け取り方は2つあります。

| 方法 | 向いている場面 |
|---|---|
| `inputCommand_`（コマンド） | ゲームの操作。キーボードとパッドを1つの名前にまとめられるので、こちらがおすすめです。 |
| `input_`（直接） | 特定のキーやマウスの位置を直接調べたいとき |

どちらもシーン（`IScene`）のメンバーとして使えます。
ゲームオブジェクトで使いたいときは、シーンからコンストラクタで渡してください。

## コマンドを使う

### 登録する

`RegisterCommand(名前, 条件の一覧)` で登録します。条件のどれか1つを満たすと、そのコマンドが有効になります。

```cpp
using namespace GameEngine;

// スペースキー、またはパッドの X ボタンを押した瞬間
inputCommand_->RegisterCommand("Decision", {
	{ InputState::KeyTrigger, DIK_SPACE },
	{ InputState::PadTrigger, XINPUT_GAMEPAD_X },
});

// A キーを押している間、左スティックを左に倒している間、十字キーの左を押している間
inputCommand_->RegisterCommand("MoveLeft", {
	{ InputState::KeyPush, DIK_A },
	{ InputState::PadLeftStick, 0, { -1.0f, 0.0f }, 0.2f }, // 方向、しきい値
	{ InputState::PadPush, XINPUT_GAMEPAD_DPAD_LEFT },
});
```

### 調べる

```cpp
if (inputCommand_->IsCommandActive("MoveLeft")) {
	position_.x -= speed_;
}
```

### 条件の種類（InputState）

| 種類 | 意味 |
|---|---|
| `KeyPush` / `KeyTrigger` / `KeyRelease` | キーを押している間 / 押した瞬間 / 離した瞬間。コードは `DIK_〜` |
| `MousePush` / `MouseTrigger` / `MouseRelease` | マウスボタン。コードは 0:左、1:右、2:中 |
| `PadPush` / `PadTrigger` / `PadRelease` | パッドのボタン。コードは `XINPUT_GAMEPAD_〜` |
| `PadLeftStick` / `PadRightStick` | スティックを指定した方向に倒している間 |
| `PadPushLeftTrigger` など | L2 / R2 トリガー |

パッドを振動させるときは `inputCommand_->PlayPadVibration(左, 右)` を使います（0〜1）。

## 直接調べる

```cpp
// キーボード
if (input_->TriggerKey(DIK_ESCAPE)) { /* 押した瞬間 */ }
if (input_->PushKey(DIK_W)) { /* 押している間 */ }

// マウス
if (input_->TriggerMouse(0)) { /* 左クリック */ }
Vector2 mousePos = input_->GetMousePosition();
int32_t wheel = input_->GetWheel();

// パッド
if (input_->IsPadConnected()) {
	Vector2 stick = input_->GetLeftStick();
}
```
