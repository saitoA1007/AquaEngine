#include "GameParamEditor.h"
#include <cassert>
#include <algorithm>
#include "JsonSerializer.h"
using json = nlohmann::json;
using namespace GameEngine;

GameParamEditor* GameParamEditor::GetInstance() {
	static GameParamEditor instance;
	return &instance;
}

void GameParamEditor::SaveFile(const std::string& rootGroupName) {

	// グループを検索
	std::map<std::string, Group>::iterator itGroup = datas_.find(rootGroupName);
	// 未登録チェック
	assert(itGroup != datas_.end());

	json root = json::object();
	// jsonオブジェクト登録
	root[rootGroupName] = json::object();
	// シーン名を保存
	root[rootGroupName]["SceneName"] = activeSceneName_;

	// シーン名はルートグループに保存
	root[rootGroupName]["SceneName"] = activeSceneName_;

	// グループツリーをシリアライズ
	SerializeGroupToJson(root[rootGroupName], itGroup->second);

	// 各項目について
	for (auto& [itemName, item] : itGroup->second.items) {
		json& jsonNode = root[rootGroupName][itemName];
		// jsonに値を保存する
		std::visit(JsonSaveVisitor{ jsonNode }, item.value);
	}

	// 保存する
	const std::string filePath = kDirectoryPath + rootGroupName + ".json";
	JsonSerializer::SaveToFile(filePath, root);
}

void GameParamEditor::LoadFiles() {
	// ディレクトリ内の全ファイルを取得
	JsonSerializer::LoadDirectory(kDirectoryPath,
		[this](const std::string& stem, const json& root) {
			ParseGroupJson(stem, root);
		}
	);
}

void GameParamEditor::LoadFile(const std::string& rootGroupName) {
	// 読み込みJSONファイルのフルパスを合成する
	const std::string filePath = kDirectoryPath + rootGroupName + ".json";
	const json root = JsonSerializer::LoadFromFile(filePath);
	ParseGroupJson(rootGroupName, root);
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

void GameParamEditor::SetRootGroupName(const std::string& rootGroupName) {
	rootGroupName_ = rootGroupName;
}

void GameParamEditor::SetActiveScene(const std::string& sceneName) {
	activeSceneName_ = sceneName;
}

void GameParamEditor::ParseGroupJson(const std::string& rootGroupName, const json& root) {

	auto itRoot = root.find(rootGroupName);
	assert(itRoot != root.end());

	// シーン名をルートグループに設定
	if (itRoot->contains("SceneName")) {
		datas_[rootGroupName].sceneName = itRoot->at("SceneName").get<std::string>();
	}

	// グループツリーを取得
	DeserializeGroupFromJson(datas_[rootGroupName], *itRoot);
}

void GameParamEditor::SerializeGroupToJson(json& node, const Group& group) const {

	// このグループのアイテムをシリアライズ
	for (const auto& [itemName, item] : group.items) {
		json& jsonNode = node[itemName];
		std::visit(JsonSaveVisitor{ jsonNode }, item.value);
	}

	// サブグループを再帰的にシリアライズ
	for (const auto& [childName, childGroup] : group.children) {
		node[childName] = json::object();
		SerializeGroupToJson(node[childName], childGroup);
	}
}

void GameParamEditor::DeserializeGroupFromJson(Group& group, const json& node) {

	for (auto itItem = node.begin(); itItem != node.end(); ++itItem) {
		const std::string& itemName = itItem.key();

		// SceneName はスキップ
		if (itemName == "SceneName") { continue; }

		switch (itItem->type()) {
		case json::value_t::boolean:
			group.items[itemName].value = itItem->get<bool>();
			break;
		case json::value_t::number_integer:
			group.items[itemName].value = itItem->get<int32_t>();
			break;
		case json::value_t::number_unsigned:
			group.items[itemName].value = itItem->get<uint32_t>();
			break;
		case json::value_t::number_float:
			group.items[itemName].value = itItem->get<float>();
			break;
		case json::value_t::array:
			if (itItem->size() == 2) {
				group.items[itemName].value = Vector2{ itItem->at(0), itItem->at(1) };
			} else if (itItem->size() == 3) {
				group.items[itemName].value = Vector3{ itItem->at(0), itItem->at(1), itItem->at(2) };
			} else if (itItem->size() == 4) {
				group.items[itemName].value = Vector4{ itItem->at(0), itItem->at(1), itItem->at(2), itItem->at(3) };
			}
			break;
		case json::value_t::object:
			if (itItem->contains("Min") && itItem->contains("Max")) {
				const auto& minArray = itItem->at("Min");
				const auto& maxArray = itItem->at("Max");
				if (minArray.size() == 3) {
					group.items[itemName].value = Range3{
						{ minArray[0], minArray[1], minArray[2] },
						{ maxArray[0], maxArray[1], maxArray[2] }
					};
				} else if (minArray.size() == 4) {
					group.items[itemName].value = Range4{
						{ minArray[0], minArray[1], minArray[2], minArray[3] },
						{ maxArray[0], maxArray[1], maxArray[2], maxArray[3] }
					};
				}
			} else {
				// それ以外のオブジェクトはサブグループとして再帰的に読み込む
				DeserializeGroupFromJson(group.children[itemName], *itItem);
			}
			break;
		case json::value_t::string:
			group.items[itemName].value = itItem->get<std::string>();
			break;
		default:
			break;
		}
	}
}