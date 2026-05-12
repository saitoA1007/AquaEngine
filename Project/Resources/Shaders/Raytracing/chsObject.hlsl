#include "Common.hlsli"

struct VertexData {
    float4 position;
    float2 texcoord;
    float3 normal;
};

struct MaterialData {
    float4 color;
    
    int enableLighting;
    float3 padding0;
    
    float4x4 uvTransform;
    
    float4 specularColor;
    
    float shininess;
    uint32_t textureHandle;
    float metallic;
    int32_t isActiveShadow;
};

StructuredBuffer<uint> indexBuffer : register(t0, space4);
StructuredBuffer<VertexData> vertexBuffer : register(t1, space4);

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
    
    VertexData v = (VertexData) 0;
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
    float NdotL = dot(normal, lightDir);
    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    float3 diffuse = albedo * lightColor * cos;
    return diffuse;
}

// Blinn-Phong鏡面反射
inline float3 CalcSpecular(float3 normal, float3 lightDir, float3 viewDir,
    float3 lightColor, float3 specularColor, float shininess)
{  
    float3 halfVector = normalize(lightDir + viewDir);
    float NDotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NDotH), shininess);
    float3 specular = lightColor * specularPow * specularColor;
    return specular;
}

[shader("closesthit")]
void MainObjectCHS(inout Payload payload, MyAttribute attrib) {    
    if (checkRecursiveLimit(payload))
    {
        return;
    }
    
    // 頂点データを取得する
    VertexData vtx = GetHitVertex(attrib);
    
    // ワールド空間に変換
    float3 worldPosition = mul(vtx.position, ObjectToWorld4x3());
    float3 worldNormal = mul(vtx.normal, (float3x3)ObjectToWorld4x3());
    worldNormal = normalize(worldNormal);
    
    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);
    
    // アクセスデータを取得
    uint refHandle = InstanceID();
    MaterialRef ref = gBufferRefs[refHandle];
    // マテリアルデータを取得
    MaterialData material = gBufferData[ref.MaterialIndex].Load<MaterialData>(0);
    
    // テクスチャカラーを取得
    float4 textureColor = gTexture[material.textureHandle].SampleLevel(gSampler, vtx.texcoord, 0);
    
    // 色を取得
    float3 albedoColor = material.color.rgb * textureColor.rgb;
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.xyz * gDirectionalLight.intensity;
     // 拡散反射
    float3 diffuse = CalcDiffuse(worldNormal, lightDir, lightColor, albedoColor);
    // 鏡面反射
    float3 specular = CalcSpecular(worldNormal, lightDir, viewDir,
       lightColor, material.specularColor.rgb, material.shininess);
    
    // 反射レイを飛ばして反射色を取得
    float3 reflectColor = Reflection(vtx.position.xyz, vtx.normal, payload.recursive);

    // Schlick近似によるFresnel係数
    float3 shadingNormal = dot(worldNormal, viewDir) < 0.0f ? -worldNormal : worldNormal;
    float cosTheta = saturate(dot(shadingNormal, viewDir));
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedoColor, material.metallic);
    float3 fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    // 反射色を取得
    float3 tintedReflect = reflectColor * lerp(float3(1, 1, 1), albedoColor, material.metallic);
    
     // 最終的な色を設定
    payload.color = lerp(diffuse + specular, tintedReflect, fresnel);
}