#include "pch.h"
#include "TextureManager.h"
#include <format>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include "LogManager.h"
using namespace GameEngine;

bool TextureManager::IsDataTexture(const std::string& filePath) {
	std::string stem = std::filesystem::path(filePath).stem().string();
	std::transform(stem.begin(), stem.end(), stem.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	static constexpr const char* kKeywords[] = {
		"_bump", "_normal", "_nrm", "_ddn", "_spec", "_rough",
		"_metal", "_ao", "_mask", "_height", "_disp",
	};
	for (const char* keyword : kKeywords) {
		if (stem.find(keyword) != std::string::npos) {
			return true;
		}
	}
	return false;
}

bool TextureManager::IsImageFile(const std::filesystem::path& path) {
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".dds";
}

void TextureManager::Initialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
	commandList_ = commandList;
	srvManager_ = srvManager;

#ifdef USE_IMGUI
	// 最初にwhiteの画像を読み込む
	RegisterTexture("EngineSource/Resources/Textures/white2x2.png");
#else
	// 最初にwhiteの画像を読み込む
	RegisterTexture("Resources/Textures/white2x2.png");
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
	// 法線マップなどのデータテクスチャはsRGB変換をかけずに読み込む
	texture->Create(fileName, commandList_, !IsDataTexture(fileName));
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
	for (auto it = fs::recursive_directory_iterator(kDirectoryPath); it != fs::recursive_directory_iterator(); ++it) {
		// Cacheフォルダは対象外
		if (it->is_directory() && it->path().filename() == "Cache") {
			it.disable_recursion_pending();
			continue;
		}
		// 画像を登録、ロードする
		if (it->is_regular_file() && IsImageFile(it->path())) {
			// 登録
			RegisterTexture(it->path().string());
		}
	}

	LogManager::GetInstance().Log("End Loading All Textures");
}

void TextureManager::LoadDebugAllTexture() {
	namespace fs = std::filesystem;

	// Texturesのフォルダが存在するか確認する
	if (!fs::exists(kDebugDirectoryPath)) {
		LogManager::GetInstance().Log("Texture directory not found, skipping LoadAllTexture: " + kDebugDirectoryPath);
		return;
	}

	LogManager::GetInstance().Log("Start Loading All Textures from: " + kDebugDirectoryPath);

	// Texturesのフォルダになる画像ファイルとフォルダを検索
	for (auto it = fs::recursive_directory_iterator(kDebugDirectoryPath); it != fs::recursive_directory_iterator(); ++it) {
		// Cacheフォルダは対象外
		if (it->is_directory() && it->path().filename() == "Cache") {
			it.disable_recursion_pending();
			continue;
		}
		// 画像を登録、ロードする
		if (it->is_regular_file() && IsImageFile(it->path())) {
			// 登録
			RegisterTexture(it->path().string());
		}
	}

	LogManager::GetInstance().Log("End Loading All Textures");
}

std::vector<std::string> TextureManager::GetRegisteredTextureNames() const {
	std::vector<std::string> names;
	names.reserve(textures_.size());
	for (const auto& [name, texture] : textures_) {
		names.push_back(name);
	}
	return names;
}
