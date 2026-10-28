# ドキュメントの更新方法

このドキュメントは Sphinx で作っています。API リファレンスは、ソースコードのコメントから Doxygen と Breathe を使って自動で作られます。

## 準備（初回だけ）

- Doxygen をインストールし、PATH に通しておく
- Python のパッケージを入れる

```bash
pip install -r docs/requirements.txt
```

## ビルド

リポジトリのルート（`AquaEngine/`）で実行します。

```bash
python -m sphinx -b html docs docs/_build/html
```

`docs/_build/html/index.html` をブラウザで開くと確認できます。
編集しながら確認したいときは、次のコマンドを起動したままにして http://127.0.0.1:8000 を開きます。

```bash
python -m sphinx_autobuild docs docs/_build/html
```

## 公開（GitHub Pages）

`master` ブランチに push すると、`.github/workflows/Docs.yml` がドキュメントをビルドして GitHub Pages に公開します。
`docs/` か `Project/EngineSource/` が変わったときだけ動きます。
GitHub の Actions タブから「Docs」を選ぶと、手動でも実行できます。

## 何をどこに書くか

| 書きたいもの | 書く場所 |
|---|---|
| 使い方の説明 | `docs/guide/` の `.md` ファイル。新しいページは `docs/index.rst` の toctree に追加する |
| クラスや関数の説明 | `EngineSource` のヘッダーに `/// <summary>` 形式のコメントを書く |
| API のカテゴリの説明文や、よく使うかどうかの分類 | `docs/conf.py` の `CATEGORY_INFO` |

```{warning}
`docs/api/` の中のファイルはビルドのたびに自動で作り直されます。手で編集しないでください。
```

```{note}
Doxygen はヘッダーをファイル名で区別するため、`EngineSource` の中で同じ名前のヘッダーを作らないでください。
```
