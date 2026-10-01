#ifndef SURFACE_SHADING_HLSLI
#define SURFACE_SHADING_HLSLI
#include "../RayQueryCommon.hlsli"

// 不透明なPBRサーフェスのシェーディング
// worldNormal : 視線側を向いたワールド法線
// rayDir : ヒットしたレイの方向
void ShadeOpaqueSurface(inout ShadeResult result, float3 worldPosition, float3 worldNormal, float3 rayDir,
    float3 albedoColor, float metallic, float roughness)
{
    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);

    // ライト
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.xyz * gDirectionalLight.intensity;

    // 平行光源
    float3 directLight = CalculateBRDF(albedoColor, worldNormal, viewDir, lightDir, lightColor, roughness, metallic);

    // 環境光。CalculateIBLは反射色に対して線形なので、反射色以外の成分と反射色にかかる重みに分ける
    float3 iblBase = CalculateIBL(albedoColor, float3(0.0f, 0.0f, 0.0f), worldNormal, viewDir, metallic, roughness);
    float3 iblReflectWeight = CalculateIBL(albedoColor, float3(1.0f, 1.0f, 1.0f), worldNormal, viewDir, metallic, roughness) - iblBase;

    // 反射方向
    float3 reflectDir = reflect(normalize(rayDir), worldNormal);

    // 遮蔽物が不透明かをマスクで区別して影の濃さを取得する
    float shadowFactor = ComputeShadowFactor(worldPosition, lightDir);
    // 影の中であれば、影色を設定
    float shadowScale = lerp(0.5f, 1.0f, shadowFactor);

    // 最終的な色を設定
    result.color += (directLight + iblBase) * shadowScale;
    SetReflection(result, reflectDir, iblReflectWeight * shadowScale);
}

#endif
