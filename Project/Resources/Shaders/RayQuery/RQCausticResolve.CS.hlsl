#include "RQCommon.hlsli"

// ピクセルに置かれたフォトンを周囲から集め、面の上での密度から照度を求める
[numthreads(RQ_TRACE_GROUP_SIZE, RQ_TRACE_GROUP_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= gIndices.width || id.y >= gIndices.height)
    {
        return;
    }
    uint index = id.y * gIndices.width + id.x;
    RWStructuredBuffer<float4> output = GetCausticOutput();

    RQHit hit = GetHitBuffer()[index];
    float2 rectMin, rectSize;
    if (hit.instanceID == RQ_INVALID_ID || hit.material == RQ_MATERIAL_ICE || !LoadCausticRect(rectMin, rectSize))
    {
        output[index] = float4(0, 0, 0, 0);
        return;
    }

    RayDesc centerRay = GeneratePrimaryRay(id.xy);
    float3 cameraOrigin = centerRay.Origin;
    float3 centerPos = cameraOrigin + centerRay.Direction * hit.t;

    // 同じ平面にあるピクセルだけを集めるため、面の法線を使う
    BufferRef ref = GetBufferRefs()[hit.instanceID];
    float3 Ng = FetchGeometricNormalWorld(ref, hit.primitiveIndex, LoadObjectToWorld(hit.instanceIndex));

    float spread = PixelSpreadAngle();
    int radius = int(gIndices.causticGatherRadius);
    // 奥行き方向のずれの許容量。遠いほどピクセルが大きくなるので広げる
    float planeTolerance = 0.01f + 2.0f * spread * hit.t;

    RWStructuredBuffer<uint> accum = GetCausticAccum();
    float3 sumPower = float3(0, 0, 0);
    float sumArea = 0.0f;

    [loop]
    for (int dy = -radius; dy <= radius; ++dy)
    {
        [loop]
        for (int dx = -radius; dx <= radius; ++dx)
        {
            int r2 = dx * dx + dy * dy;
            if (r2 > radius * radius)
            {
                continue;
            }
            int2 q = int2(id.xy) + int2(dx, dy);
            if (any(q < 0) || q.x >= int(gIndices.width) || q.y >= int(gIndices.height))
            {
                continue;
            }
            uint qIndex = q.y * gIndices.width + q.x;
            RQHit qHit = GetHitBuffer()[qIndex];
            if (qHit.instanceID == RQ_INVALID_ID || qHit.material == RQ_MATERIAL_ICE)
            {
                continue;
            }
            float3 qDir = GeneratePrimaryRay(uint2(q)).Direction;
            float3 qPos = cameraOrigin + qDir * qHit.t;
            if (abs(dot(qPos - centerPos, Ng)) > planeTolerance)
            {
                continue;
            }

            // 円錐状の重み
            float w = 1.0f - sqrt(float(r2)) / float(radius + 1);
            // ピクセルが面の上で占める面積
            float cosTheta = max(abs(dot(Ng, qDir)), 0.1f);
            float pixelSize = qHit.t * spread;
            float area = pixelSize * pixelSize / cosTheta;

            float3 power = float3(accum[qIndex * 3 + 0], accum[qIndex * 3 + 1], accum[qIndex * 3 + 2]) / CAUSTIC_FIXED_SCALE;
            sumPower += w * power;
            sumArea += w * area;
        }
    }

    // フォトン1本は、ライトに垂直な面でCausticPhotonAreaの広さに当たる光を運ぶ
    float3 irradiance = sumPower * CausticPhotonArea(rectSize) / max(sumArea, 1e-8f);
    output[index] = float4(irradiance * gIndices.causticIntensity, 0.0f);
}
