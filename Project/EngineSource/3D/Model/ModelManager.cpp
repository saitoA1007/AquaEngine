#include"ModelManager.h"
#include <filesystem>
#include <iostream>
#include"LogManager.h"
using namespace GameEngine;

ModelManager::~ModelManager() {
	models_.clear();
}

void ModelManager::RegisterMode(const std::string& modelFile, const std::string& objFileName) {

	// 同名のモデルが登録されている場合は早期リターン
	auto getName = nameToHandles_.find(modelFile);
	if (getName != nameToHandles_.end()) {
		return;
	}

	// 新しいハンドルを取得
	uint32_t handle = nextHandle_++;

	// 登録データするを作成
	ModelEntryData entryData;
	entryData.name = objFileName;
	entryData.model = Model::CreateModel(objFileName, modelFile);

	// 登録する
	models_[handle] = std::move(entryData);
	nameToHandles_[modelFile] = handle;
}

void ModelManager::RegisterMode(const std::string& modelName, std::unique_ptr<Model> model) {
	// 同名のモデルが登録されている場合は早期リターン
	auto getName = nameToHandles_.find(modelName);
	if (getName != nameToHandles_.end()) {
		return;
	}

	// 新しいハンドルを取得
	uint32_t handle = nextHandle_++;

	// 登録データするを作成
	ModelEntryData entryData;
	entryData.name = modelName;
	entryData.model = std::move(model);

	// 登録する
	models_[handle] = std::move(entryData);
	nameToHandles_[modelName] = handle;
}

void ModelManager::UnregisterModel(uint32_t handle) {
	auto getModel = models_.find(handle);
	if (getModel == models_.end()) {
		return;
	}

	// 名前からマップを削除
	const std::string& name = getModel->second.name;
	nameToHandles_.erase(name);

	// 登録したモデル本体を削除
	models_.erase(getModel);
}

uint32_t ModelManager::GetHandleByName(const std::string& name) const {
	auto getHandle = nameToHandles_.find(name);
	if (getHandle == nameToHandles_.end()) {
		return 0;
	}
	return getHandle->second;
}

std::string ModelManager::GetNameByHandle(uint32_t handle) const {
	auto getName = models_.find(handle);
	if (getName == models_.end()) {
		return "";
	}
	return getName->second.name;
}

[[nodiscard]]
Model* ModelManager::GetHandleByModel(uint32_t handle) const {
	auto getModel = models_.find(handle);
	if (getModel == models_.end()) {
		return nullptr;
	}

	if (getModel->second.model) {
		return getModel->second.model.get();
	}

	return nullptr;
}

[[nodiscard]]
Model* ModelManager::GetNameByModel(const std::string& name) const {

	auto getHandle = nameToHandles_.find(name);
	if (getHandle == nameToHandles_.end()) {
		return nullptr;
	}

	auto getModel = models_.find(getHandle->second);
	if (getModel == models_.end()) {
		return nullptr;
	}

	if (getModel->second.model) {
		return getModel->second.model.get();
	}

	return nullptr;
}

void ModelManager::LoadAllModel() {
	const std::string kDirectoryPath = "Resources/Models";

	// ファイルパスがなければ終了
	if (!std::filesystem::exists(kDirectoryPath)) {
		return;
	}

	// 登録する拡張子
	const std::vector<std::string> allowedExtensions = { ".obj", ".gltf" };

	LogManager::GetInstance().Log("Start Loading All Models from: " + kDirectoryPath);

	try {
		// "Resources/Models/" の中を検索する
		for (const auto& dirEntry : std::filesystem::directory_iterator(kDirectoryPath)) {

			// フォルダでなければスキップ
			if (!dirEntry.is_directory()) {
				continue;
			}

			// フォルダの名前を取得する
			const std::string folderName = dirEntry.path().filename().string();

			// モデルファイル名
			std::string modelFileName;
			// モデルファイルの取得を判断
			bool modelFound = false;

			// フォルダの中からモデルファイルを検索する
			for (const auto& fileEntry : std::filesystem::directory_iterator(dirEntry.path())) {

				if (!fileEntry.is_regular_file()) {
					continue;
				}

				// 登録している拡張子か確認する
				std::string extension = fileEntry.path().extension().string();
				for (const auto& ext : allowedExtensions) {
					if (extension == ext) {
						// モデルのファイルを取得する
						modelFileName = fileEntry.path().filename().string();
						modelFound = true;
						break;
					}
				}

				// ファイル検索を終了する
				if (modelFound) {
					break;
				}
			}

			// モデルが存在している場合、登録する
			if (modelFound) {
				RegisterMode(folderName, modelFileName);
			}
		}
	}
	catch (std::filesystem::filesystem_error& e) {
		// ファイルシステム関連のエラー処理
		std::cerr << "Filesystem error while loading models: " << e.what() << std::endl;
	}

	LogManager::GetInstance().Log("End Loading All Models");
}