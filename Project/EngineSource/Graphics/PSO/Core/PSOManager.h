#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
#include <unordered_map>
#include <string>

#include "DXC.h"

#include "ShaderCompiler.h"
#include "RootSignatureBuilder.h"
#include "InputLayoutBuilder.h"

// 本来は外から持ってくるもの達
#include "RasterizerBuilder.h"
#include "BlendBuilder.h"

#include "DescriptorCounts.h"

#include "DrawPSOData.h"

#include <json.hpp>

using json = nlohmann::json;

namespace GameEngine {

	//// 描画する際に必要なデータを送る
	//struct DrawPsoData {
	//	ID3D12RootSignature* rootSignature;
	//	ID3D12PipelineState* graphicsPipelineState;
	//};

	class PSOManager {
	public:

		// PSOを作成するためのデータ
		struct CreatePSOData {
			std::string rootSigName = "default";  // 使用するルートシグネチャの名前
			std::wstring vsPath = L"vsPath"; // 頂点シェーダーのパス
			std::wstring psPath = L"psPath"; // ピクセルシェーダーのパス
			DrawModel drawMode = DrawModel::FillFront; // 描画モード
			BlendMode blendMode = BlendMode::kBlendModeNormal; // ブレンドモード
			bool isDepthEnable = true; // 深度の使用
			D3D12_DEPTH_WRITE_MASK depthMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL; // 深度の書き込み
			D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 描画タイプ
		};

		// グラフィックスパイプラインのデータ
		struct PSOData {
			Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
			std::string rootSigName = "default"; // 関連するルートシグネチャ
		};

		// ルートシグネチャのデータ
		struct RootSignatureData {
			Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
			std::vector<ParameterType> parameterTypes; // パラメータのタイプ
		};

	public:

		// 初期化処理
		void Initialize(ID3D12Device* device, DXC* dxc);

		/// <summary>
		/// PSOを登録する(自動生成)
		/// </summary>
		/// <param name="name"></param>
		/// <param name="psoData"></param>
		void RegisterPSO(const std::string& name, const CreatePSOData& psoData);

		/// <summary>
		/// PSOを登録する(手動生成)
		/// </summary>
		/// <param name="name"></param>
		/// <param name="psoData"></param>
		/// <param name="rootSignature"></param>
		/// <param name="inputLayout"></param>
		void RegisterPSO(const std::string& name, const CreatePSOData& psoData,RootSignatureBuilder* rootSignature, InputLayoutBuilder* inputLayout);

		/// <summary>
		/// シャドウマップ専用のPSO(後で一つにまとめる)
		/// </summary>
		/// <param name="name"></param>
		/// <param name="psoData"></param>
		/// <param name="rootSignature"></param>
		/// <param name="inputLayout"></param>
		void RegisterShadowMapPSO(const std::string& name, const CreatePSOData& psoData, RootSignatureBuilder* rootSignature, InputLayoutBuilder* inputLayout);

		void LoadFromJson(const std::string& filePath);

		/// <summary>
		/// デフォルトで使用するPSOを作成
		/// </summary>
		void DefaultLoadPSO();

		/// <summary>
		/// postEffectで使用するPSOを作成
		/// </summary>
		void DeaultLoadPostEffectPSO();

	public:

		ID3D12RootSignature* GetRootSignature(const std::string& name);
		ID3D12PipelineState* GetPSO(const std::string& name);

		DrawPsoData GetDrawPsoData(const std::string& PsoName) const;

	private:

		ID3D12Device* device_ = nullptr;
		DXC* dxc_ = nullptr;

		// PSOのリスト
		std::unordered_map<std::string, PSOData> psoList_;

		// ルートシグネチャリスト
		std::unordered_map<std::string, RootSignatureData> rootSignatureList_;

		// シェーダーコンパイラ
		ShaderCompiler shaderCompiler_;

		// ラスタライザを作成
		RasterizerBuilder rasterizerBuiler_;

		// ブレンドモードの設定
		BlendBuilder blendBuilder_;

		// グローバル変数の保存先ファイルパス
		const std::string kDirectoryPath = "Resources/Json/PSO/";

	private:

		// PSOを作成する
		void CreatePSO(const std::string& psoName,const CreatePSOData& psoData);
	};
}

