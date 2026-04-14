#pragma once
#include "Vector4.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Transform.h"
#include <iostream>
#include "ConstantBuffer.h"

namespace GameEngine {

	class Material {
	public:
		struct alignas(16) MaterialData {
			Vector4 color;
			int32_t enableLighting;
			float padding[3];
			Matrix4x4 uvTransform;
			Vector3 specularColor;
			float shininess;
			uint32_t textureHandle;
			float metallic;
			int32_t isActiveShadow;
			float padding2;
		};

	public:
		Material() = default;
		~Material();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="color">物体の色</param>
		/// <param name="specularColor">specularの色</param>
		/// <param name="shininess">輝度</param>
		/// <param name="isEnableLighting">ライトを有効化</param>
		void Initialize(const Vector4& color, const Vector3& specularColor, const float& shininess, const bool& isEnableLighting);

	public:

		/// <summary>
		/// 色を設定
		/// </summary>
		/// <param name="color"></param>
		void SetColor(Vector4 color) { materialData_->color = color; }

		/// <summary>
		/// specularの色を設定
		/// </summary>
		/// <param name="specularColor"></param>
		void SetSpecularColor(Vector3 specularColor) { materialData_->specularColor = specularColor; }

		/// <summary>
		/// 透明度を設定
		/// </summary>
		/// <param name="alpha"></param>
		void SetAlpha(const float& alpha) { materialData_->color.w = alpha; }

		/// <summary>
		/// 輝度を設定
		/// </summary>
		/// <param name="shininess"></param>
		void SetShiness(const float& shininess) { materialData_->shininess = shininess; }

		/// <summary>
		/// ライトを適応させるかを設定
		/// </summary>
		/// <param name="isEnableLighting"></param>
		void SetEnableLighting(bool isEnableLighting) { materialData_->enableLighting = isEnableLighting; }

		/// <summary>
		/// 影の適応
		/// </summary>
		/// <param name="isEnableLighting"></param>
		void SetEnableShadow(bool isEnableShadow) { materialData_->isActiveShadow = isEnableShadow; }

		/// <summary>
		/// UV行列を設定
		/// </summary>
		/// <param name="uvMatrix"></param>
		void SetUVMatrix(Matrix4x4 uvMatrix) { materialData_->uvTransform = uvMatrix; }

		/// <summary>
		/// uvトランスフォームを設定
		/// </summary>
		/// <param name="uvTransform"></param>
		void SetUVTransform(Transform uvTransform);

		/// <summary>
		/// 環境光の度合
		/// </summary>
		/// <param name="metallic"></param>
		void SetMetallic(const float& metallic) { materialData_->metallic = metallic; }

		void SetTextureHandle(const uint32_t& tex) { materialData_->textureHandle = tex; }

		const uint32_t& GetTextureHandle() const { return materialData_->textureHandle; }

		void SetDefaultTexture(const uint32_t& handle) { defaultTextureHandle_ = handle; }

		void AdaptDefaultTexture() { materialData_->textureHandle = defaultTextureHandle_; }

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return constBuffer_.GetGpuVirtualAddress(); }
		ConstantBuffer<MaterialData>* GetConstantBuffer() { return &constBuffer_; }
	private:

		ConstantBuffer<MaterialData> constBuffer_;

		// マテリアルにデータを書き込む
		MaterialData* materialData_ = nullptr;

		// テクスチャ情報
		uint32_t defaultTextureHandle_ = 0;
	};
}