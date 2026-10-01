# AquaEngine ドキュメント設定
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path

DOCS_DIR = Path(__file__).parent.resolve()
ENGINE_SOURCE_DIR = DOCS_DIR.parent / "Project" / "EngineSource"
API_DIR = DOCS_DIR / "api"

# -- プロジェクト情報 ---------------------------------------------------------
project = "AquaEngine"
author = "saitoA1007"
language = "ja"

# -- 拡張機能 ------------------------------------------------------------------
extensions = [
    "breathe",
    "myst_parser",
]

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

exclude_patterns = ["_build", "_doxygen"]

# -- Breathe -------------------------------------------------------------------
breathe_projects = {"AquaEngine": str(DOCS_DIR / "_doxygen" / "xml")}
breathe_default_project = "AquaEngine"
breathe_default_members = ("members", "undoc-members")

# -- HTML出力 ------------------------------------------------------------------
html_theme = "furo"
html_title = "AquaEngine"


def run_doxygen():
    """ビルドのたびにDoxygenでXMLを再生成する"""
    (DOCS_DIR / "_doxygen").mkdir(exist_ok=True)
    subprocess.run(["doxygen", "Doxyfile"], cwd=DOCS_DIR, check=True)


# ファイル単位で出力するメンバーの種類と、対応する Breathe ディレクティブ
_MEMBER_DIRECTIVES = {
    "enum": "doxygenenum",
    "typedef": "doxygentypedef",
    "function": "doxygenfunction",
    "variable": "doxygenvariable",
}


def _collect_file_entities(xml_dir):
    """ファイル名 -> そのファイルで定義されているエンティティのディレクティブ一覧"""
    index = ET.parse(xml_dir / "index.xml").getroot()
    compound_kinds = {c.get("refid"): c.get("kind") for c in index.findall("compound")}

    result = {}
    for compound in index.findall("compound[@kind='file']"):
        root = ET.parse(xml_dir / f"{compound.get('refid')}.xml").getroot()
        cdef = root.find("compounddef")
        entries = []

        # クラス / 構造体(ネストされたクラスは外側のクラスと一緒に出力されるので除外)
        inner_names = [ic.text for ic in cdef.findall("innerclass")]
        for ic in cdef.findall("innerclass"):
            name = ic.text
            outer = name.rsplit("::", 1)[0] if "::" in name else None
            if outer in inner_names:
                continue
            kind = compound_kinds.get(ic.get("refid"), "class")
            directive = {"struct": "doxygenstruct", "union": "doxygenunion"}.get(kind, "doxygenclass")
            entries.append(f".. {directive}:: {name}")

        # 名前空間スコープの列挙型・関数など
        seen = set()
        for member in cdef.findall("sectiondef/memberdef"):
            directive = _MEMBER_DIRECTIVES.get(member.get("kind"))
            if directive is None:
                continue
            qualified = member.findtext("qualifiedname") or member.findtext("name")
            if directive == "doxygenfunction":
                # オーバーロードを区別するため引数の型を指定する(デフォルト引数は照合を妨げるので含めない)
                params = ["".join(p.find("type").itertext()) for p in member.findall("param")]
                params = [t for t in params if t != "void"]
                qualified += "(" + ", ".join(params) + ")"
            if qualified in seen:
                continue
            seen.add(qualified)
            entries.append(f".. {directive}:: {qualified}")

        result[compound.findtext("name")] = entries
    return result


def generate_api_pages():
    """EngineSource のフォルダごとにAPIページを自動生成する"""
    API_DIR.mkdir(exist_ok=True)
    entities = _collect_file_entities(DOCS_DIR / "_doxygen" / "xml")

    categories = []
    for category_dir in sorted(p for p in ENGINE_SOURCE_DIR.iterdir() if p.is_dir()):
        headers = sorted(category_dir.rglob("*.h"))
        if not headers:
            continue
        category = category_dir.name
        categories.append(category)

        lines = [category, "=" * len(category), ""]
        for header in headers:
            # Doxygen のXMLはファイル名で管理されるため、ヘッダー名はエンジン内で一意にしておくこと
            directives = entities.get(header.name)
            if not directives:
                continue
            rel = header.relative_to(ENGINE_SOURCE_DIR).as_posix()
            lines += [header.name, "-" * len(header.name), "", f"``{rel}``", ""]
            for d in directives:
                lines += [d, ""]
        _write_if_changed(API_DIR / f"{category}.rst", "\n".join(lines))

    index_lines = [
        "APIリファレンス",
        "===============",
        "",
        ".. toctree::",
        "   :maxdepth: 1",
        "",
        *[f"   {c}" for c in categories],
        "",
    ]
    _write_if_changed(API_DIR / "index.rst", "\n".join(index_lines))


def _write_if_changed(path, text):
    # 内容が変わらない場合は書き込まない(sphinx-autobuild の無限ループ防止)
    if path.exists() and path.read_text(encoding="utf-8") == text:
        return
    path.write_text(text, encoding="utf-8")


run_doxygen()
generate_api_pages()
