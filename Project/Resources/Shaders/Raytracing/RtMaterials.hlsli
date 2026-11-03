#ifndef RT_MATERIALS_HLSLI
#define RT_MATERIALS_HLSLI

// レイトレーシングで共有する頂点・マテリアルの構造体

struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 tangent;
};

struct MaterialData
{
    float4 color;

    int enableLighting;
    float dissolveThreshold;
    float2 padding0;

    float4x4 uvTransform;

    float4 specularColor;

    float shininess;
    uint textureHandle;
    float metallic;
    int isActiveShadow;

    float ior;
    float roughness;
    uint normalTextureHandle;
    uint dissolveTextureHandle;
};

struct IceMaterialData
{
    float4 color;

    int enableLighting;
    float dissolveThreshold;
    float2 padding0;

    float4x4 uvTransform;

    float4 specularColor;

    float shininess;
    uint textureHandle;
    float metallic;
    int isActiveShadow;

    float ior;
    float roughness;
    uint normalTextureHandle;
    uint dissolveTextureHandle;

    float chipScale;
    float chipStrength;
    float edgeWidth;
    float edgeStrength;

    float microScale;
    float microStrength;
    uint heightTextureHandle;
    float heightScale;

    float bubbleScale;
    float bubbleMaxDepth;
    float bubbleDensity;
    float bubbleJitter;

    float bubbleHighlight;
    float rimIntensity;
    float rimPower;
    float1 padding1;

    float4 rimColor;
};

#endif
