#pragma once
#include <d3d12.h>
#include<vector>
#include <unordered_map>
#include <wrl.h>

#include"TextureManager.h"
#include"SrvManager.h"

#include"PSO/Core/PSOManager.h"
#include"PSO/Core/DrawPSOData.h"

#include "Model.h"
#include "WorldTransform.h"
#include "WorldTransforms.h"

namespace GameEngine {

	// モデルを描画するモード
	enum class RenderMode3D {
		DefaultModel, // 通常モデルを描画用
		DefaultModelAdd,
		Instancing,   // インスタンシング描画用
		InstancingAdd,
		Grid,  // グリッド描画用
		AnimationModel, // アニメーション描画用
		Skybox, // スカイボックスの描画用
		ShadowMap, // シャドウマップ用
	};

	class ModelRenderer {
	public:
		ModelRenderer() = default;
		~ModelRenderer() = default;

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="commandList"></param>
		static void StaticInitialize(ID3D12GraphicsCommandList* commandList,SrvManager* srvManager,PSOManager* psoManager);

		/// <summary>
		/// 描画前処理
		/// </summary>
		/// <param name="mode"></param>
		/// <param name="blendMode"></param>
		static void PreDraw(RenderMode3D mode);

		/// <summary>
		/// カメラを設定する
		/// </summary>
		/// <param name="vpMatrix"></param>
		/// <param name="cameraResource"></param>
		static void SetCamera(ID3D12Resource* cameraResource);

	public:

		/// <summary>
		/// 生成したモデルを描画
		/// </summary>
		/// <param name="worldMatrix">ワールド行列</param>
		/// <param name="directionalLightResource">光源</param>
		/// <param name="material">マテリアル : 何の書かなければデフォルトのマテリアルを適応</param>
		static void Draw(const Model* model, WorldTransform& worldTransform, const Material* material = nullptr);

		/// <summary>
		/// 生成したモデルの描画(ライト適応、複数マテリアル対応)
		/// </summary>
		/// <param name="worldTransform"></param>
		/// <param name="VPMatrix"></param>
		/// <param name="lightGroupResource"></param>
		/// <param name="cameraResource"></param>
		/// <param name="material"></param>
		static void Draw(const Model* model, WorldTransform& worldTransform, ID3D12Resource* lightGroupResource, const Material* material = nullptr);

		/// <summary>
		/// 生成したモデルの複数描画
		/// </summary>
		/// <param name="worldTransforms"></param>
		/// <param name="textureHandle"></param>
		/// <param name="VPMatrix"></param>
		/// <param name="material"></param>
		static void DrawInstancing(const Model* model, const uint32_t& numInstance, WorldTransforms& worldTransforms, const Material* material = nullptr);

		/// <summary>
		/// グリッドを描画
		/// </summary>
		static void DrawGrid(const Model* model, WorldTransform& worldTransform);

		/// <summary>
		/// アニメーションのあるモデルを描画
		/// </summary>
		/// <param name="worldTransform"></param>
		/// <param name="VPMatrix"></param>
		/// <param name="material"></param>
		static void DrawAnimation(const Model* model, WorldTransform& worldTransform, const Material* material = nullptr);

		/// <summary>
		/// モデルに光源を適応させる
		/// </summary>
		/// <param name="directionalLightResource"></param>
		static void DrawLight(ID3D12Resource* lightGroupResource);

		/// <summary>
		/// スカイボックスの描画
		/// </summary>
		/// <param name="model"></param>
		/// <param name="worldTransform"></param>
		/// <param name="material"></param>
		static void DrawSkybox(const Model* model, WorldTransform& worldTransform, const Material* material = nullptr);

		/// <summary>
		/// シャドウマップ用の描画処理
		/// </summary>
		/// <param name="model"></param>
		/// <param name="worldTransform"></param>
		static void DrawShadowMap(const Model* model, WorldTransform& worldTransform);

	private:

		// コマンドリスト
		static ID3D12GraphicsCommandList* commandList_;

		// psoデータのリスト
		static std::unordered_map<RenderMode3D, DrawPsoData> psoList_;

		// srv
		static SrvManager* srvManager_;

		// カメラリソース
		static ID3D12Resource* cameraResource_;
	};
}
