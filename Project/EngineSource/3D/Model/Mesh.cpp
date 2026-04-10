#include"Mesh.h"
#include"CreateBufferResource.h"

#include<cassert>
#include<numbers>

using namespace GameEngine;

Mesh::~Mesh() {
	// マッピングを解除する
	if (vertexData_) {
		vertexResource_->Unmap(0, nullptr);
		vertexData_ = nullptr;
	}
}

void Mesh::CreateTrianglePlaneMesh(ID3D12Device* device) {
	// 頂点数とインデックス数を計算
	totalVertices_ = 3;
	totalIndices_ = 0;

	// 頂点バッファを作成
	// vertexResourceを作成
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * totalVertices_);
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * totalVertices_;
	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// 頂点データを生成
	// 書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	// 左下
	vertexData_[0].position = { -0.5f,-0.5f,0.0f,1.0f };
	vertexData_[0].position.w = 1.0f;
	vertexData_[0].texcoord = { 0.0f,1.0f };
	vertexData_[0].normal = { vertexData_[0].position.x, vertexData_[0].position.y, vertexData_[0].position.z };
	// 上
	vertexData_[1].position = { 0.0f,0.5f,0.0f,1.0f };
	vertexData_[1].position.w = 1.0f;
	vertexData_[1].texcoord = { 0.5f,0.0f };
	vertexData_[1].normal = { vertexData_[1].position.x, vertexData_[1].position.y, vertexData_[1].position.z };
	// 右下
	vertexData_[2].position = { 0.5f,-0.5f,0.0f,1.0f };
	vertexData_[2].position.w = 1.0f;
	vertexData_[2].texcoord = { 1.0f,1.0f };
	vertexData_[2].normal = { vertexData_[2].position.x, vertexData_[2].position.y, vertexData_[2].position.z };
}

void Mesh::CreateGridPlaneMesh(ID3D12Device* device, const Vector2& size) {
	// 頂点数とインデックス数を計算
	totalVertices_ = 4;
	totalIndices_ = 6;

	// 頂点バッファを作成
	// vertexResourceを作成
	vertexResource_ = CreateBufferResource(device, sizeof(GridVertexData) * totalVertices_);
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(GridVertexData) * totalVertices_;
	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(GridVertexData);

	// 頂点データを生成
	// 頂点リソースにデータを書き込む
	GridVertexData* vertexData = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	float left = -size.x / 2.0f;
	float right = size.x / 2.0f;
	float top = size.y / 2.0f;
	float bottom = -size.y / 2.0f;

	// 左上
	vertexData[0].position = { left,0.0f,top,1.0f }; // 左下
	// 右上
	vertexData[1].position = { right,0.0f,top,1.0f }; // 左上
	// 左下
	vertexData[2].position = { left,0.0f,bottom,1.0f }; // 右下
	// 右下
	vertexData[3].position = { right,0.0f,bottom,1.0f }; // 左上

	// UnMapする
	vertexResource_->Unmap(0, nullptr);
	vertexData = nullptr;

	// インデックスバッファを作成
	// 球用の頂点インデックスのリソースを作る
	indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * totalIndices_);
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * totalIndices_;
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータを生成
	// インデックスリソースにデータを書き込む
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	// 三角形
	indexData[0] = 0;  indexData[1] = 1;  indexData[2] = 2;
	// 三角形2
	indexData[3] = 1;  indexData[4] = 3;  indexData[5] = 2;

	// UnMapする
	indexResource_->Unmap(0, nullptr);
	indexData = nullptr;
}

void Mesh::CreatePlaneMesh(ID3D12Device* device, const Vector2& size) {
	// 頂点数とインデックス数を計算
	totalVertices_ = 4;
	totalIndices_ = 6;

	// 頂点バッファを作成
	// vertexResourceを作成
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * totalVertices_);
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * totalVertices_;
	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// 頂点データを生成
	// 書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	float left = -size.x / 2.0f;
	float right = size.x / 2.0f;
	float top = size.y / 2.0f;
	float bottom = -size.y / 2.0f;

	// 左上
	vertexData_[0].position = { left,top,0.0f,1.0f };
	vertexData_[0].texcoord = { 0.0f,0.0f };
	vertexData_[0].normal = { 0.0f,0.0f,-1.0f };
	// 右上
	vertexData_[1].position = { right,top,0.0f,1.0f };
	vertexData_[1].texcoord = { 0.0f,0.0f };
	vertexData_[1].normal = { 0.0f,0.0f,-1.0f };
	// 左下
	vertexData_[2].position = { left,bottom,0.0f,1.0f };
	vertexData_[2].texcoord = { 0.0f,0.0f };
	vertexData_[2].normal = { 0.0f,0.0f,-1.0f };
	// 右下
	vertexData_[3].position = { right,bottom,0.0f,1.0f };
	vertexData_[3].texcoord = { 0.0f,0.0f };
	vertexData_[3].normal = { 0.0f,0.0f,-1.0f };

	// インデックスバッファを作成
	// 球用の頂点インデックスのリソースを作る
	indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * totalIndices_);
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * totalIndices_;
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータを生成
	// インデックスリソースにデータを書き込む
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	// 三角形
	indexData[0] = 0;  indexData[1] = 1;  indexData[2] = 2;
	// 三角形2
	indexData[3] = 1;  indexData[4] = 3;  indexData[5] = 2;

	// UnMapする
	indexResource_->Unmap(0, nullptr);
	indexData = nullptr;
}

