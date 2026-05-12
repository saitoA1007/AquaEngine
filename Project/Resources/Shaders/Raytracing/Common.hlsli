#ifndef COMMON_HLSLI
#define COMMON_HLSLI
#include "../LightElement.hlsli"

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
RaytracingAccelerationStructure gRtScene : register(t0, space0);
Texture2D<float32_t4> gTexture[] : register(t0, space1);
StructuredBuffer<MaterialRef> gBufferRefs : register(t0, space2);
ByteAddressBuffer gBufferData[] : register(t0, space3);
TextureCube<float4> gBackgroundTexture : register(t1, space0);
SamplerState gSampler : register(s0);

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
    if (payload.recursive >= 3)
    {
        // 背景画像を返す
        payload.color = gBackgroundTexture.SampleLevel(
            gSampler, WorldRayDirection(), 0.0).rgb;
        return true;
    }
    return false;
}

// 反射
float3 Reflection(float3 vertexPosition, float3 vertexNormal, int recursive)
{
    float3 worldPos = mul(float4(vertexPosition, 1), ObjectToWorld4x3());
    float3 worldNormal = mul(vertexNormal, (float3x3) ObjectToWorld4x3());
    float3 worldRayDir = WorldRayDirection();
    float3 reflectDir = reflect(worldRayDir, worldNormal);

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xFF;

    RayDesc rayDesc;
    rayDesc.Origin = worldPos;
    rayDesc.Direction = reflectDir;
    rayDesc.TMin = 0.001f;
    rayDesc.TMax = 100000;

    Payload reflectPayload;
    reflectPayload.color = float3(0, 0, 0);
    reflectPayload.recursive = recursive;
    TraceRay(
        gRtScene,
        flags,
        rayMask,
        0, // ray index
        1, // MultiplierForGeometryContrib
        0, // miss index
        rayDesc,
        reflectPayload);
    return reflectPayload.color;
}
#endif