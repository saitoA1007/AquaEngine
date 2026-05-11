#include "Common.hlsli"

struct VertexData {
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct MaterialData {
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
    float32_t3 specularColor;
    float shininess;
    uint32_t textureHandle;
    float metallic;
    int32_t isActiveShadow;
};

StructuredBuffer<uint> indexBuffer : register(t0, space4);
StructuredBuffer<VertexData> vertexBuffer : register(t1, space4);
SamplerState gSampler : register(s0);

VertexData GetHitVertex(MyAttribute attrib) {
    uint start = PrimitiveIndex() * 3;
    
    float3 positions[3];
    float2 texcoords[3];
    float3 normals[3];

    for (int i = 0; i < 3; ++i) {
        uint index = indexBuffer[start + i];
        positions[i] = vertexBuffer[index].position.xyz;
        normals[i] = vertexBuffer[index].normal;
        texcoords[i] = vertexBuffer[index].texcoord;
    }
    
    VertexData v;
    v.position.xyz = CalcHitAttribute3(positions, attrib.barys);
    v.position.w = 1.0f;
    v.texcoord = CalcHitAttribute2(texcoords, attrib.barys);
    v.normal = CalcHitAttribute3(normals, attrib.barys);
    v.normal = normalize(v.normal);
    return v;
}

// Lambert拡散反射
inline float3 CalcDiffuse(float3 normal, float3 lightDir, float3 lightColor, float3 albedo) 
{
    float dotNL = saturate(dot(normal, lightDir));
    return dotNL * lightColor * albedo;
}

// Phong鏡面反射
inline float3 CalcSpecular(float3 normal, float3 lightDir, float3 viewDir,
    float3 lightColor, float3 specularColor, float specularPower)
{
    float3 reflectDir = reflect(-lightDir, normal);
    float dotRV = saturate(dot(reflectDir, viewDir));
    return pow(dotRV, specularPower) * lightColor * specularColor;
}

[shader("closesthit")]
void MainObjectCHS(inout Payload payload, MyAttribute attrib) {
    if (checkRecursiveLimit(payload))
    {
        return;
    }
    
    VertexData vtx = GetHitVertex(attrib);

    // ワールド空間に変換
    float3 worldPosition = mul(vtx.position, ObjectToWorld4x3());
    float3 worldNormal = mul(vtx.normal, (float3x3) ObjectToWorld4x3());
    
    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);
    
    // アクセスデータを取得
    uint refHandle = InstanceID();
    MaterialRef ref = gBufferRefs[refHandle];
    MaterialData material = gBufferData[ref.MaterialIndex].Load<MaterialData>(0);
    
     // 拡散反射
    float3 diffuse = CalcDiffuse(worldNormal, gDirectionalLight.direction, gDirectionalLight.color.xyz, material.color.xyz);
    
    // 鏡面反射
    float3 specular = CalcSpecular(worldNormal, gDirectionalLight.direction, viewDir,
        gDirectionalLight.color.xyz, material.specularColor,material.shininess);

    // 最終的な色を設定
    payload.color = diffuse + specular;
}