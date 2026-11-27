#include "RQCommon.hlsli"

// 通常マテリアル
[numthreads(RQ_SHADE_GROUP_SIZE, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 pixel;
    RQHit hit;
    if (!FetchShadePixel(id.x, RQ_MATERIAL_DEFAULT, pixel, hit))
    {
        return;
    }

    // デバック用にAOだけを白黒で表示する
    if (gIndices.aoDebugView != 0)
    {
        GetOutput()[pixel] = float4(GetAO(pixel).xxx, 1.0f);
        return;
    }
    // デバック用にコースティクスだけを表示する
    if (gIndices.causticDebugView != 0)
    {
        GetOutput()[pixel] = float4(GetCaustic(pixel), 1.0f);
        return;
    }

    // 1次レイのヒットは再帰段数1
    const int recursive = 1;

    float3 rayDir = GeneratePrimaryRay(pixel).Direction;
    SurfaceHit s = LoadSurfaceHit(hit);
    MaterialData material = GetBuffer(s.ref.MaterialIndex).Load<MaterialData>(0);

    // uvをトランスフォーム
    float2 uv = mul(float4(s.vtx.texcoord, 0.0f, 1.0f), material.uvTransform).xy;
    // テクスチャのLOD
    TriangleWorld tri = LoadTriangleWorld(s.ref, hit.primitiveIndex, s.objectToWorld);
    TexLod lod = MakePrimaryTexLod(tri, s.worldPosition, material.uvTransform);
    // 反射・屈折先のLODに使う、ヒット位置でのレイコーンの幅
    float coneWidth = PixelSpreadAngle() * hit.t;

    float3 localNormal = s.vtx.normal;
    // ノーマルマップがあれば法線に適応
    if (material.normalTextureHandle != 0)
    {
        float4 normalMapColor = SampleTexture(material.normalTextureHandle, uv, lod);
        localNormal = GetNormalFromMap(normalMapColor, s.vtx.normal, float4(normalize(s.vtx.tangent.xyz), s.vtx.tangent.w));
    }
    float3 worldPosition = s.worldPosition;
    float3 worldNormal = ObjectNormalToWorld(localNormal, s.objectToWorld);

    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);
    // 裏面の法線を視線側に向け直す
    if (dot(worldNormal, viewDir) < 0.0f) { worldNormal = -worldNormal; }

    // アルベド色を取得
    float4 textureColor = SampleTexture(material.textureHandle, uv, lod);
    float3 albedoColor = material.color.rgb * textureColor.rgb;

    // ライティングをしない場合はアルベドの色を返す
    if (!material.enableLighting)
    {
        GetOutput()[pixel] = float4(albedoColor, 1.0f);
        return;
    }

    // 平行光源
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.xyz * gDirectionalLight.intensity;
    float3 directLight = CalculateBRDF(albedoColor, worldNormal, viewDir, lightDir, lightColor, material.roughness, material.metallic);

    // 反射レイを飛ばして反射色を取得
    float3 reflectColor = TraceRadiance(worldPosition, reflect(rayDir, worldNormal), RAY_FLAG_NONE, recursive, coneWidth);

    // 環境光
    float3 indirectLight = CalculateIBL(albedoColor, reflectColor, worldNormal, viewDir, material.metallic, material.roughness);

    // RTAOと、背景から受ける環境光
    float ao = GetAO(pixel);
    float3 ambientLight = AmbientDiffuse(albedoColor, worldNormal, material.metallic) * ao;

    float3 color;
    if (s.ref.type == 1)
    {
        // 半透明は屈折レイを飛ばして表面色と混ぜる
        float3 refracted = refract(rayDir, worldNormal, 1.0f / material.ior);
        float3 refractColor = (length(refracted) < 0.01f)
            ? reflectColor
            : TraceRadiance(worldPosition, refracted, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, recursive, coneWidth);
        color = lerp(refractColor, directLight + indirectLight + ambientLight, material.color.a);
    }
    else
    {
        // 遮蔽物が不透明かをマスクで区別して影の濃さを取得する
        float shadowFactor = ComputeShadowFactor(worldPosition, lightDir);
        // 影の中に残す光(は環境光の代わりなので、AOで遮蔽する
        color = (directLight + indirectLight) * lerp(0.5f * ao, 1.0f, shadowFactor);
        color += ambientLight;
        // 氷で屈折して集まった光。フォトンの密度が面の向きによる減衰も含んでいる
        color += albedoColor * lightColor * GetCaustic(pixel) * (1.0f - material.metallic);
    }

    GetOutput()[pixel] = float4(color, 1.0f);
}
