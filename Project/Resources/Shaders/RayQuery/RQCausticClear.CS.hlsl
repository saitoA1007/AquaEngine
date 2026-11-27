#include "RQCommon.hlsli"

// コースティクスの加算先と氷の範囲をリセットする
[numthreads(RQ_TRACE_GROUP_SIZE, RQ_TRACE_GROUP_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (all(id.xy == 0))
    {
        RWStructuredBuffer<int> bounds = GetCausticBounds();
        bounds[0] = CAUSTIC_BOUNDS_EMPTY;
        bounds[1] = CAUSTIC_BOUNDS_EMPTY;
        bounds[2] = -CAUSTIC_BOUNDS_EMPTY;
        bounds[3] = -CAUSTIC_BOUNDS_EMPTY;
    }

    if (id.x >= gIndices.width || id.y >= gIndices.height)
    {
        return;
    }
    uint index = (id.y * gIndices.width + id.x) * 3;
    RWStructuredBuffer<uint> accum = GetCausticAccum();
    accum[index + 0] = 0;
    accum[index + 1] = 0;
    accum[index + 2] = 0;
}
