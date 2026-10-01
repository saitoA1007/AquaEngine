#ifndef ICE_MATERIAL_HLSLI
#define ICE_MATERIAL_HLSLI
#include "../RayQueryCommon.hlsli"

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

// 氷のBeer-Lambert則の吸収係数
static const float3 ICE_ABSORPTION_COEFF = float3(0.80f, 0.25f, 0.04f);

// 影の中の氷にかける色
static const float3 ICE_SHADOW_TINT = float3(0.80f, 0.87f, 1.00f);

// 氷のBSDF。反射・屈折レイは飛ばさずにresultへ方向と重みを設定する
// shadowFactor : 影の中なら0、日向なら1。直接光由来の成分だけを遮るために使う
// tint : 反射・屈折を含めた全体にかける色
void IceBSDF(inout ShadeResult result, HitInfo hit, float3 rayDir, float3 worldPos, float3 worldNormal,
    float ior, float roughness, float3 iceColor, float shadowFactor, float3 tint)
{
    float3 worldRayDir = normalize(rayDir);

    // 裏面確認
    bool entering = dot(worldRayDir, worldNormal) < 0.0f;
    float3 N = entering ? worldNormal : -worldNormal;
    float eta = entering ? (1.0f / ior) : ior;

    // Fresnel反射率
    float cosTheta = saturate(dot(-worldRayDir, N));
    float r0 = (1.0f - ior) / (1.0f + ior);
    r0 = r0 * r0;
    // 氷のr0はior1.31で約1.8%ぐらいでほぼ反射は移りません。ですが、現在はいったんビジュアル面を重視して上限を上げています。
    r0 = max(r0, 0.3f);
    float F = r0 + (1.0f - r0) * pow(1.0f - cosTheta, 5.0f);
    F = saturate(F);

    // Beer-Lambert
    float3 transmittance = float3(1.0f, 1.0f, 1.0f);
    if (!entering)
    {
        float dist = hit.t;
        float absorpScale = 1.0f - roughness * 0.6f;
        float3 absorption = ICE_ABSORPTION_COEFF * absorpScale;
        transmittance = BeerLambert(absorption, dist);
    }

    float3 reflectDir = reflect(worldRayDir, N);

    // 屈折方向の計算
    float3 refracted = refract(worldRayDir, N, eta);
    if (length(refracted) < 0.001f)
    {
        // 全反射
        SetReflection(result, reflectDir, transmittance * tint);
        return;
    }

    // 透過光にBeer-Lambert吸収とiceColorティントを適用
    SetRefraction(result, refracted, (1.0f - F) * transmittance * iceColor * tint, RAY_FLAG_NONE);
    SetReflection(result, reflectDir, F * tint);

    // 近似SSS。影の中では消える
    float3 sssContrib = float3(0.0f, 0.0f, 0.0f);
    if (entering && roughness > 0.01f && shadowFactor > 0.0f)
    {
        float3 lightDir = normalize(-gDirectionalLight.direction);
        float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        float scatterStr = roughness * 0.4f;
        sssContrib = IceFakeSSS(N, lightDir, worldRayDir, lightColor, scatterStr);
        sssContrib *= (1.0f - F) * shadowFactor;
    }
    result.color += sssContrib * tint;
}

