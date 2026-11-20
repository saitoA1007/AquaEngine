#include "RQCommon.hlsli"

// 蓄積するフレーム数の上限。大きいほどなめらかになるが、変化への追従が遅くなる
static const float AO_MAX_HISTORY = 32.0f;
// 履歴が周囲の範囲から大きく外れていたときに、蓄積をやり直すフレーム数
static const float AO_CLAMP_RESET_LENGTH = 4.0f;
// これ以上外れていたら、履歴が古くなったとみなす
static const float AO_CLAMP_RESET_THRESHOLD = 0.1f;

// 履歴
void ClampHistory(uint2 pixel, RQAOSample current, inout float historyAO, inout float historyLength)
{
    RWStructuredBuffer<RQAOSample> currentHistory = GetAOHistoryCurrent();
    float m1 = 0.0f;
    float m2 = 0.0f;
    float count = 0.0f;

    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            int2 p = int2(pixel) + int2(x, y);
            if (p.x < 0 || p.y < 0 || p.x >= (int) gIndices.width || p.y >= (int) gIndices.height)
            {
                continue;
            }
            // rawAOとdepth、normalは他のスレッドが書き換えないので、同じDispatch内で読む
            RQAOSample s = currentHistory[p.y * gIndices.width + p.x];
            bool sameSurface = s.depth > 0.0f
                && abs(s.depth - current.depth) < current.depth * 0.05f
                && dot(s.normal, current.normal) > 0.9f;
            if (!sameSurface)
            {
                continue;
            }
            m1 += s.rawAO;
            m2 += s.rawAO * s.rawAO;
            count += 1.0f;
        }
    }

    float mean = m1 / count;
    float sigma = sqrt(max(m2 / count - mean * mean, 0.0f));
    float clamped = clamp(historyAO, mean - gIndices.aoClampGamma * sigma, mean + gIndices.aoClampGamma * sigma);

    // 大きく外れていた履歴は古いので、蓄積をやり直して新しい値に早く追従させる
    if (abs(clamped - historyAO) > AO_CLAMP_RESET_THRESHOLD)
    {
        historyLength = min(historyLength, AO_CLAMP_RESET_LENGTH);
    }
    historyAO = clamped;
}

[numthreads(RQ_TRACE_GROUP_SIZE, RQ_TRACE_GROUP_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= gIndices.width || id.y >= gIndices.height)
    {
        return;
    }
    uint index = id.y * gIndices.width + id.x;
    RWStructuredBuffer<RQAOSample> currentHistory = GetAOHistoryCurrent();
    RQAOSample current = currentHistory[index];
    if (current.depth < 0.0f)
    {
        return;
    }

    RQPrevCamera prev = GetPrevCamera()[0];
    float historyAO = 0.0f;
    float historyLength = 0.0f;

    if (prev.valid != 0)
    {
        // このピクセルのワールド位置を前フレームの画面へ投影する
        float3 worldPos = gCamera.worldPosition.xyz + GeneratePrimaryRay(id.xy).Direction * current.depth;
        float4 clip = mul(float4(worldPos, 1.0f), prev.vpMatrix);
        if (clip.w > 0.0f)
        {
            float2 ndc = clip.xy / clip.w;
            // GeneratePrimaryRayの逆変換
            float2 prevPixel = float2((ndc.x + 1.0f) * 0.5f * gIndices.width, (1.0f - ndc.y) * 0.5f * gIndices.height) - 0.5f;
            float expectedDepth = distance(prev.worldPosition, worldPos);

            // 周囲4ピクセルの履歴を、同じ面とみなせるものだけ双線形で混ぜる
            int2 base = int2(floor(prevPixel));
            float2 f = prevPixel - base;
            RWStructuredBuffer<RQAOSample> previousHistory = GetAOHistoryPrevious();
            float weightSum = 0.0f;
            [unroll]
            for (int k = 0; k < 4; ++k)
            {
                int2 offset = int2(k & 1, k >> 1);
                int2 p = base + offset;
                if (p.x < 0 || p.y < 0 || p.x >= (int) gIndices.width || p.y >= (int) gIndices.height)
                {
                    continue;
                }
                RQAOSample h = previousHistory[p.y * gIndices.width + p.x];
                // 深度と法線が大きく違えば別の面
                bool sameSurface = h.depth > 0.0f
                    && abs(h.depth - expectedDepth) < expectedDepth * 0.05f
                    && dot(h.normal, current.normal) > 0.9f;
                if (!sameSurface)
                {
                    continue;
                }
                float w = (offset.x ? f.x : 1.0f - f.x) * (offset.y ? f.y : 1.0f - f.y);
                historyAO += h.ao * w;
                historyLength += h.historyLength * w;
                weightSum += w;
            }

            if (weightSum > 0.01f)
            {
                historyAO /= weightSum;
                historyLength /= weightSum;
            }
            else
            {
                historyLength = 0.0f;
            }
        }
    }

    // 物体が動いたときの残像対策
    if (historyLength > 0.0f && gIndices.aoHistoryClamp != 0)
    {
        ClampHistory(id.xy, current, historyAO, historyLength);
    }

    // 履歴が短いうちは今フレームの値を強く、長くなるほど履歴を重視する
    float length = min(historyLength + 1.0f, AO_MAX_HISTORY);
    current.ao = lerp(historyAO, current.ao, 1.0f / length);
    current.historyLength = length;
    currentHistory[index] = current;
}
