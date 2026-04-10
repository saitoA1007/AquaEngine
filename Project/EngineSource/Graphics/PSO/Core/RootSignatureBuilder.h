#pragma once
#include <d3d12.h>
#include<dxcapi.h>
#include<vector>
#include<string>
#include <wrl.h>
#include<map>

namespace GameEngine {

	// パラメータタイプを設定した
	enum class ParameterType {
		CBV,
		SRV
	};

	// パラメータの情報を保持するデータ
	struct ResourceInfo {
		D3D12_SHADER_VISIBILITY visibility;
		uint32_t bindPoint;
		uint32_t space;
		uint32_t bindCount; // SRV用
	};

	class RootSignatureBuilder {
	public:

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="device"></param>
		void Initialize(ID3D12Device* device);

		/// <summary>
		/// cbvを追加する
		/// </summary>
		/// <param name="shaderRegister">レジスタ番号</param>
		/// <param name="visibility">使用するシェーダー</param>
		void AddCBVParameter(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility);

		/// <summary>
		/// srvを追加する
		/// </summary>
		/// <param name="shaderRegister">レジスタ番号</param>
		/// <param name="arryNum">配列の数</param>
		/// <param name="visibility">使用するシェーダー</param>
		void AddSRVDescriptorTable(uint32_t shaderRegister, uint32_t arryNum, uint32_t spaceNum, D3D12_SHADER_VISIBILITY visibility);

		/// <summary>
		/// サンプラーを追加する
		/// </summary>
		/// <param name="shaderRegister">レジスタ番号</param>
		/// <param name="filter">バイナリフィルタ</param>
		/// <param name="texAddress">範囲外の振る舞い</param>
		/// <param name="visibility">使用するシェーダー</param>
		void AddSampler(uint32_t shaderRegister, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE texAddress, D3D12_SHADER_VISIBILITY visibility, D3D12_COMPARISON_FUNC func = D3D12_COMPARISON_FUNC_NEVER);

		void CreateRootSignature();

		/// <summary>
		/// shaderReflectionを使用して自動生成
		/// </summary>
		/// <param name="vsBlob"></param>
		/// <param name="psBlob"></param>
		void CreateRootSignatureFromReflection(IDxcUtils* utils,IDxcBlob* vsBlob, IDxcBlob* psBlob);

		ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

		std::vector<ParameterType> GetParameterTypes() const { return parameterTypes_; }

	private:

		ID3D12Device* device_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

		std::vector<D3D12_ROOT_PARAMETER> rootParameters_;
		std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges_;
		std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers_;
		
		// パラメータのタイプ
		std::vector<ParameterType> parameterTypes_;

	private:

		/// <summary>
		/// バインドされたリソースを解析する
		/// </summary>
		void ReflectionBoundResource(IDxcUtils* utils, DxcBuffer reflectionBuffer, IDxcBlob* shaderBlob,D3D12_SHADER_VISIBILITY visibility);

		void ReflectionBoundResourceToMap(IDxcUtils* utils, DxcBuffer reflectionBuffer, IDxcBlob* shaderBlob, D3D12_SHADER_VISIBILITY visibility,
			std::map<uint32_t, ResourceInfo>& cbvMap, std::map<uint32_t, ResourceInfo>& srvMap, std::map<uint32_t, ResourceInfo>& samplerMap);

		void CreateParametersFromMaps(const std::map<uint32_t, ResourceInfo>& cbvMap, const std::map<uint32_t, ResourceInfo>& srvMap, const std::map<uint32_t, ResourceInfo>& samplerMap);

		D3D12_SHADER_VISIBILITY MergeVisibility(D3D12_SHADER_VISIBILITY v1, D3D12_SHADER_VISIBILITY v2);
	};
}