#include "RayQueryCommon.hlsli"
#include "MaterialDispatch.hlsli"

// 反射レイを1段だけ飛ばす
// ヒットしたマテリアルの二次レイはこれ以上飛ばさず、背景色で近似する
float3 TraceReflectionOnce(float3 origin, float3 direction, int depth)
{
    // 再帰上限に達していれば背景色を返す
    if (depth >= MAX_RAY_DEPTH - 1)
    {
        return SampleBackground(direction);
    }

    HitInfo hit = TraceClosestHit(MakeRay(origin, direction), RAY_FLAG_NONE, RAY_MASK_ALL);
    if (!hit.isHit)
    {
        return SampleBackground(direction);
    }

    ShadeResult shade = EvaluateMaterial(hit, direction);
    float3 color = shade.color;
    if (shade.hasReflection)
    {
        color += shade.reflectionWeight * SampleBackground(shade.reflectionDir);
    }
    if (shade.hasRefraction)
    {
        color += shade.refractionWeight * SampleBackground(shade.refractionDir);
    }
    return color;
}

// カメラからのレイを追跡して色を求める
// 屈折を優先してループで追跡し、反射は各ヒットで1段だけ飛ばす
// 屈折が無い場合は反射を主経路として追跡する
float3 TracePath(RayDesc primaryRay, out float outDepth)
{
    outDepth = 1.0f;

    float3 radiance = float3(0.0f, 0.0f, 0.0f);
    float3 throughput = float3(1.0f, 1.0f, 1.0f);

    RayDesc ray = primaryRay;
    uint rayFlags = RAY_FLAG_NONE;

    [loop]
    for (int depth = 0; depth < MAX_RAY_DEPTH; ++depth)
    {
        // 再帰上限に達したら、ヒットの有無に関わらず背景色を返す
        if (depth == MAX_RAY_DEPTH - 1)
        {
            radiance += throughput * SampleBackground(ray.Direction);
            break;
        }

        HitInfo hit = TraceClosestHit(ray, rayFlags, RAY_MASK_ALL);

        // 何も当たらなければ背景色
        if (!hit.isHit)
        {
            radiance += throughput * SampleBackground(ray.Direction);
            break;
        }

        // マテリアルを評価
        ShadeResult shade = EvaluateMaterial(hit, ray.Direction);

        // 一次レイのヒット位置の深度を書き込む
        if (depth == 0)
        {
            float4 clipPos = mul(float4(shade.position, 1.0f), gCamera.vpMatrix);
            outDepth = clipPos.z / clipPos.w;
        }

        radiance += throughput * shade.color;

        if (shade.hasRefraction)
        {
            // 反射は1段だけ飛ばす
            if (shade.hasReflection)
            {
                radiance += throughput * shade.reflectionWeight * TraceReflectionOnce(shade.position, shade.reflectionDir, depth + 1);
            }

            // 屈折を主経路として追跡を続ける
            throughput *= shade.refractionWeight;
            ray = MakeRay(shade.position, shade.refractionDir);
            rayFlags = shade.refractionFlags;
        }
        else if (shade.hasReflection)
        {
            // 反射を主経路として追跡を続ける
            throughput *= shade.reflectionWeight;
            ray = MakeRay(shade.position, shade.reflectionDir);
            rayFlags = RAY_FLAG_NONE;
        }
        else
        {
            break;
        }
    }

    return radiance;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 dims;
    gOutput.GetDimensions(dims.x, dims.y);

    uint2 launchIndex = dispatchThreadId.xy;
    if (launchIndex.x >= dims.x || launchIndex.y >= dims.y)
    {
        return;
    }

    float2 d = (float2(launchIndex) + 0.5f) / float2(dims) * 2.0f - 1.0f;

    matrix mtxViewInv = gCamera.mtxViewInv;
    matrix mtxProjInv = gCamera.mtxProjInv;

    // カメラからのレイを生成
    float3 origin = mul(float4(0, 0, 0, 1), mtxViewInv).xyz;
    float4 target = mul(float4(d.x, -d.y, 1, 1), mtxProjInv);
    float3 direction = mul(float4(target.xyz / target.w, 0), mtxViewInv).xyz;

    float depth = 1.0f;
    float3 color = TracePath(MakeRay(origin, normalize(direction)), depth);

    gOutput[launchIndex] = float4(color, 1.0f);
    gDepthOutput[launchIndex] = depth;
}
