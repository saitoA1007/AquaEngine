#pragma once
#include <iostream>
#include <variant>
#include <map>
#include <string>

#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Range.h"

#include <json.hpp>
#include "MyMath.h"

class GameParamEditor final {
public:

	// 項目
	struct Item {
		std::variant<int32_t, uint32_t, float, Vector2, Vector3,Vector4,Range3, Range4, bool, std::string> value;
		int priority = INT_MAX; // 優先順位
	};

	// グループ
	struct Group {
		std::map<std::string, Item> items;
		std::map<std::string, Group> children;
		std::string sceneName; // 保存しているシーンの名前
	};

public:

	// シングルトン
	static GameParamEditor* GetInstance();

	/// <summary>
	/// ディレクトリの全ファイル読み込み
	/// </summary>
	void LoadFiles();

	/// <summary>
	/// ファイルから読み込む
	/// </summary>
	/// <param name="groupName"></param>
	void LoadFile(const std::string& rootGroupName);

	/// <summary>
	/// jsonファイルに保存する
	/// </summary>
	/// <param name="groupName"></param>
	void SaveFile(const std::string& rootGroupName);

	/// <summary>
	/// ルートを選択
	/// </summary>
	/// <param name="groupName"></param>
	void SetRootGroupName(const std::string& rootGroupName);

	/// <summary>
	/// 選択中のグループを取得
	/// </summary>
	/// <returns></returns>
	const std::string& GetRootGroupName() const { return rootGroupName_; }

	/// <summary>
	/// シーンを選択
	/// </summary>
	/// <param name="sceneName"></param>
	void SetActiveScene(const std::string& sceneName);

	/// <summary>
	/// 選択中のシーンを取得
	/// </summary>
	/// <returns></returns>
	const std::string& GetActiveScene() const { return activeSceneName_; }

	/// <summary>
	/// 全グループのデータを取得
	/// </summary>
	/// <returns></returns>
	std::map<std::string, Group>& GetAllGroups() { return datas_; }

	/// <summary>
	/// 値を登録する
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="path"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	template<typename T>
	void AddItem(const std::string& path, const std::string& key, const T& value, int priority = INT_MAX) {
		// グループの参照を取得
		Group& group = ResolveOrCreateGroup(path);

		// すでに登録されていれば優先順位だけ更新
		if (group.items.find(key) != group.items.end()) {
			group.items[key].priority = priority;
			return;
		}

		// ルートグループにシーン名を設定
		const std::string rootName = SplitPath(path)[0];
		Group& rootGroup = datas_[rootName];
		if (rootGroup.sceneName.empty()) {
			rootGroup.sceneName = activeSceneName_;
		}

		// 新しい項目のデータを設定
		Item newItem{};
		newItem.value = value;
		newItem.priority = priority;
		// 設定した項目をstd::mapに追加
		group.items[key] = newItem;
	}

	/// <summary>
	/// 登録した値を取得する
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="path"></param>
	/// <param name="key"></param>
	/// <returns></returns>
	template<typename T>
	T GetValue(const std::string& path, const std::string& key) const {
		// 指定グループが存在するかチェック
		const Group* group = FindGroup(path);
		assert(group != nullptr);
		// 指定キーが存在するかチェック
		assert(group->items.find(key) != group->items.end());

		auto itItem = group->items.find(key);
		// 型があるかを確認する
		assert(std::holds_alternative<T>(itItem->second.value));
		// グループの参照を取得
		return std::get<T>(itItem->second.value);
	}

	/// <summary>
	/// 登録した値を削除する
	/// </summary>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	void RemoveItem(const std::string& groupName, const std::string& key);

	void ParseGroupJson(const std::string& rootGroupName, const nlohmann::json& root);

private:
	GameParamEditor() = default;
	~GameParamEditor() = default;
	GameParamEditor(const GameParamEditor&) = delete;
	GameParamEditor& operator=(const GameParamEditor&) = delete;

	// 全データ
	std::map<std::string, Group> datas_;

	// 選択中のグループ名
	std::string rootGroupName_;

	// 現在アクティブなシーン名
	std::string activeSceneName_ = "None";

	// グローバル変数の保存先ファイルパス
	const std::string kDirectoryPath = "Resources/Json/GameData/";

private:

	// jsonファイルに値を保存するためのビジター
	struct JsonSaveVisitor {
		nlohmann::json& jsonData;
		explicit JsonSaveVisitor(nlohmann::json& jsonNode) : jsonData(jsonNode){}

		void operator()(const Range3& value) const {
			jsonData = nlohmann::json::object({
				{ "Min", nlohmann::json::array({ value.min.x, value.min.y, value.min.z }) },
				{ "Max", nlohmann::json::array({ value.max.x, value.max.y, value.max.z }) }
			});
		}

		void operator()(const Range4& value) const {
			jsonData = nlohmann::json::object({
				{ "Min", nlohmann::json::array({ value.min.x, value.min.y, value.min.z, value.min.w}) },
				{ "Max", nlohmann::json::array({ value.max.x, value.max.y, value.max.z, value.max.w}) }
				});
		}

		void operator()(const Vector4& value) const {
			jsonData = nlohmann::json::array({ value.x, value.y, value.z,value.w });
		}
		
		void operator()(const Vector3& value) const {
			jsonData = nlohmann::json::array({ value.x, value.y, value.z });
		}

		void operator()(const Vector2& value) const {
			jsonData = nlohmann::json::array({ value.x, value.y });
		}

		template<typename T>
		void operator()(const T& value) const {
			jsonData = value;
		}
	};

private:

	/// グループを分ける
	static std::vector<std::string> SplitPath(const std::string& path) {
		std::vector<std::string> segments;
		std::stringstream ss(path);
		std::string segment;
		while (std::getline(ss, segment, '/')) {
			if (!segment.empty()) {
				segments.push_back(segment);
			}
		}
		return segments;
	}

	/// パスをたどってGroupを取得する。存在しなければ作成する
	Group& ResolveOrCreateGroup(const std::string& path) {
		auto segments = SplitPath(path);
		Group* current = &datas_[segments[0]];
		for (size_t i = 1; i < segments.size(); ++i) {
			current = &current->children[segments[i]];
		}
		return *current;
	}

	/// パスをたどってGroupを検索する。存在しなければnullptrを返す
	const Group* FindGroup(const std::string& path) const {
		auto segments = SplitPath(path);
		auto it = datas_.find(segments[0]);
		if (it == datas_.end()) return nullptr;

		const Group* current = &it->second;
		for (size_t i = 1; i < segments.size(); ++i) {
			auto itChild = current->children.find(segments[i]);
			if (itChild == current->children.end()) return nullptr;
			current = &itChild->second;
		}
		return current;
	}

	/// GroupをJSON化する
	void SerializeGroupToJson(nlohmann::json& node, const Group& group) const;

	/// JSONからGroupを復元する
	void DeserializeGroupFromJson(Group& group, const nlohmann::json& node);

	/// <summary>
	/// 外部ファイルから値を取得する
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="groupName"></param>
	/// <param name="key"></param>
	/// <param name="value"></param>
	template<typename T>
	void SetValue(const std::string& groupName, const std::string& key, T value) {
		// グループの参照を取得
		Group& group = ResolveOrCreateGroup(groupName);
		// 新しい項目のデータを設定
		Item newItem{};
		newItem.value = value;
		// 設定した項目をstd::mapに追加
		group.items[key] = newItem;
	}
};