void Mesh::CreateSphereMesh(ID3D12Device* device, uint32_t subdivision) {
	// 頂点数とインデックス数を計算
	totalVertices_ = (subdivision + 1) * (subdivision + 1);
	totalIndices_ = subdivision * subdivision * 6;

	// 頂点バッファを作成
	// vertexResourceを作成
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * totalVertices_);
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * totalVertices_;
	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// 頂点データを生成
	// 書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	// 緯度分割1つ分の角度
	const float kLatEvery = std::numbers::pi_v<float> / static_cast<float>(subdivision);
	// 経度分割1つ分の角度
	const float kLonEvery = 2.0f * std::numbers::pi_v<float> / static_cast<float>(subdivision);
	for (uint32_t latIndex = 0; latIndex <= subdivision; ++latIndex) {
		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;
		float v = 1.0f - static_cast<float>(latIndex) / static_cast<float>(subdivision);
		for (uint32_t lonIndex = 0; lonIndex <= subdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery;
			float u = static_cast<float>(lonIndex) / static_cast<float>(subdivision);
			uint32_t start = latIndex * (subdivision + 1) + lonIndex;

			vertexData_[start].position.x = cos(lat) * cos(lon);
			vertexData_[start].position.y = sin(lat);
			vertexData_[start].position.z = cos(lat) * sin(lon);
			vertexData_[start].position.w = 1.0f;
			vertexData_[start].texcoord = { u, v };
			vertexData_[start].normal = { vertexData_[start].position.x, vertexData_[start].position.y, vertexData_[start].position.z };
		}
	}

	// インデックスバッファを作成
	// 球用の頂点インデックスのリソースを作る
	indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * totalIndices_);
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * totalIndices_;
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータを生成
	// インデックスリソースにデータを書き込む
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	uint32_t index = 0;
	for (uint32_t latIndex = 0; latIndex < subdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < subdivision; ++lonIndex) {
			uint32_t a = (latIndex * (subdivision + 1)) + lonIndex;
			uint32_t b = a + subdivision + 1;
			uint32_t c = a + 1;
			uint32_t d = b + 1;

			// 三角形1
			indexData[index] = a;
			index++;
			indexData[index] = b;
			index++;
			indexData[index] = d;
			index++;

			// 三角形2
			indexData[index] = a;
			index++;
			indexData[index] = d;
			index++;
			indexData[index] = c;
			index++;
		}
	}

	// UnMapする
	indexResource_->Unmap(0, nullptr);
	indexData = nullptr;
}

void Mesh::CreateModelMesh(ID3D12Device* device,ModelData modelData, const uint32_t& index) {

	// 要素が無ければエラー
	assert(modelData.meshes.size() > index);

	// メッシュに対応するマテリアル
	materialName_ = modelData.meshes[index].materialName;

	// 描画する時に利用する頂点数
	totalVertices_ = static_cast<UINT>(modelData.meshes[index].vertices.size());
	totalIndices_ = static_cast<UINT>(modelData.meshes[index].indices.size());

	// 頂点リソースを作る
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * modelData.meshes[index].vertices.size());
	// 頂点バッファビューを作成する
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();// リソースの先頭のアドレスから使う
	vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData.meshes[index].vertices.size());// 使用するリソースのサイズは頂点サイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);// 1頂点あたりのサイズ

	// 頂点リソースにデータを書き込む
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));// 書き込むためのアドレスを取得
	std::memcpy(vertexData_, modelData.meshes[index].vertices.data(), sizeof(VertexData) * modelData.meshes[index].vertices.size());// 頂点データをリソースにコピー

	// インデックスバッファを作成
	// 球用の頂点インデックスのリソースを作る
	indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * totalIndices_);
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * totalIndices_;
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータを生成
	// インデックスリソースにデータを書き込む
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData, modelData.meshes[index].indices.data(), sizeof(uint32_t) * modelData.meshes[index].indices.size());

	// UnMapする
	indexResource_->Unmap(0, nullptr);
	indexData = nullptr;
}

