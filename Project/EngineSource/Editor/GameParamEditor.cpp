#include"GameParamEditor.h"
#include<cassert>
#include<algorithm>
#include <fstream>
#include<Windows.h>

GameParamEditor* GameParamEditor::GetInstance() {
	static GameParamEditor instance;
	return &instance;
}

void GameParamEditor::CreateGroup(const std::string& groupName, const std::string& sceneName) {
	// 指定名のオブジェクトが無ければ追加する
	datas_[groupName].sceneName = sceneName;
}

void GameParamEditor::SaveFile(const std::string& groupName) {

	// グループを検索
	std::map<std::string, Group>::iterator itGroup = datas_.find(groupName);

	// 未登録チェック
	assert(itGroup != datas_.end());

	json root;
	root = json::object();
	// jsonオブジェクト登録
	root[groupName] = json::object();

	// シーン名を保存
	root[groupName]["SceneName"] = activeSceneName_;

	// 各項目について
	for (std::map<std::string, Item>::iterator itItem = itGroup->second.items.begin(); itItem != itGroup->second.items.end(); ++itItem) {

		// 項目名を取得
		const std::string& itemName = itItem->first;
		// 項目の参照を取得
		Item& item = itItem->second;

		// jsonに値を保存する
		json & jsonNode = root[groupName][itemName];
		std::visit(JsonSaveVisitor{ jsonNode }, item.value);
	}

	// ディレクトリがなければ作成する
	std::filesystem::path dir(kDirectoryPath);
	if (!std::filesystem::exists(kDirectoryPath)) {
		std::filesystem::create_directory(kDirectoryPath);
	}

	// 書き込むJSONファイルのフルパスを合成する
	std::string filePath = kDirectoryPath + groupName + ".json";
	// 書き込み用ファイルストリーム
	std::ofstream ofs;
	// ファイルを書き込み用に開く
	ofs.open(filePath);

	// ファイルオープン失敗
	if (ofs.fail()) {
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "GameParamEditor", 0);
		assert(0);
		return;
	}

	// ファイルにjson文字列を書き込む(インデント幅4)
	ofs << std::setw(4) << root << std::endl;
	// ファイルを閉じる
	ofs.close();
}

void GameParamEditor::LoadFiles() {

	// ディレクトリがなければスキップする
	std::filesystem::path dir(kDirectoryPath);
	if (!std::filesystem::exists(kDirectoryPath)) {
		return;
	}

	std::filesystem::directory_iterator dir_it(kDirectoryPath);
	for (const std::filesystem::directory_entry& entry : dir_it) {

		// ファイルパスを取得
		const std::filesystem::path& filePath = entry.path();

		// ファイル拡張子を取得
		std::string extension = filePath.extension().string();
		// .jsonファイル以外はスキップ
		if (extension.compare(".json") != 0) {
			continue;
		}

		// ファイル読み込み
		LoadFile(filePath.stem().string());
	}

}

void GameParamEditor::LoadFile(const std::string& groupName) {

	// 読み込みJSONファイルのフルパスを合成する
	std::string filePath = kDirectoryPath + groupName + ".json";
	// 読み込み用ファイルストリーム
	std::ifstream ifs;
	// ファイルを読み込み用に開く
	ifs.open(filePath);

	// ファイルオープン失敗
	if (ifs.fail()) {
		std::string message = "Failed open data file for load.";
		MessageBoxA(nullptr, message.c_str(), "GameParamEditor", 0);
		assert(0);
	}

	
	json root;

	// json文字列からjsonのデータ構造に展開
	ifs >> root;
	// ファイルを閉じる
	ifs.close();

	// シーン名を読み込み
	std::string sceneName = root[groupName]["SceneName"].get<std::string>();
	datas_[groupName].sceneName = sceneName;

	// グループを検索
	json::iterator itGroup = root.find(groupName);
	// 未登録チェック
	assert(itGroup != root.end());

	// 各アイテムについて
	for (json::iterator itItem = itGroup->begin(); itItem != itGroup->end(); ++itItem) {
		// アイテム名を取得
		const std::string& itemName = itItem.key();
		if (itemName == "SceneName") {
			continue;
		}

		// パラメータの型を取得
		const auto itemType = itItem->type();

		switch (itemType)
		{
			// bool型を取得
		case json::value_t::boolean:
			SetValue(groupName, itemName, itItem->get<bool>());
			break;

			// int32_t型を取得
		case json::value_t::number_integer:
			SetValue(groupName, itemName, itItem->get<int32_t>());
			break;

			// uint32_t型を取得
		case json::value_t::number_unsigned:
			SetValue(groupName, itemName, itItem->get<uint32_t>());
			break;

			// float型を取得
		case json::value_t::number_float:
			SetValue(groupName, itemName, itItem->get<float>());
			break;

			// Vector型を取得
		case json::value_t::array:
			if (itItem->size() == 3) {
				Vector3 value = { itItem->at(0), itItem->at(1), itItem->at(2) };
				SetValue(groupName, itemName, value);
			} else if (itItem->size() == 2) {
				Vector2 value = { itItem->at(0), itItem->at(1) };
				SetValue(groupName, itemName, value);
			} else if (itItem->size() == 4) {
				Vector4 value = { itItem->at(0), itItem->at(1), itItem->at(2),itItem->at(3) };
				SetValue(groupName, itemName, value);
			}
			break;

		case json::value_t::object:
			if (itItem->contains("Min") && itItem->contains("Max")) {
				const auto& minArray = itItem->at("Min");
				const auto& maxArray = itItem->at("Max");

				if (minArray.is_array() && maxArray.is_array() &&
					minArray.size() == 3 && maxArray.size() == 3) {
					Range3 value = {
						{ minArray[0], minArray[1], minArray[2] },
						{ maxArray[0], maxArray[1], maxArray[2] }
					};
					SetValue(groupName, itemName, value);
				} else if (minArray.size() == 4 && maxArray.size() == 4) {
					Range4 value = {
						{ minArray[0], minArray[1], minArray[2], minArray[3] },
						{ maxArray[0], maxArray[1], maxArray[2], maxArray[3] }
					};
					SetValue(groupName, itemName, value);
				}
			}
			break;

			// std::string型を取得
		case json::value_t::string:
			SetValue(groupName, itemName, itItem->get<std::string>());
			break;

		default:
			break;
		}
	}
}

void GameParamEditor::RemoveItem(const std::string& groupName, const std::string& key) {
	// グループを検索
	auto itGroup = datas_.find(groupName);

	// グループが存在しなければ終了
	if (itGroup == datas_.end()) {
		return;
	}

	// グループ内のアイテムマップの参照を取得
	std::map<std::string, Item>& items = itGroup->second.items;

	// アイテムを検索
	auto itItem = items.find(key);

	// アイテムが存在すれば削除
	if (itItem != items.end()) {
		items.erase(itItem);
	}
}

void GameParamEditor::SelectGroup(const std::string& groupName) {
	selectedGroupName_ = groupName;
}

void GameParamEditor::SetActiveScene(const std::string& sceneName) {
	activeSceneName_ = sceneName;
}