#ifndef MATERIAL_SURFACE_HLSLI
#define MATERIAL_SURFACE_HLSLI

// マテリアルグラフから生成されるサーフェス関数の入出力
// ラスタライズ(PS)とRayQuery(CS)の両方から同じ関数を呼び出す

// テクスチャのサンプリング方法。CSでは微分が使えないためSampleLevelに置き換える
#ifndef MATERIAL_SAMPLE
#define MATERIAL_SAMPLE(tex, uv) (tex).Sample(gSampler, (uv))
#endif

// サーフェス関数への入力
struct MaterialInput
{
    float2 texcoord; // UV
    float3 normal; // ワールド法線
    float3 worldPosition; // ワールド座標
};

// サーフェス関数の出力
struct SurfaceOutput
{
    float4 baseColor;
    float metallic;
    float roughness;
    float3 normal; // ワールド法線
    float3 emissive;
};

// ノード間で型が異なる場合に明示的に変換する
float AsFloat(float v) { return v; }
float AsFloat(float2 v) { return v.x; }
float AsFloat(float3 v) { return v.x; }
float AsFloat(float4 v) { return v.x; }

float2 AsFloat2(float v) { return float2(v, v); }
float2 AsFloat2(float2 v) { return v; }
float2 AsFloat2(float3 v) { return v.xy; }
float2 AsFloat2(float4 v) { return v.xy; }

float3 AsFloat3(float v) { return float3(v, v, v); }
float3 AsFloat3(float2 v) { return float3(v, 0.0f); }
float3 AsFloat3(float3 v) { return v; }
float3 AsFloat3(float4 v) { return v.xyz; }

float4 AsFloat4(float v) { return float4(v, v, v, v); }
float4 AsFloat4(float2 v) { return float4(v, 0.0f, 1.0f); }
float4 AsFloat4(float3 v) { return float4(v, 1.0f); }
float4 AsFloat4(float4 v) { return v; }

#endif