void Mesh::CreateRingMesh(ID3D12Device* device,const uint32_t& subdivision,const float& outerRadius,const float& innerRadius) {
	// 頂点数とインデックス数を計算
	totalVertices_ = subdivision * 2;
	totalIndices_ = subdivision * 6;

	// 頂点バッファを作成
	// vertexResourceを作成
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * totalVertices_);
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * totalVertices_;
	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// 頂点データを生成
	// 書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / static_cast<float>(subdivision);

	for (uint32_t index = 0; index < subdivision; ++index) {
		float angle = index * radianPerDivide;
		float sin = std::sinf(angle);
		float cos = std::cosf(angle);
		float u = static_cast<float>(index) / static_cast<float>(subdivision);

		 // 外周の頂点
		vertexData_[index * 2 + 0].position = { cos * outerRadius, 0.0f, sin * outerRadius };
		vertexData_[index * 2 + 0].texcoord = { u, 0.0f };
		vertexData_[index * 2 + 0].normal = {0.0f,0.0f,1.0f};
		// 内周の頂点
		vertexData_[index * 2 + 1].position = { cos * innerRadius, 0.0f, sin * innerRadius };
		vertexData_[index * 2 + 1].texcoord = { u, 1.0f };
		vertexData_[index * 2 + 1].normal = { 0.0f,0.0f,1.0f };
	}

	// インデックスバッファを作成
	// 球用の頂点インデックスのリソースを作る
	indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * totalIndices_);
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * totalIndices_;
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータを生成
	// インデックスリソースにデータを書き込む
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	for (uint32_t index = 0; index < subdivision; ++index) {
		uint32_t p0 = (index * 2 + 0) % (subdivision * 2);
		uint32_t p1 = (index * 2 + 1) % (subdivision * 2);
		uint32_t p2 = (index * 2 + 2) % (subdivision * 2);
		uint32_t p3 = (index * 2 + 3) % (subdivision * 2);

		// 三角形1
		indexData[index * 6 + 0] = p0;
		indexData[index * 6 + 1] = p1;
		indexData[index * 6 + 2] = p2;
		// 三角形2
		indexData[index * 6 + 3] = p2;
		indexData[index * 6 + 4] = p1;
		indexData[index * 6 + 5] = p3;
	}

	// UnMapする
	indexResource_->Unmap(0, nullptr);
	indexData = nullptr;
}

void Mesh::CreateCylinderMesh(ID3D12Device* device, const uint32_t& subdivision, const float& topRadius, const float& bottomRadius, const float& height) {
	// 頂点数とインデックス数を計算
	totalVertices_ = subdivision * 2;
	totalIndices_ = subdivision * 6;

	// 頂点バッファを作成
	// vertexResourceを作成
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * totalVertices_);
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * totalVertices_;
	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// 頂点データを生成
	// 書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / static_cast<float>(subdivision);
	const float halfHeight = height * 0.5f;

	for (uint32_t index = 0; index < subdivision; ++index) {
		float angle = index * radianPerDivide;
		float sin = std::sinf(angle);
		float cos = std::cosf(angle);
		float u = static_cast<float>(index) / static_cast<float>(subdivision);

		// 上面の頂点
		vertexData_[index * 2 + 0].position = { cos * topRadius, +halfHeight, sin * topRadius };
		vertexData_[index * 2 + 0].texcoord = { u, 0.0f };
		vertexData_[index * 2 + 0].normal = {cos,0.0f,sin};
		// 下面の頂点
		vertexData_[index * 2 + 1].position = { cos * bottomRadius, -halfHeight, sin * bottomRadius };
		vertexData_[index * 2 + 1].texcoord = { u, 1.0f };
		vertexData_[index * 2 + 1].normal = {cos,0.0f,sin};
	}

	// インデックスバッファを作成
	// 球用の頂点インデックスのリソースを作る
	indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * totalIndices_);
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * totalIndices_;
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータを生成
	// インデックスリソースにデータを書き込む
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	for (uint32_t index = 0; index < subdivision; ++index) {
		uint32_t p0 = (index * 2 + 0) % (subdivision * 2);
		uint32_t p1 = (index * 2 + 1) % (subdivision * 2);
		uint32_t p2 = (index * 2 + 2) % (subdivision * 2);
		uint32_t p3 = (index * 2 + 3) % (subdivision * 2);

		// 三角形1
		indexData[index * 6 + 0] = p0;
		indexData[index * 6 + 1] = p1;
		indexData[index * 6 + 2] = p2;
		// 三角形2
		indexData[index * 6 + 3] = p2;
		indexData[index * 6 + 4] = p1;
		indexData[index * 6 + 5] = p3;
	}

	// UnMapする
	indexResource_->Unmap(0, nullptr);
	indexData = nullptr;
}