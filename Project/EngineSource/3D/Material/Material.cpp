#include "Material.h"
#include "MyMath.h"
#include "CreateBufferResource.h"
using namespace GameEngine;

Material::~Material() {
	
}

void Material::Initialize(const Vector4& color, const Vector3& specularColor,const float& shininess,const bool& isEnableLighting) {
	// 定数バッファの作成
	constBuffer_.Create();
	materialData_ = constBuffer_.GetData();

	// 白色に設定
	materialData_->color = color;
	// Lightingするのでtrueに設定する
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