#include "RQCommon.hlsli"

// 粗い間隔でフォトンを撃ち、最初に氷へ当たったフォトンの範囲を求める
[numthreads(RQ_TRACE_GROUP_SIZE, RQ_TRACE_GROUP_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint res = gIndices.causticCoarseResolution;
    if (id.x >= res || id.y >= res || !gDirectionalLight.active)
    {
        return;
    }

    CausticFrame frame = MakeCausticFrame();
    float cell = CausticCoarseCellSize();
    float2 uv = (float2(id.xy) + 0.5f) * cell - gIndices.causticEmitRange;
    float3 origin = frame.center + frame.T * uv.x + frame.B * uv.y;

    // 氷を探すだけなので、最初のヒットが氷かどうかで判定する
    RQHit hit;
    uint material;
    if (!TraceClosest(MakeRay(origin, frame.dir), RAY_FLAG_NONE, RAY_MASK_ALL, hit, material) || material != RQ_MATERIAL_ICE)
    {
        return;
    }

    RWStructuredBuffer<int> bounds = GetCausticBounds();
    InterlockedMin(bounds[0], int(id.x));
    InterlockedMin(bounds[1], int(id.y));
    InterlockedMax(bounds[2], int(id.x));
    InterlockedMax(bounds[3], int(id.y));
}
