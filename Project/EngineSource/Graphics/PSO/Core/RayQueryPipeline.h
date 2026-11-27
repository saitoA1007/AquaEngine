#pragma once
#include <array>
#include <d3d12.h>
#include <wrl.h>

#include "StructuredBuffer.h"
#include "RtaoSettings.h"
#include "TextureLodSettings.h"
#include "CausticSettings.h"
#include "Vector4.h"

namespace GameEngine {

	/// <summary>
	/// RayQueryでシェーディングするマテリアルの種類
	/// </summary>
	enum class RayQueryMaterial : uint32_t {
		kDefault = 0, // 通常モデル
		kIce = 1,     // 氷

		kCount
	};

	// PSOManagerに登録するRayQuery用の名前
	namespace RayQueryPsoName {
		inline constexpr const char* kRootSignature = "RayQuery";
		inline constexpr const char* kReset = "RQReset";
		inline constexpr const char* kTrace = "RQTrace";
		inline constexpr const char* kBuildArgs = "RQBuildArgs";
		inline constexpr const char* kAOTrace = "RQAOTrace";
		inline constexpr const char* kAOTemporal = "RQAOTemporal";
		inline constexpr const char* kAOBlur = "RQAOBlur";
		inline constexpr const char* kCausticClear = "RQCausticClear";
		inline constexpr const char* kCausticBounds = "RQCausticBounds";
		inline constexpr const char* kCausticTrace = "RQCausticTrace";
		inline constexpr const char* kCausticResolve = "RQCausticResolve";
		// RayQueryMaterialの順に並べる
		inline constexpr const char* kShade[static_cast<size_t>(RayQueryMaterial::kCount)] = {
			"RQShadeDefault",
			"RQShadeIce",
		};
		// Dispatchだけを行うコマンドシグネチャ
		inline constexpr const char* kDispatchSignature = "RQDispatchIndirect";
	}

	// RayQueryで使用するリソースのヒープインデックス。HLSLのRQIndicesと一致させる
	struct RayQueryIndices {
		uint32_t tlas;        // TLAS
		uint32_t bufferRefs;  // BufferRefのStructuredBuffer
		uint32_t output;      // 出力画像(UAV)
		uint32_t depthOutput; // 出力深度(UAV)
		uint32_t skybox;      // 背景のキューブマップ
		uint32_t hitBuffer;   // 1次レイのヒット情報(UAV)
		uint32_t pixelList;   // マテリアルごとのピクセルリスト(UAV)
		uint32_t counter;     // マテリアルごとのピクセル数(UAV)
		uint32_t dispatchArgs;// マテリアルごとのDispatch引数(UAV)
		uint32_t width;       // 出力の幅
		uint32_t height;      // 出力の高さ

		// RTAO
		uint32_t frameIndex;        // 乱数の種に使うフレーム番号
		uint32_t aoHistoryCurrent;  // 今フレームのAO履歴(UAV)
		uint32_t aoHistoryPrevious; // 前フレームのAO履歴(UAV)
		uint32_t aoOutput;          // ノイズ除去後のAO(UAV)
		uint32_t prevCamera;        // 前フレームのカメラ(UAV)
		uint32_t aoEnabled;         // AOを使用するか
		uint32_t aoSampleCount;     // 1ピクセルあたりのAOレイの本数
		float aoRadius;             // 遮蔽物を探す距離
		float aoStrength;           // AOの強さ
		float ambientIntensity;     // 環境光の強さ
		uint32_t aoDebugView;       // AOだけを表示するか
		uint32_t aoHistoryClamp;    // 履歴を周囲の値の範囲に収めるか
		float aoClampGamma;         // 収める範囲の広さ

		// テクスチャのLOD
		uint32_t texLodEnabled;     // LODを計算するか
		float texLodBias;           // ミップレベルに足す値

		// コースティクス
		uint32_t causticEnabled;          // コースティクスを使用するか
		uint32_t causticBounds;           // 氷のライト空間での範囲(UAV)
		uint32_t causticAccum;            // ピクセルごとに着地したフォトンの量(UAV)
		uint32_t causticOutput;           // 密度推定後の照度(UAV)
		uint32_t causticCoarseResolution; // 範囲を探すフォトンの縦横の本数
		uint32_t causticPhotonResolution; // 氷の範囲へ撃つフォトンの縦横の本数
		float causticEmitRange;           // 氷を探す範囲の半径
		uint32_t causticGatherRadius;     // フォトンを集めるピクセルの半径
		float causticIntensity;           // 明るさ
		float causticIceShadowDensity;    // コースティクス使用時の氷の影の濃さ
		uint32_t causticDebugView;        // コースティクスだけを表示するか
	};

	// グローバルルートシグネチャのパラメータ番号
	enum class RayQueryRootParam : uint32_t {
		kIndices,       // ヒープインデックス
		kCamera,        // カメラ
		kLight,         // ライト
		kInstanceDescs, // TLASのインスタンス情報
	};

	// 1フレームの描画に必要な外部リソース
	struct RayQueryFrameParams {
		uint32_t tlasSrvIndex = 0;
		D3D12_GPU_VIRTUAL_ADDRESS instanceDescAddress = 0;
		uint32_t bufferRefSrvIndex = 0;
		uint32_t outputUavIndex = 0;
		uint32_t depthOutputUavIndex = 0;
		uint32_t skyboxSrvIndex = 0;
		D3D12_GPU_VIRTUAL_ADDRESS cameraAddress = 0;
		D3D12_GPU_VIRTUAL_ADDRESS lightAddress = 0;
	};

