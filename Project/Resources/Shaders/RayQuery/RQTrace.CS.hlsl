#include "RQCommon.hlsli"

// 1次レイだけを飛ばし、ヒット情報を保存してマテリアルごとのピクセルリストに振り分ける
[numthreads(RQ_TRACE_GROUP_SIZE, RQ_TRACE_GROUP_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= gIndices.width || id.y >= gIndices.height)
    {
        return;
    }

    RayDesc ray = GeneratePrimaryRay(id.xy);

    RQHit hit;
    uint material;
    if (!TraceClosest(ray, RAY_FLAG_NONE, RAY_MASK_ALL, hit, material))
    {
        // ミスしたピクセルはここで背景を書いて終わる
        GetOutput()[id.xy] = float4(SampleEnvironment(ray.Direction), 1.0f);
        GetDepthOutput()[id.xy] = 1.0f;
        // ミスの印を残す(RTAOで参照する)
        RQHit missHit = (RQHit) 0;
        missHit.instanceID = RQ_INVALID_ID;
        GetHitBuffer()[id.y * gIndices.width + id.x] = missHit;
        return;
    }

    // ヒット情報を保存
    GetHitBuffer()[id.y * gIndices.width + id.x] = hit;

    // 深度を書き込む
    GetDepthOutput()[id.xy] = WriteDepthFromWorld(ray.Origin + ray.Direction * hit.t);

    // マテリアルのピクセルリストに追加
    RWStructuredBuffer<uint> counter = GetCounter();
    uint slot;
    InterlockedAdd(counter[material], 1, slot);
    GetPixelList()[material * GetPixelCount() + slot] = id.x | (id.y << 16);
}
