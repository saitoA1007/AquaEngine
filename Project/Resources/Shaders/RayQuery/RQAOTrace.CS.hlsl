#include "RQCommon.hlsli"

// RTAO
[numthreads(RQ_TRACE_GROUP_SIZE, RQ_TRACE_GROUP_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= gIndices.width || id.y >= gIndices.height)
    {
        return;
    }
    uint index = id.y * gIndices.width + id.x;
    RQHit hit = GetHitBuffer()[index];

    RQAOSample result = (RQAOSample) 0;
    result.ao = 1.0f;
    result.depth = -1.0f;

    // ミスしたピクセルと氷は対象外
    if (hit.instanceID == RQ_INVALID_ID || hit.material == RQ_MATERIAL_ICE)
    {
        GetAOHistoryCurrent()[index] = result;
        return;
    }

    SurfaceHit s = LoadSurfaceHit(hit);
    float3 viewDir = normalize(gCamera.worldPosition.xyz - s.worldPosition);

    // 頂点法線を半球の中心に、面の法線を自己交差の回避に使う
    float3 N = ObjectNormalToWorld(s.vtx.normal, s.objectToWorld);
    if (dot(N, viewDir) < 0.0f) { N = -N; }
    float3 Ng = FetchGeometricNormalWorld(s.ref, hit.primitiveIndex, s.objectToWorld);
    if (dot(Ng, N) < 0.0f) { Ng = -Ng; }

    float3 up = abs(N.y) < 0.999f ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);

    float3 origin = s.worldPosition + Ng * 0.01f;
    uint sampleCount = max(gIndices.aoSampleCount, 1u);
    float occlusion = 0.0f;

    for (uint i = 0; i < sampleCount; ++i)
    {
        // コサイン分布で半球の方向を選ぶ
        float2 u = Random2(id.xy, gIndices.frameIndex, i);
        float r = sqrt(u.x);
        float phi = 2.0f * PI * u.y;
        float3 dir = T * (r * cos(phi)) + B * (r * sin(phi)) + N * sqrt(1.0f - u.x);
        // 面の裏側へ向かうレイは面で折り返す
        if (dot(dir, Ng) <= 0.0f)
        {
            dir = reflect(dir, Ng);
        }

        RayDesc ray = MakeRay(origin, dir);
        ray.TMax = gIndices.aoRadius;

        // 遮蔽物の有無だけ分かればよいので、最初のヒットで打ち切る
        RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
        q.TraceRayInline(GetScene(), RAY_FLAG_NONE, RAY_MASK_ALL, ray);
        q.Proceed();
        if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        {
            // 氷は光を通すので、影と同じく一部だけ遮る
            bool isIce = q.CommittedInstanceContributionToHitGroupIndex() == RQ_MATERIAL_ICE;
            occlusion += isIce ? ICE_SHADOW_DENSITY : 1.0f;
        }
    }

    result.normal = N;
    result.depth = distance(gCamera.worldPosition.xyz, s.worldPosition);
    result.ao = 1.0f - occlusion / sampleCount;
    result.rawAO = result.ao;
    result.historyLength = 0.0f;
    GetAOHistoryCurrent()[index] = result;
}
