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

同じ音を続けて鳴らすと、前の音を止めずに重ねて再生されます。

| 関数 | 説明 |
|---|---|
| `Play(handle, volume, isLoop, groupName)` | 再生する。BGM は `isLoop` を `true` にします。`groupName` は省略できます |
| `Stop(handle)` | 止める。同じハンドルで重ねて鳴らしている音はすべて止まります |
| `IsPlay(handle)` | 再生中か調べる |
| `StopAll()` | すべての音を止める |

```{tip}
ハンドルの取り出しは、コンストラクタなどで1回だけ行い、メンバー変数に保存しておくと便利です。
```

## 音量を調整する

BGM や SE など、音をグループに分けてまとめて音量を変えられます。
グループの名前は自由に決められます。

```cpp
auto& audio = AudioManager::GetInstance();

// グループを登録する（名前、初期音量）
audio.RegisterGroup("BGM", 0.8f);
audio.RegisterGroup("SE");

// 再生するときにグループ名を指定する
audio.Play(bgmHandle, 1.0f, true, "BGM");
audio.Play(seHandle, 0.8f, false, "SE");

// オプション画面などで音量を変える（0.0〜1.0）
audio.SetGroupVolume("BGM", 0.5f);
audio.SetMasterVolume(0.8f);
```

実際の音量は「`Play` で指定した音量 × グループの音量 × 全体の音量」になります。
グループや全体の音量を変えると、再生中の音にもすぐ反映されます。

| 関数 | 説明 |
|---|---|
| `RegisterGroup(name, volume)` | グループを登録する。同じ名前で呼ぶと音量だけ変わります |
| `SetGroupVolume(name, volume)` | グループの音量を変える |
| `GetGroupVolume(name)` | グループの音量を取得する。登録されていなければ 1.0 |
| `HasGroup(name)` | グループが登録されているか調べる |
| `SetMasterVolume(volume)` / `GetMasterVolume()` | 全体の音量を変える / 取得する |

```{note}
`groupName` を省略した音や、登録されていないグループ名を指定した音は、グループを通らずに全体の音量だけがかかります。
登録されていない名前を指定したときはログに出るので、名前の打ち間違いに気づけます。
```

```{tip}
グループ名は `constexpr const char* kGroupBGM = "BGM";` のように定数にしておくと、打ち間違いを防げます。
```
