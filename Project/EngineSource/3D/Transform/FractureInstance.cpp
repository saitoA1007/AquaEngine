#include "FractureInstance.h"
#include "MyMath.h"
#include "CreateBufferResource.h"
#include "DescriptorHandle.h"
using namespace GameEngine;

FractureInstance::~FractureInstance() {

}
void FractureInstance::Initialize(const std::vector<uint32_t>& chunkIds, const PackedGeometryBuffer& geometryBuffer) {

	// チャンクの数
	numInstance_ = static_cast<uint32_t>(chunkIds.size());

	// リソース作成
	buffer_.Create(numInstance_);
	instancingData_ = buffer_.GetData();

	// CPU側のトランスフォーム配列も同じサイズに確保
	transformDatas_.resize(numInstance_);

	argumentBuffer_.Create(numInstance_);
	argumentData_ = argumentBuffer_.GetData();

	// 情報を書き込む
	for (uint32_t index = 0; index < numInstance_; ++index) {
		uint32_t chunkId = chunkIds[index];

		// PackedGeometryBufferからこのチャンクの描画範囲（オフセット等）を取得
		const GeometryRange& range = geometryBuffer.GetRange(chunkId);

		// CPU側の初期トランスフォーム設定
		transformDatas_[index].transform.scale = { 1.0f, 1.0f, 1.0f };
		transformDatas_[index].transform.rotate = { 0.0f, 0.0f, 0.0f };
		transformDatas_[index].transform.translate = { 0.0f, 0.0f, 0.0f };

		// GPU側バッファの初期化
		instancingData_[index].World = Matrix4x4::MakeIdentity();

		// PackedGeometryBufferから得た描画範囲をGPU用構造体に詰める
		instancingData_[index].vertexOffset = range.vertexOffset;
		instancingData_[index].indexOffset = range.indexOffset;
		instancingData_[index].indexCount = range.indexCount;
		instancingData_[index].chunkId = chunkId;

		// ExecuteIndirect 用のDrawコマンド引数を設定
		argumentData_[index].instanceIndex = index;
		argumentData_[index].drawArguments.IndexCountPerInstance = range.indexCount; // 破片のインデックス数
		argumentData_[index].drawArguments.InstanceCount = 1;                        // 1回につき1個描画
		argumentData_[index].drawArguments.StartIndexLocation = range.indexOffset;   // インデックスの開始位置
		argumentData_[index].drawArguments.BaseVertexLocation = static_cast<INT>(range.vertexOffset); // 頂点の開始位置
		argumentData_[index].drawArguments.StartInstanceLocation = 0;
	}
}

void FractureInstance::Update() {
	// 数によって更新を変える
	for (uint32_t i = 0; i < transformDatas_.size(); ++i) {
		instancingData_[i].World = Math::MakeAffineMatrix(transformDatas_[i].transform.scale, transformDatas_[i].transform.rotate, transformDatas_[i].transform.translate);
	}
}