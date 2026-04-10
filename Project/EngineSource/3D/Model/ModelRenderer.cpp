#include"ModelRenderer.h"
#include<cassert>

using namespace GameEngine;

ID3D12GraphicsCommandList* ModelRenderer::commandList_ = nullptr;
TextureManager* ModelRenderer::textureManager_ = nullptr;
std::unordered_map<RenderMode3D, DrawPsoData> ModelRenderer::psoList_;
Matrix4x4 ModelRenderer::vpMatrix_ = {};
ID3D12Resource* ModelRenderer::cameraResource_ = nullptr;
SrvManager* ModelRenderer::srvManager_ = nullptr;

void ModelRenderer::StaticInitialize(ID3D12GraphicsCommandList* commandList, TextureManager* textureManager, SrvManager* srvManager, PSOManager* psoManager) {
	commandList_ = commandList;
	textureManager_ = textureManager;
	srvManager_ = srvManager;

	// 通常描画のpsoデータを取得する
	psoList_[RenderMode3D::DefaultModel] = psoManager->GetDrawPsoData("Default3D");
	psoList_[RenderMode3D::DefaultModelAdd] = psoManager->GetDrawPsoData("Additive3D");
	// インスタンシング描画用のデータを取得する
	psoList_[RenderMode3D::Instancing] = psoManager->GetDrawPsoData("Instancing3D");
	psoList_[RenderMode3D::InstancingAdd] = psoManager->GetDrawPsoData("AdditiveInstancing3D");
	// グリッド描画用のデータを取得する
	psoList_[RenderMode3D::Grid] = psoManager->GetDrawPsoData("Grid");
	// アニメーション描画用のデータを取得する
	psoList_[RenderMode3D::AnimationModel] = psoManager->GetDrawPsoData("Animation");
	// スカイボックス描画用のデータを取得
	psoList_[RenderMode3D::Skybox] = psoManager->GetDrawPsoData("Skybox");
	// シャドウマップ用
	psoList_[RenderMode3D::ShadowMap] = psoManager->GetDrawPsoData("ShadowMap");
}

void ModelRenderer::PreDraw(RenderMode3D mode) {
	auto pso = psoList_.find(mode);
	if (pso == psoList_.end()) {
		assert(0);
		return;
	}

	commandList_->SetGraphicsRootSignature(pso->second.rootSignature);
	commandList_->SetPipelineState(pso->second.graphicsPipelineState);
}

void ModelRenderer::SetCamera(const Matrix4x4& vpMatrix, ID3D12Resource* cameraResource) {
	vpMatrix_ = vpMatrix;
	cameraResource_ = cameraResource;
}

void ModelRenderer::Draw(const Model* model, WorldTransform& worldTransform, const Material* material) {
	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransform.SetWVPMatrix(model->GetLocalMatrix());
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetResource()->GetGPUVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetResource()->GetGPUVirtualAddress());
		}
		commandList_->SetGraphicsRootConstantBufferView(1, worldTransform.GetResource()->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::Draw(const Model* model, WorldTransform& worldTransform, ID3D12Resource* lightGroupResource, const Material* material) {
	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransform.SetWVPMatrix(model->GetLocalMatrix());
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetResource()->GetGPUVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetResource()->GetGPUVirtualAddress());
		}
		commandList_->SetGraphicsRootConstantBufferView(1, worldTransform.GetResource()->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootConstantBufferView(4, lightGroupResource->GetGPUVirtualAddress());
		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::DrawInstancing(const Model* model, const uint32_t& numInstance, WorldTransforms& worldTransforms, const Material* material) {

	// 描画するのが0以下の場合は早期リターン
	if (numInstance <= 0) { return; }

	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransforms.SetWVPMatrix(numInstance, model->GetLocalMatrix());
	} else {
		worldTransforms.SetWVPMatrix(numInstance);
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetResource()->GetGPUVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetResource()->GetGPUVirtualAddress());
		}

		commandList_->SetGraphicsRootDescriptorTable(1, *worldTransforms.GetInstancingSrvGPU());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), numInstance, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), numInstance, 0, 0);
		}
	}
}

void ModelRenderer::DrawAnimation(const Model* model, WorldTransform& worldTransform, const Material* material) {
	
	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {

		D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
			meshes[i]->GetVertexBufferView(),
			model->skinClusterBron_->influenceBufferView
		};

		commandList_->IASetVertexBuffers(0, 2, vbvs);
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetResource()->GetGPUVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetResource()->GetGPUVirtualAddress());
		}
		commandList_->SetGraphicsRootConstantBufferView(1, worldTransform.GetResource()->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());

		commandList_->SetGraphicsRootDescriptorTable(3, model->skinClusterBron_->paletteSrvHandle.second);
		commandList_->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::DrawGrid(const Model* model, WorldTransform& worldTransform) {

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	commandList_->IASetVertexBuffers(0, 1, &meshes[0]->GetVertexBufferView());
	commandList_->IASetIndexBuffer(&meshes[0]->GetIndexBufferView());
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->SetGraphicsRootConstantBufferView(0, worldTransform.GetResource()->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, cameraResource_->GetGPUVirtualAddress());
	commandList_->DrawIndexedInstanced(meshes[0]->GetTotalIndices(), 1, 0, 0, 0);
}

void ModelRenderer::DrawSkybox(const Model* model, WorldTransform& worldTransform, const Material* material) {
	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransform.SetWVPMatrix(model->GetLocalMatrix());
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetResource()->GetGPUVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetResource()->GetGPUVirtualAddress());
		}
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(1, worldTransform.GetResource()->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::DrawShadowMap(const Model* model, WorldTransform& worldTransform) {
	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransform.SetWVPMatrix(model->GetLocalMatrix());
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList_->SetGraphicsRootConstantBufferView(0, worldTransform.GetResource()->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootConstantBufferView(1, cameraResource_->GetGPUVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::DrawLight(ID3D12Resource* lightGroupResource) {
	commandList_->SetGraphicsRootConstantBufferView(4, lightGroupResource->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(5, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
	commandList_->SetGraphicsRootDescriptorTable(6, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
}