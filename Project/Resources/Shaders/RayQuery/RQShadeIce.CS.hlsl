#include "RQCommon.hlsli"

// 氷のマテリアル
float3 IceBSDF(float3 rayDir, float hitT, float3 worldPos, float3 worldNormal, int recursive, float ior, float roughness, float3 iceColor, float shadowFactor, float coneWidth)
{
    // 裏面確認
    bool entering = dot(rayDir, worldNormal) < 0.0f;
    float3 N = entering ? worldNormal : -worldNormal;
    float eta = entering ? (1.0f / ior) : ior;

    // Fresnel反射率
    float cosTheta = saturate(dot(-rayDir, N));
    float r0 = (1.0f - ior) / (1.0f + ior);
    r0 = r0 * r0;
    // 氷のr0はior1.31で反射はほぼ映らないが、ビジュアル重視で設定してる
    r0 = max(r0, 0.3f);
    float F = saturate(r0 + (1.0f - r0) * pow(1.0f - cosTheta, 5.0f));

    // Beer-Lambert
    float3 transmittance = float3(1.0f, 1.0f, 1.0f);
    if (!entering)
    {
        float absorpScale = 1.0f - roughness * 0.6f;
        transmittance = BeerLambert(ICE_ABSORPTION_COEFF * absorpScale, hitT);
    }

    float3 reflectDir = reflect(rayDir, N);

    // 屈折方向の計算
    float3 refracted = refract(rayDir, N, eta);
    if (length(refracted) < 0.001f)
    {
        return TraceRadiance(worldPos, reflectDir, RAY_FLAG_NONE, recursive, coneWidth) * transmittance;
    }

    // 屈折
    float3 refractRadiance = TraceRadiance(worldPos, refracted, RAY_FLAG_NONE, recursive, coneWidth);
    float3 refractColor = refractRadiance * transmittance * iceColor;

    // 反射
    float3 reflectColor = TraceRadiance(worldPos, reflectDir, RAY_FLAG_NONE, recursive, coneWidth);

    // 近似SSS
    float3 sssContrib = float3(0.0f, 0.0f, 0.0f);
    if (entering && roughness > 0.01f && shadowFactor > 0.0f)
    {
        float3 lightDir = normalize(-gDirectionalLight.direction);
        float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        sssContrib = IceFakeSSS(N, lightDir, rayDir, lightColor, roughness * 0.4f);
        sssContrib *= (1.0f - F) * shadowFactor;
    }

    return lerp(refractColor, reflectColor, F) + sssContrib;
}

