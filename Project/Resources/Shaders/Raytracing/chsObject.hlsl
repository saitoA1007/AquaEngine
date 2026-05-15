#include "Common.hlsli"
#include "../LightElement.hlsli"

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
    uint textureHandle;
    float metallic;
    int isActiveShadow;
    
    float ior;
    float roughness;
    float2 padding1;
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
    
    // 深度情報を書き込む
    float4 clipPos = mul(float4(worldPosition, 1.0f), gCamera.vpMatrix);
    payload.depth = clipPos.z / clipPos.w;
  
    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);
    
     // 裏面の法線を視線側に向け直す
    if (dot(worldNormal, viewDir) < 0.0f) { worldNormal = -worldNormal;}
    
    // アクセスデータを取得
    uint refHandle = InstanceID();
    MaterialRef ref = gBufferRefs[refHandle];
    // マテリアルデータを取得
    MaterialData material = gBufferData[ref.MaterialIndex].Load<MaterialData>(0);
    
    // テクスチャカラーを取得
    float4 textureColor = gTexture[material.textureHandle].SampleLevel(gSampler, vtx.texcoord, 0);
    // アルベド色を取得
    float3 albedoColor = material.color.rgb * textureColor.rgb;
    
    // ライティングをしない場合はアルベドの色を返す
    if (!material.enableLighting)
    {
        payload.color = albedoColor;
        return;
    }
     
    // ライト
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.xyz * gDirectionalLight.intensity;
    
    // 平行光源
    float3 directLight = CalculateBRDF(albedoColor, worldNormal, viewDir, lightDir, lightColor, material.roughness, material.metallic);
    
    // 反射レイを飛ばして反射色を取得
    float3 reflectColor = Reflection(vtx.position.xyz, vtx.normal, payload.recursive);
    
    // 環境光
    float3 indirectLight = CalculateIBL(albedoColor, reflectColor, worldNormal, viewDir, material.metallic, material.roughness);
    
     // 透明度の表示
    if (ref.type == 1)
    {   
        /// 屈折
        // 屈折レイを飛ばす
        float3 refractColor = TranslucentRefraction(vtx.position.xyz, vtx.normal, payload.recursive, material.ior);
        // オブジェクトの色を取得
        float3 objectColor = directLight + indirectLight;
        
        payload.color = lerp(refractColor, objectColor, material.color.a);
        return;
    }
    
    // 最終的な色を設定
    payload.color = directLight + indirectLight;
    
    // 影判定を取得
    bool isInShadow = ShootShadowRay(worldPosition, lightDir);
    // 影の中であれば、影色を設定
    if (isInShadow)
    {
        payload.color.xyz *= 0.5;
    }
}