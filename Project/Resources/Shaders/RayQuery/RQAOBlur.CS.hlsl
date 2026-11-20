#include "RQCommon.hlsli"

// RTAOのぼかし
[numthreads(RQ_TRACE_GROUP_SIZE, RQ_TRACE_GROUP_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= gIndices.width || id.y >= gIndices.height)
    {
        return;
    }
    uint index = id.y * gIndices.width + id.x;
    RWStructuredBuffer<RQAOSample> history = GetAOHistoryCurrent();
    RQAOSample center = history[index];
    if (center.depth < 0.0f)
    {
        GetAOOutput()[index] = 1.0f;
        return;
    }

    int stride = center.historyLength < 8.0f ? 2 : 1;
    float sum = 0.0f;
    float weightSum = 0.0f;

    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            int2 p = int2(id.xy) + int2(x, y) * stride;
            if (p.x < 0 || p.y < 0 || p.x >= (int) gIndices.width || p.y >= (int) gIndices.height)
            {
                continue;
            }
            RQAOSample s = history[p.y * gIndices.width + p.x];
            if (s.depth < 0.0f)
            {
                continue;
            }
            float spatial = exp(-(x * x + y * y) / 4.5f);
            float normalWeight = pow(saturate(dot(s.normal, center.normal)), 32.0f);
            float depthWeight = exp(-abs(s.depth - center.depth) / (center.depth * 0.02f + 1e-4f));
            float w = spatial * normalWeight * depthWeight;
            sum += s.ao * w;
            weightSum += w;
        }
    }

    GetAOOutput()[index] = weightSum > 0.0f ? sum / weightSum : center.ao;
}
