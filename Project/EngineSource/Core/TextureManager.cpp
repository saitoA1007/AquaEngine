#include"TextureManager.h"
#include"ConvertString.h"
#include"CreateBufferResource.h"
#include"DescriptorHandle.h"
#include<format>
#include <filesystem>
#include"LogManager.h"
using namespace GameEngine;

void TextureManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
	device_ = device;
	commandList_ = commandList;
	srvManager_ = srvManager;

#ifdef USE_IMGUI
	// 最初にwhiteの画像を読み込む
	RegisterTexture("EngineSource/Resources/Textures/white2x2.png");
#endif
}

void TextureManager::Finalize() {
	// 解放処理
	for (auto& [s, tex] : textureDatas_) {
		tex.textureResource.Reset();
		tex.intermediateResources_.Reset();
	}
	textureDatas_.clear();
}

void TextureManager::RegisterTexture(const std::string& fileName) {
	// 画像のファイル名を取得
	std::string textureName = GetFileName(fileName);

	// 登録している場合は早期リターン
	auto tex = textureDatas_.find(textureName);
	if (tex != textureDatas_.end()) {
		return;
	}

	// 登録する
	Load(textureName, fileName);
}

uint32_t TextureManager::GetHandleByName(const std::string& name) const {
	auto tex = textureDatas_.find(name);
	// 登録されていなければ0を返す
	if (tex == textureDatas_.end()) {
		return 0;
	}
	return tex->second.srvIndex;
}

void TextureManager::Load(const std::string& registerName,const std::string& fileName) {

	// テクスチャーの読み込みを開始するログ
	LogManager::GetInstance().Log("Start LoadTexture : " + fileName);

	Texture texture;
	// テクスチャ名を記録
	texture.fileName = fileName;

	// テクスチャを読み込む
	texture.mipImage = LoadTexture(fileName);
	if (!texture.mipImage.GetImages()) {
		LogManager::GetInstance().Log("Failed to load texture: " + fileName);
		assert(false);
	}
	const DirectX::TexMetadata* metadata = &texture.mipImage.GetMetadata();
	// テクスチャリソースを作成
	texture.textureResource = CreateTextureResource(*metadata);
	if (!texture.textureResource) {
		LogManager::GetInstance().Log("Failed to create textureResource for: " + fileName);
		assert(false);
	}
	// テクスチャデータをアップロード
	texture.intermediateResources_ = UploadTextureData(texture.textureResource.Get(), texture.mipImage);
	if (!texture.intermediateResources_) {
		LogManager::GetInstance().Log("Failed to upload texture data for: " + fileName);
		assert(false);
	}

	// srvインデックスを取得
	uint32_t index = srvManager_->AllocateSrvIndex(SrvHeapType::Texture);
	texture.srvIndex = index;

	// metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata->format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (metadata->IsCubemap()) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;// 2Dテクスチャ
		srvDesc.Texture2D.MipLevels = UINT(metadata->mipLevels);
	}

	// SRVを作成するDescriptorHeapの場所を決める。
	texture.textureSrvHandleCPU = srvManager_->GetCPUHandle(index);
	texture.textureSrvHandleGPU = srvManager_->GetGPUHandle(index);
	LogManager::GetInstance().Log(std::format("CPU Handle: {}, GPU Handle: {}", texture.textureSrvHandleCPU.ptr, texture.textureSrvHandleGPU.ptr));
	// SRVを作成
	device_->CreateShaderResourceView(texture.textureResource.Get(), &srvDesc, texture.textureSrvHandleCPU);

	// 登録する
	textureDatas_[registerName] = std::move(texture);

	// テクスチャーの読み込みを完了するログ
	LogManager::GetInstance().Log("End LoadTexture : " + fileName + "\n");
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetTextureSrvHandlesGPU(const uint32_t& textureHandle) {
	return srvManager_->GetGPUHandle(textureHandle);
}

[[nodiscard]]
DirectX::ScratchImage TextureManager::LoadTexture(const std::string& filePath) {
	// テクスチャファイルを読み込んでプログラムを扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr;
	if (filePathW.ends_with(L".dds")) {
		// .ddsの場合
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}
	assert(SUCCEEDED(hr));

	// ミップマップの作成
	DirectX::ScratchImage mipImages{};

	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		// 圧縮フォーマットならそのまま使用する
		mipImages = std::move(image);
	} else {
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	}
	assert(SUCCEEDED(hr));

	// ミップマップ付きのデータを返す
	return mipImages;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(const DirectX::TexMetadata& metadata) {
	// metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width); // Textureの幅
	resourceDesc.Height = UINT(metadata.height); // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行 or 配列Textureの配列数 
	resourceDesc.Format = metadata.format; // TextureのFormat
	resourceDesc.SampleDesc.Count = 1; // ダンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。

	// 利用するHeapの設定。
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし。
		&resourceDesc, // Resourceの設定。
		D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
		nullptr, // Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device_, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device_, intermediateSize);
	UpdateSubresources(commandList_, texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
	// Textureへ転送後は利用出来るよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList_->ResourceBarrier(1, &barrier);
	return intermediateResource;
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