[numthreads(RQ_SHADE_GROUP_SIZE, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 pixel;
    RQHit hit;
    if (!FetchShadePixel(id.x, RQ_MATERIAL_ICE, pixel, hit))
    {
        return;
    }

    // 1次レイのヒットは再帰段数1
    const int recursive = 1;

    float3 rayDir = GeneratePrimaryRay(pixel).Direction;
    SurfaceHit s = LoadSurfaceHit(hit);
    IceMaterialData material = GetBuffer(s.ref.MaterialIndex).Load<IceMaterialData>(0);
    VertexData vtx = s.vtx;

    // uvをトランスフォーム
    float2 transformedUV = mul(float4(vtx.texcoord, 0.0f, 1.0f), material.uvTransform).xy;
    // テクスチャのLOD
    TriangleWorld tri = LoadTriangleWorld(s.ref, hit.primitiveIndex, s.objectToWorld);
    TexLod lod = MakePrimaryTexLod(tri, s.worldPosition, material.uvTransform);
    // 反射・屈折先のLODに使う、ヒット位置でのレイコーンの幅
    float coneWidth = PixelSpreadAngle() * hit.t;
    vtx.tangent.xyz = normalize(vtx.tangent.xyz);

    float3 worldPosition = s.worldPosition;
    float3x3 objectToWorldRot = (float3x3) s.objectToWorld;
    float3x3 worldToObjectRot = Inverse3x3(objectToWorldRot);

    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);

    float2 parallaxUV = transformedUV;
    // 視線ベクトルをオブジェクト空間へ戻す
    float3 localViewDir = normalize(mul(worldToObjectRot, viewDir));
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
            GetTexture(material.heightTextureHandle), gSampler,
            transformedUV, tangentViewDir, material.heightScale);
    }

    float3 bubbleColor = float3(0.0f, 0.0f, 0.0f);
    if (material.bubbleMaxDepth > 0.0001f)
    {
        // 屈折方向を計算
        float3 incident = -localViewDir; // 入射方向を反転して内向きに
        float eta = 1.0f / material.ior; // 空気→氷
        float3 refractedDir = refract(incident, vtx.normal, eta);
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
            float3 bubbleNormalWorld = normalize(mul(worldToObjectRot, bubbleHitNormalLocal));

            float NdotV = saturate(dot(bubbleNormalWorld, viewDir));
            float3 lightDir = normalize(-gDirectionalLight.direction);
            float3 halfVec = normalize(lightDir + viewDir);

            float bubbleDiffuse = saturate(dot(bubbleNormalWorld, lightDir)) * 0.4f;
            float bubbleSpec = pow(saturate(dot(bubbleNormalWorld, halfVec)), 48.0f);
            float bubbleRim = pow(1.0f - NdotV, 3.0f) * 0.3f;

            bubbleColor = float3(1.0f, 1.0f, 1.0f) * (bubbleDiffuse + bubbleSpec + bubbleRim) * material.bubbleHighlight;

            // 深度に応じて減衰させ、奥にある気泡ほど淡く見せる
            float3 absorption = float3(0.15f, 0.05f, 0.02f);
            bubbleColor *= BeerLambert(absorption, hitDistance);
        }
    }

    float3 localNormal = vtx.normal;
    // ノーマルマップがあれば法線に適応
    if (material.normalTextureHandle != 0)
    {
        float4 normalMapColor = SampleTexture(material.normalTextureHandle, parallaxUV, lod);
        localNormal = GetNormalFromMap(normalMapColor, vtx.normal, vtx.tangent);
    }

    // 粗い表面
    if (material.dissolveThreshold > 0.0f)
    {
        float mask = FBMNoise(vtx.position.xyz * 2.0f, 1);
        if (mask <= material.dissolveThreshold)
        {
            localNormal = ChiseledIceNormal(localNormal, vtx.position.xyz,
                material.chipScale, material.chipStrength,
                material.edgeWidth, material.edgeStrength,
                material.microScale, material.microStrength);
        }
    }

    float3 worldNormal = normalize(mul(objectToWorldRot, localNormal));
    // 裏面の法線を視線側に向け直す
    if (dot(worldNormal, viewDir) < 0.0f)
    {
        worldNormal = -worldNormal;
    }

    // テクスチャカラーを取得
    float4 textureColor = SampleTexture(material.textureHandle, parallaxUV, lod);

    float3 lightDir = normalize(-gDirectionalLight.direction);
    // 影を取る
    float shadowFactor = ComputeShadowFactor(worldPosition, lightDir);

    float3 iceColor = material.color.rgb * textureColor.rgb;
    float3 color = IceBSDF(rayDir, hit.t, worldPosition, worldNormal, recursive,
        material.ior, material.roughness, iceColor, shadowFactor, coneWidth);

    // 直射日光を失った分だけ透過、反射光をわずかに寒色へ寄せ、影の位置を判別できるようにする
    color *= lerp(ICE_SHADOW_TINT, float3(1.0f, 1.0f, 1.0f), shadowFactor);

    // 平行光源による鏡面ハイライト
    if (gDirectionalLight.active)
    {
        float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        float3 iceSpecular = CalcSpecular(worldNormal, lightDir, viewDir,
            lightColor, material.specularColor.rgb, material.shininess);
        color += iceSpecular * shadowFactor;
    }

    // リムライト
    float rimFactor = pow(1.0f - saturate(dot(worldNormal, viewDir)), material.rimPower);
    float3 rimLight = material.rimColor.rgb * rimFactor * material.rimIntensity * gDirectionalLight.intensity;
    color += rimLight * shadowFactor;

    // バブルを描画。気泡のハイライトは影の中では消える
    float3 F0Ice = float3(0.02f, 0.02f, 0.02f);
    float NdotV = saturate(dot(worldNormal, viewDir));
    float surfaceTransmittance = 1.0f - F_Schlick(NdotV, F0Ice).x;
    color += bubbleColor * surfaceTransmittance * shadowFactor;

    GetOutput()[pixel] = float4(color, 1.0f);
}
