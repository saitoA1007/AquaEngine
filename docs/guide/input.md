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

`RegisterCommand(名前, 入力の一覧)` で登録します。入力はデバイスごとの型（`KeyBinding` など）で書き、どれか1つを満たすとそのコマンドが有効になります。

```cpp
using namespace GameEngine;

// スペースキー、またはパッドの X ボタンを押した瞬間
inputCommand_->RegisterCommand("Decision", {
	KeyBinding{ DIK_SPACE, PressType::kTrigger },
	PadButtonBinding{ XINPUT_GAMEPAD_X, PressType::kTrigger },
});

// A キーを押している間、左スティックを左に倒している間、十字キーの左を押している間
inputCommand_->RegisterCommand("MoveLeft", {
	KeyBinding{ DIK_A },
	PadStickBinding{ PadSide::kLeft, { -1.0f, 0.0f } },
	PadButtonBinding{ XINPUT_GAMEPAD_DPAD_LEFT },
});

// 右クリックを離した瞬間、または R2 を押した瞬間
inputCommand_->RegisterCommand("Shot", {
	MouseBinding{ 1, PressType::kRelease },
	PadTriggerBinding{ PadSide::kRight, PressType::kTrigger },
});
```

### 調べる

```cpp
if (inputCommand_->IsCommandActive("MoveLeft")) {
	position_.x -= speed_;
}
```

### 入力の種類

| 型 | 書き方 | 意味 |
|---|---|---|
| `KeyBinding` | `{ キー, 押し方 }` | キーボード。キーは `DIK_〜` |
| `MouseBinding` | `{ ボタン, 押し方 }` | マウス。ボタンは 0:左、1:右、2:中 |
| `PadButtonBinding` | `{ ボタン, 押し方 }` | パッドのボタン。ボタンは `XINPUT_GAMEPAD_〜` |
| `PadStickBinding` | `{ 左右, 方向, しきい値 }` | スティックを指定した方向に倒している間 |
| `PadTriggerBinding` | `{ 左右, 押し方, しきい値 }` | L2 / R2 トリガー。しきい値は 0〜1 |

押し方（`PressType`）は次の3つです。省略すると `kPush` になります。しきい値は省略すると `0.2f` です。

| `PressType` | 意味 |
|---|---|
| `kPush` | 押している間 |
| `kTrigger` | 押した瞬間 |
| `kRelease` | 離した瞬間 |

左右（`PadSide`）は `kLeft` / `kRight` で指定します。

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
