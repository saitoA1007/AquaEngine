#ifndef DEFAULT_MATERIAL_HLSLI
#define DEFAULT_MATERIAL_HLSLI
#include "../RayQueryCommon.hlsli"

struct DefaultMaterialData
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

// デフォルトマテリアルのシェーディング
// rayDir : ヒットしたレイの方向
ShadeResult ShadeDefaultMaterial(HitInfo hit, float3 rayDir)
{
    ShadeResult result = InitShadeResult();

    // アクセスデータを取得
    BufferRef ref = gBufferRefs[hit.instanceID];
    // マテリアルデータを取得
    DefaultMaterialData material = gBufferData[NonUniformResourceIndex(ref.MaterialIndex)].Load<DefaultMaterialData>(0);

    // 頂点データを取得する
    VertexData vtx = FetchHitVertex(hit, ref.vertexHandle, ref.indexHandle, 0, 0);
    // uvをトランスフォーム
    float4 transformedUV = mul(float4(vtx.texcoord, 0.0f, 1.0f), material.uvTransform);

    float3 localNormal = vtx.normal;
    // ノーマルマップがあれば法線に適応
    if (material.normalTextureHandle != 0)
    {
        float4 normalMapColor = SampleMaterialTexture(material.normalTextureHandle, transformedUV.xy);
        vtx.tangent.xyz = normalize(vtx.tangent.xyz);
        localNormal = GetNormalFromMap(normalMapColor, vtx.normal, vtx.tangent);
    }
    // ワールド空間に変換
    float3 worldPosition = mul(vtx.position, hit.objectToWorld);
    float3x3 normalMatrix = transpose((float3x3) hit.worldToObject);
    float3 worldNormal = normalize(mul(localNormal, normalMatrix));
    result.position = worldPosition;

    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);

    // 裏面の法線を視線側に向け直す
    if (dot(worldNormal, viewDir) < 0.0f) { worldNormal = -worldNormal; }

    // テクスチャカラーを取得
    float4 textureColor = SampleMaterialTexture(material.textureHandle, transformedUV.xy);
    // アルベド色を取得
    float3 albedoColor = material.color.rgb * textureColor.rgb;

    // ライティングをしない場合はアルベドの色を返す
    if (!material.enableLighting)
    {
        result.color = albedoColor;
        return result;
    }

    // ライト
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.xyz * gDirectionalLight.intensity;

    // 平行光源
    float3 directLight = CalculateBRDF(albedoColor, worldNormal, viewDir, lightDir, lightColor, material.roughness, material.metallic);

    // 環境光。CalculateIBLは反射色に対して線形なので、反射色以外の成分と反射色にかかる重みに分ける
    float3 iblBase = CalculateIBL(albedoColor, float3(0.0f, 0.0f, 0.0f), worldNormal, viewDir, material.metallic, material.roughness);
    float3 iblReflectWeight = CalculateIBL(albedoColor, float3(1.0f, 1.0f, 1.0f), worldNormal, viewDir, material.metallic, material.roughness) - iblBase;

    // 反射方向
    float3 worldRayDir = normalize(rayDir);
    float3 reflectDir = reflect(worldRayDir, worldNormal);

    // 透明度の表示
    if (ref.type == 1)
    {
        float alpha = material.color.a;
        // オブジェクトの色。lerp(refractColor, objectColor, alpha)のobjectColor側
        result.color = (directLight + iblBase) * alpha;

        /// 屈折
        float nr = dot(worldNormal, worldRayDir);
        float3 refracted;
        if (nr < 0)
        {
            // 表面. 空気中 -> 屈折媒質.
            refracted = refract(worldRayDir, worldNormal, 1.0f / material.ior);
        }
        else
        {
            // 裏面. 屈折媒質 -> 空気中.
            refracted = refract(worldRayDir, -worldNormal, material.ior);
        }

        if (length(refracted) < 0.01)
        {
            // 全反射。屈折の代わりに反射レイを使う
            float3 weight = iblReflectWeight * alpha + (1.0f - alpha);
            SetReflection(result, reflectDir, weight);
        }
        else
        {
            SetReflection(result, reflectDir, iblReflectWeight * alpha);
            // 裏面をスキップ
            float3 weight = float3(1.0f, 1.0f, 1.0f) * (1.0f - alpha);
            SetRefraction(result, refracted, weight, RAY_FLAG_CULL_BACK_FACING_TRIANGLES);
        }
        return result;
    }

    // 遮蔽物が不透明かをマスクで区別して影の濃さを取得する
    float shadowFactor = ComputeShadowFactor(worldPosition, lightDir);
    // 影の中であれば、影色を設定
    float shadowScale = lerp(0.5f, 1.0f, shadowFactor);

    // 最終的な色を設定
    result.color = (directLight + iblBase) * shadowScale;
    SetReflection(result, reflectDir, iblReflectWeight * shadowScale);
    return result;
}

#endif
