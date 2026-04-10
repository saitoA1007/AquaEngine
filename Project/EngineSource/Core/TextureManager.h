#pragma once
#include <d3d12.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <wrl.h>
#include <format>
#include "Externals/DirectXTex/DirectXTex.h"
#include "Externals/DirectXTex/d3dx12.h"

#include "SrvManager.h"

namespace GameEngine {

	class TextureManager final {
	public:
		// テクスチャ
		struct Texture {
			// テクスチャリソース
			Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
			// テクスチャアップロードするための変数
			Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResources_;

			DirectX::ScratchImage mipImage{};
			// シェーダリソースビューのハンドル(CPU)
			CD3DX12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU;
			// シェーダリソースビューのハンドル(CPU)
			CD3DX12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU;
			// 名前
			std::string fileName;
			// インデックス
			uint32_t srvIndex;
		};

	public:
		TextureManager() = default;
		~TextureManager() = default;

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="commandList">コマンドリスト</param>
		/// <param name="srvManager">srvの管理クラス</param>
		void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, SrvManager* srvManager);

		/// <summary>
		/// 解放処理
		/// </summary>
		void Finalize();

		/// <summary>
		/// 登録する関数
		/// </summary>
		/// <param name="registerName">登録名</param>
		/// <param name="fileName">読み込む画像のファイルパス</param>
		void RegisterTexture(const std::string& fileName);

	public:

		/// <summary>
		/// 全てのテクスチャデータを読み込む
		/// </summary>
		void LoadAllTexture();

		/// <summary>
		/// 名前からハンドルを取得
		/// </summary>
		/// <param name="name">登録名</param>
		/// <returns>ハンドル</returns>
		uint32_t GetHandleByName(const std::string& name) const;

		// GPUハンドルを取得する
		D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandlesGPU(const uint32_t& textureHandle);

	private:
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		ID3D12Device* device_ = nullptr;
		ID3D12GraphicsCommandList* commandList_ = nullptr;
		SrvManager* srvManager_ = nullptr;

		// 読み取り先のパス名
		static inline const std::string kDirectoryPath = "Resources/Textures/";

		// テクスチャ情報
		std::unordered_map<std::string, Texture> textureDatas_;

	private:

		/// <summary>
		/// 画像を読み込んで登録する処理
		/// </summary>
		/// <param name="registerName">登録名</param>
		/// <param name="fileName">読み込むファイルパス</param>
		void Load(const std::string& registerName, const std::string& fileName);

		/// <summary>
		/// ファイルから画像を読み込んでミップマップを生成する
		/// </summary>
		/// <param name="filePath">ファイルパス</param>
		/// <returns>読み込んだ画像データ</returns>
		[[nodiscard]]
		DirectX::ScratchImage LoadTexture(const std::string& filePath);

		/// <summary>
		/// メタデータにより、DirectX12のテクスチャリソースを作成する
		/// </summary>
		/// <param name="metadata">画像メタデータ</param>
		/// <returns>作成されたリソース</returns>
		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

		/// <summary>
		/// テクスチャデータをCPUメモリからGPUメモリに転送する
		/// </summary>
		/// <param name="texture">転送先のリソース</param>
		/// <param name="mipImages">転送元の画像データ</param>
		/// <returns>中間リソース</returns>
		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

		/// <summary>
		/// フルパスからファイル名を取得
		/// </summary>
		/// <param name="fullPath"></param>
		/// <returns></returns>
		std::string GetFileName(const std::string& fullPath);
	};
}