	/// <summary>
	/// RayQueryによる描画パイプライン
	/// </summary>
	class PSOManager;

	class RayQueryPipeline {
	public:
		RayQueryPipeline() = default;
		~RayQueryPipeline() = default;

		// 初期化
		void Initialize(ID3D12Device5* device, PSOManager* psoManager, uint32_t width, uint32_t height);

		// 描画コマンドを積む
		void Execute(ID3D12GraphicsCommandList4* cmdList, const RayQueryFrameParams& params);

		// RTAOの設定
		void SetRtaoSettings(const RtaoSettings& settings) { rtaoSettings_ = settings; }
		const RtaoSettings& GetRtaoSettings() const { return rtaoSettings_; }

		// テクスチャのLODの設定
		void SetTextureLodSettings(const TextureLodSettings& settings) { textureLodSettings_ = settings; }

		// コースティクスの設定
		void SetCausticSettings(const CausticSettings& settings) { causticSettings_ = settings; }

	private:

		// 1次レイのヒット情報
		struct RayQueryHit {
			uint32_t instanceIndex; // TLAS内のインスタンス番号
			uint32_t instanceID;    // BufferRefのインデックス
			uint32_t primitiveIndex;
			float barycentrics[2];
			float t;
			uint32_t material;      // マテリアル番号
			uint32_t pad;
		};

		// AOの履歴
		struct RayQueryAOSample {
			float normal[3];        // ワールド法線
			float depth;            // カメラからの距離
			float ao;               // AO
			float historyLength;    // 蓄積したフレーム数
			float rawAO;            // ノイズ除去前のこのフレームのAO
			float pad;
		};

		// 前フレームのカメラ
		struct RayQueryPrevCamera {
			float vpMatrix[16];
			float worldPosition[3];
			uint32_t valid;         // 0なら履歴を使わない
		};

	private:
		RayQueryPipeline(const RayQueryPipeline&) = delete;
		RayQueryPipeline& operator=(const RayQueryPipeline&) = delete;

		// Dispatch時のスレッドグループサイズ
		static constexpr uint32_t kTraceGroupSize = 8;   // 8x8
		static constexpr uint32_t kShadeGroupSize = 64;  // 64x1
		// 氷の範囲を探すフォトンの縦横の本数
		static constexpr uint32_t kCausticCoarseResolution = 512;
		// 氷の範囲へ撃つフォトンの縦横の本数の上限
		static constexpr uint32_t kMaxCausticPhotonResolution = 4096;
		// フォトンを集めるピクセルの半径の上限
		static constexpr uint32_t kMaxCausticGatherRadius = 16;

		ID3D12Device5* device_ = nullptr;

		uint32_t width_ = 0;
		uint32_t height_ = 0;

		// PSOManagerが所有するルートシグネチャとPSO
		ID3D12RootSignature* rootSignature_ = nullptr;
		ID3D12PipelineState* resetPso_ = nullptr;
		ID3D12PipelineState* tracePso_ = nullptr;
		ID3D12PipelineState* buildArgsPso_ = nullptr;
		// RTAO
		ID3D12PipelineState* aoTracePso_ = nullptr;
		ID3D12PipelineState* aoTemporalPso_ = nullptr;
		ID3D12PipelineState* aoBlurPso_ = nullptr;
		// コースティクス
		ID3D12PipelineState* causticClearPso_ = nullptr;
		ID3D12PipelineState* causticBoundsPso_ = nullptr;
		ID3D12PipelineState* causticTracePso_ = nullptr;
		ID3D12PipelineState* causticResolvePso_ = nullptr;
		// マテリアルごとのシェーディングPSO
		std::array<ID3D12PipelineState*, static_cast<size_t>(RayQueryMaterial::kCount)> shadePsos_{};

		// GPUで作った引数でDispatchするためのコマンドシグネチャ
		ID3D12CommandSignature* dispatchSignature_ = nullptr;

		// 中間バッファ
		StructuredBuffer<RayQueryHit> hitBuffer_;
		StructuredBuffer<uint32_t> pixelList_;
		StructuredBuffer<uint32_t> counter_;
		StructuredBuffer<D3D12_DISPATCH_ARGUMENTS> dispatchArgs_;
		// RTAOの中間バッファ。履歴は前フレームと交互に使う
		std::array<StructuredBuffer<RayQueryAOSample>, 2> aoHistory_;
		StructuredBuffer<float> aoOutput_;
		StructuredBuffer<RayQueryPrevCamera> prevCamera_;
		// コースティクスの中間バッファ
		StructuredBuffer<int32_t> causticBounds_;  // 氷の範囲。最小x,最小y,最大x,最大y
		StructuredBuffer<uint32_t> causticAccum_;  // ピクセルごとのフォトンの量。RGBを固定小数で持つ
		StructuredBuffer<Vector4> causticOutput_;  // 密度推定後の照度

		// RTAOの設定
		RtaoSettings rtaoSettings_;
		// テクスチャのLODの設定
		TextureLodSettings textureLodSettings_;
		// コースティクスの設定
		CausticSettings causticSettings_;

		// フレーム番号
		uint32_t frameIndex_ = 0;

	private:

		// 中間バッファを作成
		void CreateBuffers();

		// UAVバリアを張る
		void UavBarrier(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* resource);
	};
}
