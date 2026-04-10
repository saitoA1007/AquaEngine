#include"Sprite.h"
#include"CreateBufferResource.h"
#include"MyMath.h"
using namespace GameEngine;

ID3D12Device* Sprite::device_ = nullptr;
Matrix4x4 Sprite::orthoMatrix_;

Sprite::~Sprite() {
	// マッピングを解除する
	if (vertexData_) {
		vertexResource_->Unmap(0, nullptr);
		vertexData_ = nullptr;
	}

	if (constBufferData_) {
		constBufferResource_->Unmap(0, nullptr);
		constBufferData_ = nullptr;
	}
}

void Sprite::StaticInitialize(ID3D12Device* device, int32_t width, int32_t height) {
	device_ = device;
	orthoMatrix_ = Multiply(MakeIdentity4x4(), MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 100.0f));
}

std::unique_ptr<Sprite> Sprite::Create(const Vector2& position,const Vector2& size,const Vector2& anchorPoint,const Vector4& color,
	const Vector2& leftTop, const Vector2& textureSize, const Vector2& textureMaxSize) {

	// インスタンスを生成
	std::unique_ptr<Sprite> sprite = std::make_unique<Sprite>();

	// 座標と大きさを取得
	sprite->position_ = position;
	sprite->size_ = size;
	sprite->anchorPoint_ = anchorPoint;
	// 座標を元にワールド行列の生成
	sprite->worldMatrix_ = MakeTranslateMatrix({ position.x,position.y,0.0f });

	// テクスチャのサイズを取得
	sprite->textureLeftTop_ = leftTop;
	sprite->textureSize_ = textureSize;
	sprite->textureMaxeSize_ = textureMaxSize;

	// メッシュを作成
	sprite->CreateMesh();

	// マテリアルを作成
	sprite->color_ = color;
	sprite->CreateConstBufferData(color);

	return sprite;
}

void Sprite::Update() {

	// 位置の更新
	SetPosition(position_);

	// 大きさの更新
	SetSize(size_);

	constBufferData_->color = color_;

	// 座標を元にワールド行列の生成
	worldMatrix_ = MakeAffineMatrix(Vector3(scale_.x, scale_.y, 0.0f), Vector3(0.0f,0.0f,rotate_), Vector3(position_.x, position_.y, 0.0f));
	// 座標を適用 
	constBufferData_->WVP = Multiply(worldMatrix_, orthoMatrix_);
}

void Sprite::SetPosition(const Vector2& position) {
	position_ = position;
	// 座標を元にワールド行列の生成
	worldMatrix_ = MakeTranslateMatrix({ position.x,position.y,0.0f });
	// 座標を適用 
	constBufferData_->WVP = Multiply(worldMatrix_, orthoMatrix_);
}

void Sprite::SetSize(const Vector2& size) {
	size_ = size;
	// 画像のサイズを決める	
	float left = -anchorPoint_.x * size_.x;
	float right = (1.0f - anchorPoint_.x) * size_.x;
	float top = -anchorPoint_.y * size_.y;
	float bottom = (1.0f - anchorPoint_.y) * size_.y;

	// 頂点インデックス
	vertexData_[0].position = { left,bottom,0.0f,1.0f }; // 左下
	vertexData_[1].position = { left,top,0.0f,1.0f }; // 左上
	vertexData_[2].position = { right,bottom,0.0f,1.0f }; // 右下
	vertexData_[3].position = { right,top,0.0f,1.0f }; // 左上
}

void Sprite::SetColor(const Vector4& color) {
	color_ = color;
	// 色の設定
	constBufferData_->color = color;
}

void Sprite::SetUvMatrix(const Transform& transform) {
	// uv行列の設定
	constBufferData_->uvTransform = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
}

void Sprite::CreateMesh() {
	// Sprite用の頂点リソースを作る
	vertexResource_ = CreateBufferResource(device_, sizeof(VertexPosUv) * 4);

	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点4つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexPosUv) * 4;
	// 1頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexPosUv);
	// 書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	// 画像のサイズを決める	
	float left = -anchorPoint_.x * size_.x;
	float right = (1.0f - anchorPoint_.x) * size_.x;
	float top = -anchorPoint_.y * size_.y;
	float bottom = (1.0f - anchorPoint_.y) * size_.y;

	// 頂点インデックス
	vertexData_[0].position = { left,bottom,0.0f,1.0f }; // 左下
	vertexData_[1].position = { left,top,0.0f,1.0f }; // 左上
	vertexData_[2].position = { right,bottom,0.0f,1.0f }; // 右下
	vertexData_[3].position = { right,top,0.0f,1.0f }; // 右上

	// uv座標指定
	float leftTex = textureLeftTop_.x / textureMaxeSize_.x;
	float rightTex = (textureLeftTop_.x + textureSize_.x) / textureMaxeSize_.x;
	float topTex = textureLeftTop_.y / textureMaxeSize_.y;
	float bottomTex = (textureLeftTop_.y + textureSize_.y) / textureMaxeSize_.y;

	vertexData_[0].texcoord = { leftTex,bottomTex };
	vertexData_[1].texcoord = { leftTex,topTex };
	vertexData_[2].texcoord = { rightTex,bottomTex };
	vertexData_[3].texcoord = { rightTex,topTex };

	// Sprite用の頂点インデックスのリソースを作る
	indexResource_ = CreateBufferResource(device_, sizeof(uint32_t) * 6);
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	// インデックスリソースにデータを書き込む
	uint32_t* indexDataSprite = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	// 三角形
	indexDataSprite[0] = 0;  indexDataSprite[1] = 1;  indexDataSprite[2] = 2;
	// 三角形2
	indexDataSprite[3] = 1;  indexDataSprite[4] = 3;  indexDataSprite[5] = 2;

	// マッピングを解除する
	indexResource_->Unmap(0, nullptr);
	indexDataSprite = nullptr;
}

void Sprite::CreateConstBufferData(const Vector4& color) {
	// Sprite用の定数バッファリソースを作る
	constBufferResource_ = CreateBufferResource(device_, sizeof(ConstBufferData));
	// 書き込むためのアドレスを取得
	constBufferResource_->Map(0, nullptr, reinterpret_cast<void**>(&constBufferData_));
	// 色の設定
	constBufferData_->color = color;
	// UVTransform行列を初期化
	constBufferData_->uvTransform = MakeIdentity4x4();
	// wvp行列を初期化
	constBufferData_->WVP = Multiply(worldMatrix_, orthoMatrix_);
}