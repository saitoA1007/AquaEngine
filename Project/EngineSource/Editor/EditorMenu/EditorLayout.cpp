#include"EditorLayout.h"
#include <fstream>
#include <iostream>
#include<cassert>
#include<Windows.h>

using namespace GameEngine;

void EditorLayout::SaveLayout(const std::vector<std::unique_ptr<IEditorWindow>>& windows) const {
	json layoutJson;

	// すべてのウィンドウの状態をJSONオブジェクトにまとめる
	for (const auto& window : windows) {
		layoutJson[window->GetName()] = window->isActive;
	}

	// ファイルを書き込み用に開く
	std::ofstream file;
	// ファイルを書き込み用に開く
	file.open(kFilePath);

	// ファイルオープン失敗
	if (file.fail()) {
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "WindowLayout", 0);
		assert(0);
		return;
	}

	// ファイルにjson文字列を書き込む
	file << std::setw(4) << layoutJson << std::endl;
	// ファイルを閉じる
	file.close();
}

void EditorLayout::LoadLayout(std::vector<std::unique_ptr<IEditorWindow>>& windows) {

	// ファイルがなければスキップする
	std::filesystem::path dir(kFilePath);
	if (!std::filesystem::exists(kFilePath)) {
		return;
	}

	// 読み込み用ファイルストリーム
	std::ifstream ifs;
	// ファイルを読み込み用に開く
	ifs.open(kFilePath);

	// ファイルオープン失敗
	if (ifs.fail()) {
		std::string message = "Failed open data file for load.";
		MessageBoxA(nullptr, message.c_str(), "WindowLayout", 0);
		assert(0);
	}

	json layoutJson;
	// json文字列からjsonのデータ構造に展開
	ifs >> layoutJson;
	// ファイルを閉じる
	ifs.close();

	// 登録されているウィンドウの状態をJSONから読み込む
	for (auto& window : windows) {
		std::string name = window->GetName();

		if (layoutJson.contains(name)) {
			window->isActive = layoutJson[name].get<bool>();
		}
	}
}