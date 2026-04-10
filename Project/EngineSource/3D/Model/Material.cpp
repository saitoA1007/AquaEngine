#include"Material.h"
#include"MyMath.h"
#include"CreateBufferResource.h"
using namespace GameEngine;

ID3D12Device* Material::device_ = nullptr;

Material::~Material() {
	// マッピングを解除する
	if (materialData_) {
		materialResource_->Unmap(0, nullptr);
		materialData_ = nullptr;
	}
}

void Material::StaticInitialize(ID3D12Device* device) {
	device_ = device;
}

void Material::Initialize(const Vector4& color, const Vector3& specularColor,const float& shininess,const bool& isEnableLighting) {
	// マテリアルリソースを作成
	// マテリアル用のリソースを作る。color1つ分のサイズを用意する
	materialResource_ = CreateBufferResource(device_, sizeof(MaterialData));
	// 書き込むためのアドレスを取得
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	// 白色に設定
	materialData_->color = color;
	// Lightingするのでtureに設定する
	materialData_->enableLighting = isEnableLighting;
	// UVTransform行列を初期化
	materialData_->uvTransform = MakeIdentity4x4();
	// specularの色を設定
	materialData_->specularColor = specularColor;
	// 輝度を設定
	materialData_->shininess = shininess;
	// テクスチャデータ
	materialData_->textureHandle = 0;
	// 環境光
	materialData_->metallic = 0.01f;
	// 影の適応
	materialData_->isActiveShadow = false;
}

void Material::SetUVTransform(Transform uvTransform) {
	materialData_->uvTransform = MakeAffineMatrix(uvTransform.scale, uvTransform.rotate, uvTransform.translate);
}