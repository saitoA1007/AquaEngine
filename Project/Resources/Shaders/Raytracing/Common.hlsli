#ifndef COMMON_HLSLI
#define COMMON_HLSLI
#include"../LightElement.hlsli"

struct Payload
{
    float3 color;
    int recursive;
};

struct MyAttribute
{
    float2 barys;
};

struct Camera
{
    float32_t3 worldPosition;
    float32_t4x4 vpMatrix;
    float32_t4x4 mtxViewInv; // ビュー逆行列
    float32_t4x4 mtxProjInv; // プロジェクション逆行列
};

struct MaterialRef
{
    uint32_t type; // マテリアルデータのタイプ
    uint32_t MaterialIndex; // マテリアルデータの参照するハンドル
};

// Global Root Signature
RWTexture2D<float4> gOutput : register(u0);
RaytracingAccelerationStructure gRtScene : register(t0,space0);
Texture2D<float32_t4> gTexture[] : register(t0, space1);
StructuredBuffer<MaterialRef> gBufferRefs : register(t0, space2);
ByteAddressBuffer gBufferData[] : register(t0, space3);

ConstantBuffer<Camera> gCamera : register(b0);
cbuffer LightGroup : register(b1)
{
    DirectionalLight gDirectionalLight;
    PointLight gPointLight;
    SpotLight gSpotLight;
    uint32_t environmentTexture;
    int32_t isActiveEnvironment;
};

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
    if (payload.recursive > 3)
    {
        payload.color = float3(0, 0, 0);
        return true;
    }
    return false;
}
#endif