// 氷マテリアルのシェーディング
// rayDir : ヒットしたレイの方向
ShadeResult ShadeIceMaterial(HitInfo hit, float3 rayDir)
{
    ShadeResult result = InitShadeResult();

    // アクセスデータを取得
    BufferRef ref = gBufferRefs[hit.instanceID];
    // マテリアルデータを取得
    IceMaterialData material = gBufferData[NonUniformResourceIndex(ref.MaterialIndex)].Load<IceMaterialData>(0);

    // 頂点データを取得する
    VertexData vtx = FetchHitVertex(hit, ref.vertexHandle, ref.indexHandle, ref.vertexOffset, ref.indexOffset);
    // uvをトランスフォーム
    float4 transformedUV = mul(float4(vtx.texcoord, 0.0f, 1.0f), material.uvTransform);
    vtx.tangent.xyz = normalize(vtx.tangent.xyz);

    // ワールド空間に変換
    float3 worldPosition = mul(vtx.position, hit.objectToWorld);
    result.position = worldPosition;

    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);

    float2 parallaxUV = transformedUV.xy;
    // 視線ベクトルをオブジェクト空間へ戻す
    float3x3 worldToObjectRot = (float3x3) hit.worldToObject;
    float3 localViewDir = normalize(mul(viewDir, worldToObjectRot));
    // ハイトマップがあればUVをオフセットする
    if (material.heightTextureHandle != 0)
    {
        // オブジェクト空間のTBNで接空間へ変換
        float3 N = normalize(vtx.normal);
        float3 T = vtx.tangent.xyz;
        float3 B = normalize(cross(N, T) * vtx.tangent.w);
        float3x3 tbn = float3x3(T, B, N);
        float3 tangentViewDir = normalize(mul(tbn, localViewDir));

        parallaxUV = ParallaxOcclusionMapping(
            gTexture[NonUniformResourceIndex(material.heightTextureHandle)], gSampler,
            transformedUV.xy, tangentViewDir, material.heightScale);
    }

    float3 bubbleColor = float3(0.0f, 0.0f, 0.0f);
    if (material.bubbleMaxDepth > 0.0001f)
    {
        // 屈折方向を計算
        float3 localNormalForRefract = vtx.normal; // 表面法線
        float3 incident = -localViewDir; // 入射方向を反転して内向きに
        float eta = 1.0f / material.ior; // 空気→氷
        float3 refractedDir = refract(incident, localNormalForRefract, eta);

        if (dot(refractedDir, refractedDir) < 0.0001f)
        {
            refractedDir = incident;
        }

        float3 bubbleHitPos, bubbleHitNormalLocal;
        float hitDistance;
        bool hitBubble = ParallaxBubbleMapping(
            vtx.position.xyz, -refractedDir,
            material.bubbleScale, material.bubbleMaxDepth,
            material.bubbleJitter, material.bubbleDensity,
            bubbleHitPos, bubbleHitNormalLocal, hitDistance);

        if (hitBubble)
        {
            float3x3 normalMatrix = transpose((float3x3) hit.worldToObject);
            float3 bubbleNormalWorld = normalize(mul(normalMatrix, bubbleHitNormalLocal));

            float NdotV = saturate(dot(bubbleNormalWorld, viewDir));
            float3 lightDir = normalize(-gDirectionalLight.direction);
            float3 halfVec = normalize(lightDir + viewDir);

            float bubbleDiffuse = saturate(dot(bubbleNormalWorld, lightDir)) * 0.4f;
            float bubbleSpec = pow(saturate(dot(bubbleNormalWorld, halfVec)), 48.0f);
            float bubbleRim = pow(1.0f - NdotV, 3.0f) * 0.3f;

            bubbleColor = float3(1.0f, 1.0f, 1.0f) * (bubbleDiffuse + bubbleSpec + bubbleRim) * material.bubbleHighlight;

            // 深度に応じて減衰させ、奥にある気泡ほど淡く見せる
            float3 absorption = float3(0.15f, 0.05f, 0.02f);
            float3 depthAttenuation = BeerLambert(absorption, hitDistance);
            bubbleColor *= depthAttenuation;
        }
    }

    float3 localNormal = vtx.normal;
    // ノーマルマップがあれば法線に適応
    if (material.normalTextureHandle != 0)
    {
        float4 normalMapColor = SampleMaterialTexture(material.normalTextureHandle, parallaxUV);
        localNormal = GetNormalFromMap(normalMapColor, vtx.normal, vtx.tangent);
    }
    float3 worldNormal = mul(localNormal, (float3x3) hit.objectToWorld);
    worldNormal = normalize(worldNormal);

    // 裏面の法線を視線側に向け直す
    if (dot(worldNormal, viewDir) < 0.0f)
    {
        worldNormal = -worldNormal;
    }

    // テクスチャカラーを取得
    float4 textureColor = SampleMaterialTexture(material.textureHandle, parallaxUV);

    if (material.dissolveThreshold > 0.0f)
    {
        float mask = FBMNoise(worldPosition * 2.0f, 1);
        if (mask <= material.dissolveThreshold)
        {
            // 法線を取得
            worldNormal = ChiseledIceNormal(worldNormal, worldPosition,
                                    material.chipScale, material.chipStrength,
                                    material.edgeWidth, material.edgeStrength,
                                    material.microScale, material.microStrength);
        }
    }

    float3 lightDir = normalize(-gDirectionalLight.direction);
    // 影を取る
    float shadowFactor = ComputeShadowFactor(worldPosition, lightDir);

    // 直射日光を失った分だけ透過、反射光をわずかに寒色へ寄せ、影の位置を判別できるようにする
    float3 shadowTint = lerp(ICE_SHADOW_TINT, float3(1.0f, 1.0f, 1.0f), shadowFactor);

    float3 iceColor = material.color.rgb * textureColor.rgb;
    IceBSDF(result, hit, rayDir, worldPosition, worldNormal,
        material.ior, material.roughness, iceColor, shadowFactor, shadowTint);

    // 平行光源による鏡面ハイライト。影の中では出ない
    if (gDirectionalLight.active)
    {
        float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        float3 iceSpecular = CalcSpecular(worldNormal, lightDir, viewDir,
            lightColor, material.specularColor.rgb, material.shininess);
        result.color += iceSpecular * shadowFactor;
    }

    // 視線と法線の内積を取る
    float rimNdotV = saturate(dot(worldNormal, viewDir));
    float rimFactor = 1.0f - rimNdotV;
    rimFactor = pow(rimFactor, material.rimPower);
    // リムライトの最終成分
    float3 rimLight = material.rimColor.rgb * rimFactor * material.rimIntensity * gDirectionalLight.intensity;
    result.color += rimLight * shadowFactor;

    // バブルを描画。気泡のハイライトは影の中では消える
    float3 F0Ice = float3(0.02f, 0.02f, 0.02f);
    float NdotV = saturate(dot(worldNormal, viewDir));
    float surfaceFresnel = F_Schlick(NdotV, F0Ice).x;
    float transmittance = 1.0f - surfaceFresnel;
    result.color += bubbleColor * transmittance * shadowFactor;
    return result;
}

#endif
