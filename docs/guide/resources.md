# リソースの読み込み

モデル・画像・音・フォントは、`Project/Resources/` の決まったフォルダに置くだけで、起動時にエンジンが自動で読み込みます。
コードで読み込み処理を書く必要はなく、シーンの中で**ファイル名を指定して取り出す**だけです。

## 置き場所と取り出し方

| 種類 | 置く場所 | 対応形式 | 取り出し方 |
|---|---|---|---|
| モデル | `Resources/Models/`（サブフォルダ可） | `.obj` `.gltf` | `modelManager_->GetNameByModel("player.gltf")` |
| 画像 | `Resources/Textures/`（サブフォルダ可） | `.png` `.dds` など | `textureManager_->GetHandleByName("grass.png")` |
| 音 | `Resources/Sounds/` | `.wav` `.mp3` | `AudioManager::GetInstance().GetHandleByName("se.mp3")` |
| フォント | `Resources/Text/Generated/` | MSDF フォント（`.json` + `.png`） | `fontManager_->GetFont("フォント名")`（下記参照） |
| アニメーション | `AnimationsRegister.json` に登録 | `.gltf` | `animationManager_->GetNameByAnimations("登録名")`（下記参照） |

```{important}
モデル・画像・音は**ファイル名（拡張子付き）**で管理されます。フォルダが違っても、同じファイル名の素材は2つ目が読み込まれません。
ファイル名はプロジェクト全体で重複しないようにしてください。
```

## モデル

```cpp
GameEngine::Model* model = modelManager_->GetNameByModel("terrain.obj");

// モデル全体の初期設定を変える
model->SetDefaultIsEnableLight(true);              // ライティングを有効にする
model->SetDefaultColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 色
model->SetDefaultTextureHandle(textureManager_->GetHandleByName("grass.png")); // テクスチャを差し替える
```

`Model` は同じファイル名なら1つを共有しています。`SetDefault〜` で変えた設定は、そのモデルを使うすべての場所に反映されます。
オブジェクトごとに見た目を変えたいときは `ModelComponent` を使ってください（[描画する](rendering.md)）。

## 画像

画像は `uint32_t` のハンドル（番号）で扱います。スプライトやモデルのテクスチャに渡して使います。

```cpp
uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
renderQueue_->SetSkyboxTexture(skyboxGH);
```

## アニメーション

アニメーションは、フォルダに置くだけでは読み込まれません。
`Resources/Animations/AnimationsRegister.json` に登録すると、起動時に読み込まれます。

```json
{
    "Walk": { "file": "walk.gltf", "directory": "Resources/Models/Walk" },
    "PlayerWalk": { "file": "PlayerWalk.gltf", "directory": "Resources/Animations/Player/PlayerWalk" }
}
```

| 項目 | 内容 |
|---|---|
| キー（`"PlayerWalk"` など） | 登録名。コードではこの名前で取り出します |
| `file` | `.gltf` のファイル名 |
| `directory` | そのファイルがあるフォルダ（`Project/` からの相対パス） |

```{note}
`file` か `directory` が書かれていない項目は、エラーにならずに読み飛ばされます。アニメーションが見つからないときは、まずここを確認してください。
```

登録したアニメーションは、シーンの中で登録名を使って取り出し、`Animator` で再生します。

```cpp
// 1つの gltf に入っているアニメーションがすべて取れる（名前 -> データ）
walkAnimationData_ = animationManager_->GetNameByAnimations("Walk");

walkAnimator_ = std::make_unique<Animator>();
walkAnimator_->Initialize(model_, &walkAnimationData_["Armature|mixamo.com|Layer0"]);

// 毎フレーム更新する
walkAnimator_->ComputeUpdate();
```

## フォント

フォントは、`.ttf` / `.otf` を MSDF 形式の画像（`.png`）と情報ファイル（`.json`）に変換して使います。

1. フォントファイルを `Resources/Text/Source/` に置く
2. 使う文字を `Resources/Text/charset.txt` に書く（ここにない文字は表示されません）
3. `Resources/Text/GenerateFontAtlas.bat` を実行する → `Resources/Text/Generated/` に `.png` と `.json` ができる

取り出すときの名前は、拡張子を除いたファイル名です。

```cpp
const GameEngine::Font* font = fontManager_->GetFont("源直ゴシック EMG 2 - Medium");
```
