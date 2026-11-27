#include "RQCommon.hlsli"

// 氷の表面の法線(ワールド空間)。1次レイのシェーディングと同じ凹凸を使う
float3 IcePhotonNormal(SurfaceHit s, RQHit hit, IceMaterialData material, float3 rayDir, float coneWidth)
{
    float3 localNormal = s.vtx.normal;
    if (material.normalTextureHandle != 0)
    {
        float2 uv = mul(float4(s.vtx.texcoord, 0.0f, 1.0f), material.uvTransform).xy;
        TexLod lod = MakeConeTexLod(LoadTriangleWorld(s.ref, hit.primitiveIndex, s.objectToWorld), material.uvTransform, coneWidth, rayDir);
        float4 normalMapColor = SampleTexture(material.normalTextureHandle, uv, lod);
        localNormal = GetNormalFromMap(normalMapColor, s.vtx.normal, float4(normalize(s.vtx.tangent.xyz), s.vtx.tangent.w));
    }
    // 削れた表面
    if (material.dissolveThreshold > 0.0f)
    {
        float mask = FBMNoise(s.vtx.position.xyz * 2.0f, 1);
        if (mask <= material.dissolveThreshold)
        {
            localNormal = ChiseledIceNormal(localNormal, s.vtx.position.xyz,
                material.chipScale, material.chipStrength,
                material.edgeWidth, material.edgeStrength,
                material.microScale, material.microStrength);
        }
    }
    return ObjectNormalToWorld(localNormal, s.objectToWorld);
}

// 着地したフォトンを、その位置が見えているピクセルへ加算する
void DepositPhoton(float3 position, float3 power)
{
    float4 clip = mul(float4(position, 1.0f), gCamera.vpMatrix);
    if (clip.w <= 0.0f)
    {
        return;
    }
    float2 ndc = clip.xy / clip.w;
    if (any(abs(ndc) >= 1.0f))
    {
        return;
    }
    // GeneratePrimaryRayの逆
    uint2 pixel = uint2((ndc.x * 0.5f + 0.5f) * gIndices.width, (0.5f - ndc.y * 0.5f) * gIndices.height);
    pixel = min(pixel, uint2(gIndices.width - 1, gIndices.height - 1));

    uint index = pixel.y * gIndices.width + pixel.x;
    RQHit hit = GetHitBuffer()[index];
    if (hit.instanceID == RQ_INVALID_ID || hit.material == RQ_MATERIAL_ICE)
    {
        return;
    }
    // カメラから見えている面に着地したかを距離で確かめる
    float3 cameraOrigin = GeneratePrimaryRay(pixel).Origin;
    float dist = distance(cameraOrigin, position);
    if (abs(dist - hit.t) > 0.02f + 0.01f * dist)
    {
        return;
    }

    uint3 fixedPower = uint3(power * CAUSTIC_FIXED_SCALE + 0.5f);
    RWStructuredBuffer<uint> accum = GetCausticAccum();
    InterlockedAdd(accum[index * 3 + 0], fixedPower.r);
    InterlockedAdd(accum[index * 3 + 1], fixedPower.g);
    InterlockedAdd(accum[index * 3 + 2], fixedPower.b);
}

// 氷の範囲へ細かくフォトンを撃ち、氷で屈折したフォトンを着地点に置く
[numthreads(RQ_TRACE_GROUP_SIZE, RQ_TRACE_GROUP_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint res = gIndices.causticPhotonResolution;
    if (id.x >= res || id.y >= res || !gDirectionalLight.active)
    {
        return;
    }
    float2 rectMin, rectSize;
    if (!LoadCausticRect(rectMin, rectSize))
    {
        return;
    }

    // 格子の模様が出ないよう、セルの中で位置をずらす。フレームごとには変えずにちらつきを防ぐ
    uint seed = PcgHash(id.x + PcgHash(id.y));
    float2 jitter = float2(seed & 0xFFFF, seed >> 16) / 65536.0f;
    float2 uv = rectMin + (float2(id.xy) + jitter) / float(res) * rectSize;

    CausticFrame frame = MakeCausticFrame();
    float3 origin = frame.center + frame.T * uv.x + frame.B * uv.y;
    float3 dir = frame.dir;
    // テクスチャのLODに使うフォトン1本分の幅
    float coneWidth = sqrt(CausticPhotonArea(rectSize));

    float3 power = float3(1.0f, 1.0f, 1.0f);
    bool passedIce = false;

    [loop]
    for (int bounce = 0; bounce < CAUSTIC_MAX_BOUNCES; ++bounce)
    {
        RQHit hit;
        uint material;
        if (!TraceClosest(MakeRay(origin, dir), RAY_FLAG_NONE, RAY_MASK_ALL, hit, material))
        {
            return;
        }
        SurfaceHit s = LoadSurfaceHit(hit);

        if (material != RQ_MATERIAL_ICE)
        {
            // 氷を通った光だけがコースティクスになる。直接光は通常のライティングで計算済み
            if (passedIce)
            {
                DepositPhoton(s.worldPosition, power);
            }
            return;
        }
        passedIce = true;

        IceMaterialData ice = GetBuffer(s.ref.MaterialIndex).Load<IceMaterialData>(0);
        float3 N = IcePhotonNormal(s, hit, ice, dir, coneWidth);

        bool entering = dot(dir, N) < 0.0f;
        float3 n = entering ? N : -N;
        float eta = entering ? (1.0f / ice.ior) : ice.ior;

        // 氷の中を進んだ距離だけ吸収する
        if (!entering)
        {
            float absorpScale = 1.0f - ice.roughness * 0.6f;
            power *= BeerLambert(ICE_ABSORPTION_COEFF * absorpScale, hit.t);
        }

        float3 refracted = refract(dir, n, eta);
        if (dot(refracted, refracted) < 1e-6f)
        {
            // 全反射は氷の中に留まる
            dir = reflect(dir, n);
        }
        else
        {
            // 反射した分は失う。見た目用の下限は付けずに物理的なFresnelを使う
            float cosTheta = saturate(dot(-dir, n));
            float r0 = (1.0f - ice.ior) / (1.0f + ice.ior);
            r0 *= r0;
            float F = r0 + (1.0f - r0) * pow(1.0f - cosTheta, 5.0f);
            power *= 1.0f - F;
            if (entering)
            {
                power *= ice.color.rgb;
            }
            dir = normalize(refracted);
        }

        origin = s.worldPosition;
        if (MaxComponent(power) < 0.01f)
        {
            return;
        }
    }
}
