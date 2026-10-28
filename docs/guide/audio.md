# サウンドを鳴らす

音声ファイル（`.wav` / `.mp3`）を `Resources/Sounds/` に置くと、起動時に自動で読み込まれます。
音は `AudioManager::GetInstance()` からどこでも鳴らせます。

```cpp
#include "AudioManager.h"
using namespace GameEngine;

// ファイル名（拡張子付き）からハンドルを取り出す
uint32_t seHandle = AudioManager::GetInstance().GetHandleByName("se.mp3");

// 鳴らす（ハンドル、音量、ループするか）
AudioManager::GetInstance().Play(seHandle, 0.5f, false);
```

| 関数 | 説明 |
|---|---|
| `Play(handle, volume, isLoop)` | 再生する。BGM は `isLoop` を `true` にします |
| `Stop(handle)` | 止める |
| `IsPlay(handle)` | 再生中か調べる |
| `StopAll()` | すべての音を止める |

```{tip}
ハンドルの取り出しは、コンストラクタなどで1回だけ行い、メンバー変数に保存しておくと便利です。
```
