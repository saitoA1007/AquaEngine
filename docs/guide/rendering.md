# 描画する

AquaEngine では、`Draw()` の中で直接描画するのではなく、**`renderQueue_` に「これを描いて」と登録**します。
登録されたものは、そのフレームの最後にエンジンがまとめて描画します。

`renderQueue_` は、シーン（`IScene`）とゲームオブジェクト（`IGameObject`）のどちらからでも使えます。

## カメラ

3D のものを描くには、まずカメラを用意して、シーンの `Draw()` で `SetCamera()` を呼びます。

```cpp
// 初期化（大きさ・回転・位置、画面の幅、画面の高さ）
mainCamera_ = std::make_unique<Camera>();
mainCamera_->Initialize({ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,1.0f,-15.0f} }, 1280, 720);

// 毎フレーム（Update）
mainCamera_->transform_.translate.z += 0.1f;
mainCamera_->Update();

// 描画（Draw）
renderQueue_->SetCamera(mainCamera_.get());
```

## 3D モデル

### 位置・回転・大きさ（WorldTransform）

モデルをどこにどう置くかは `WorldTransform` で決めます。
`transform_` の値を変えたら、`UpdateTransformMatrix()` を呼んで反映します。

```cpp
GameEngine::WorldTransform world_;

// 大きさ・回転・位置
world_.Initialize({ {2.0f,2.0f,2.0f}, {0.0f,0.0f,0.0f}, {2.0f,-1.0f,0.0f} });

world_.transform_.translate.x += 0.1f;
world_.UpdateTransformMatrix();
```

`SetParent()` で親を設定すると、親に付いて動くようになります。

### ModelComponent を使う（おすすめ）

`ModelComponent` は、モデル・`WorldTransform`・マテリアルをまとめたクラスです。
オブジェクトごとに色などを変えられるので、ゲームオブジェクトにはこちらを使うのがおすすめです。

```cpp
GameEngine::ModelComponent modelComponent_{ model };

// Update
modelComponent_.worldTransform_.transform_.translate = position_;
modelComponent_.materialData_->color = { 1.0f, 0.0f, 0.0f, 1.0f }; // このオブジェクトだけ赤くする
modelComponent_.Update();

// Draw
modelComponent_.DrawRaytracing(renderQueue_); // レイトレーシングで描画
modelComponent_.Draw(renderQueue_);           // ラスタライズで描画
```

### renderQueue_ に直接登録する

`Model` と `WorldTransform` を直接渡すこともできます。

```cpp
renderQueue_->SubmitRaytracingModel(terrainModel_, terrainWorld_); // レイトレーシング
renderQueue_->SubmitModel(terrainModel_, terrainWorld_);           // ラスタライズ
```

主な登録関数は次のとおりです。

| 関数 | 用途 |
|---|---|
| `SubmitRaytracingModel` | レイトレーシングで描画するモデル |
| `SubmitModel` | 通常のモデル |
| `SubmitAddModel` | 加算合成で描画するモデル（光るエフェクトなど） |
| `SubmitAnimation` | アニメーションするモデル |
| `SubmitInstancing` | 同じモデルを大量に描画する |
| `SubmitSprite` / `SubmitText` | 2D の画像・文字 |

すべての関数は [RenderQueue の API](../api/Graphics.rst) を見てください。

## 2D の画像（Sprite）

```cpp
// 生成
sprite_ = std::make_unique<GameEngine::Sprite>();
sprite_->textureHandle_ = textureManager_->GetHandleByName("title.png");
sprite_->position_ = { 640.0f, 360.0f };  // 画面上の位置（ピクセル）
sprite_->size_ = { 400.0f, 200.0f };      // 大きさ（ピクセル）
sprite_->anchorPoint_ = { 0.5f, 0.5f };   // 中心を基準にする

// Update
sprite_->Update();

// Draw
renderQueue_->SubmitSprite(sprite_.get());
```

## 文字（Text）

```cpp
// フォントと表示する文字列を渡す（3つ目の引数は最大文字数。初期値は 16）
text_ = std::make_unique<GameEngine::Text>(fontManager_->GetFont("源直ゴシック EMG 2 - Medium"), "スコア", 32);
text_->position_ = { 100.0f, 50.0f };
text_->scale_ = { 32.0f, 32.0f }; // 文字の大きさ

// 文字列を変える
text_->SetText("スコア : 100");

// Update
text_->Update();

// Draw
renderQueue_->SubmitText(text_.get());
```

フォントの用意のしかたは [リソースの読み込み](resources.md) を見てください。

## ライト

ライトは `renderQueue_->GetLightManager()` から設定します。

```cpp
auto* light = renderQueue_->GetLightManager();
light->SetDirectionalDirction({ 0.0f, -1.0f, 0.0f });
light->SetDirectionalIntensity(0.8f);
light->SetDirectionalColor({ 1.0f, 1.0f, 1.0f, 1.0f });
```

## 背景（スカイボックス）

```cpp
renderQueue_->SetSkyboxTexture(textureManager_->GetHandleByName("grasslands_sunset_1k.dds"));
```
