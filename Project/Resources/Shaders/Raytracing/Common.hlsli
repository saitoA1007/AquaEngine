#ifndef COMMON_HLSLI
#define COMMON_HLSLI

struct Payload
{
    float3 color;
    int recursive;
};

struct MyAttribute
{
    float2 barys;
};

struct SceneCB
{
    matrix mtxView; // ビュー行列.
    matrix mtxProj; // プロジェクション行列.
    matrix mtxViewInv; // ビュー逆行列.
    matrix mtxProjInv; // プロジェクション逆行列.
    float4 lightDirection; // 平行光源の向き.
    float4 lightColor; // 平行光源色.
    float4 ambientColor; // 環境光.
    float4 eyePosition; // 視点.

    float3 pointLight; // ポイントライト.
    uint shadowRayCount; // シャドウレイ数.
    uint4 flags; // x: 平行光源シャドウON/OFF, y: ポイントライト位置描画
};

struct MaterialRef
{
    uint32_t type; // マテリアルデータのタイプ
    uint32_t MaterialIndex = 0; // マテリアルデータの参照するハンドル
};

// Global Root Signature
RaytracingAccelerationStructure gRtScene : register(t0);
Texture2D<float32_t4> gTexture[] : register(t0, space1);
StructuredBuffer<MaterialRef> gMaterialRefs[] : register(t0, space2);
ByteAddressBuffer gBufferData[] : register(t0, space3);

ConstantBuffer<SceneCB> gSceneParam : register(b0);

inline float3 CalcBarycentrics(float2 barys)
{
    return float3(
        1.0 - barys.x - barys.y,
        barys.x,
        barys.y);
}

inline float2 CalcHitAttribute2(float2 vertexAttribute[3], float2 barycentrics)
{
    float2 ret;
    ret = vertexAttribute[0];
    ret += barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]);
    ret += barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
    return ret;
}

float3 CalcHitAttribute3(float3 vertexAttribute[3], float2 barycentrics)
{
    float3 ret;
    ret = vertexAttribute[0];
    ret += barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]);
    ret += barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
    return ret;
}

// レイの再帰チェック
inline bool checkRecursiveLimit(inout Payload payload)
{
    payload.recursive++;
    if (payload.recursive >= 15)
    {
        payload.color = float3(0, 0, 0);
        return true;
    }
    return false;
}
#endif