#include "TextureManager.h"
#include <format>
#include <filesystem>
#include "LogManager.h"
using namespace GameEngine;

void TextureManager::Initialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
	commandList_ = commandList;
	srvManager_ = srvManager;

#ifdef USE_IMGUI
	// 最初にwhiteの画像を読み込む
	RegisterTexture("EngineSource/Resources/Textures/white2x2.png");
#endif
}

void TextureManager::Finalize() {
	// 解放処理
	textures_.clear();
}

void TextureManager::RegisterTexture(const std::string& fileName) {
	// 画像のファイル名を取得
	std::string textureName = GetFileName(fileName);

	// 登録している場合は早期リターン
	auto tex = textures_.find(textureName);
	if (tex != textures_.end()) {
		return;
	}

	// 登録する
	auto texture = std::make_unique<Texture>();
	texture->Create(fileName, commandList_);
	textures_[textureName] = std::move(texture);
}

uint32_t TextureManager::GetHandleByName(const std::string& name) const {
	auto tex = textures_.find(name);
	// 登録されていなければ0を返す
	if (tex == textures_.end()) {
		return 0;
	}
	return tex->second->GetSrvIndex();
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetTextureSrvHandlesGPU(const uint32_t& textureHandle) {
	return srvManager_->GetGPUHandle(textureHandle);
}

std::string TextureManager::GetFileName(const std::string& fullPath) {
	return std::filesystem::path(fullPath).filename().string();
}

void TextureManager::LoadAllTexture() {
	namespace fs = std::filesystem;

	// Texturesのフォルダが存在するか確認する
	if (!fs::exists(kDirectoryPath)) {
		LogManager::GetInstance().Log("Texture directory not found, skipping LoadAllTexture: " + kDirectoryPath);
		return;
	}

	LogManager::GetInstance().Log("Start Loading All Textures from: " + kDirectoryPath);

	// Texturesのフォルダになる画像ファイルとフォルダを検索
	for (const auto& entry : fs::recursive_directory_iterator(kDirectoryPath)) {

		// 現在のファイル/ディレクトリの絶対パスを取得
		fs::path currentPath = fs::absolute(entry.path());

		// 画像を登録、ロードする
		if (entry.is_regular_file()) {
			// ファイルパスを取得する
			std::string filePath = entry.path().string();
			// 登録
			RegisterTexture(filePath);
		}
	}

	LogManager::GetInstance().Log("End Loading All Textures");
}