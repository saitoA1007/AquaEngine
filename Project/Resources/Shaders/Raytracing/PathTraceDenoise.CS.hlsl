// パストレーシングのデノイズ
// MainTemporalCS    : 前フレームの結果を再投影して時間方向に蓄積する
// MainStoreCameraCS : 次のフレームの再投影のために、今のカメラを保存する
// MainAtrousCS      : 法線・距離・輝度の差で重みを付けるÀ-Trousフィルタ。間隔を広げながら繰り返す
// 光はアルベドで割ってからフィルタし(テクスチャの模様をぼかさないため)、最後にアルベドを掛け戻す

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
    float4x4 mtxViewInv;
    float4x4 mtxProjInv;
};

cbuffer DenoiseConstants : register(b0)
{
    uint gStepSize; // À-Trousのサンプル間隔
    uint gIsFinal; // 1なら最終出力に書き込む
    uint gResetHistory; // 1なら履歴を使わない
    uint gPadding;
};
ConstantBuffer<Camera> gCamera : register(b1);

RWTexture2D<float4> gNoisy : register(u0); // パストレーシングの結果
RWTexture2D<float4> gGuide : register(u1); // xyz:法線, w:距離
RWTexture2D<float4> gPrevGuide : register(u2); // 前フレームのガイド
RWTexture2D<float4> gAlbedo : register(u3); // アルベド
RWTexture2D<float4> gPrevHistory : register(u4); // rgb:蓄積した色, a:蓄積数
RWTexture2D<float4> gHistory : register(u5);
RWTexture2D<float4> gPrevMoments : register(u6); // x:輝度の平均, y:輝度の2乗の平均
RWTexture2D<float4> gMoments : register(u7);
RWTexture2D<float4> gFilterInput : register(u8); // rgb:色, a:分散
RWTexture2D<float4> gFilterOutput : register(u9);
RWTexture2D<float4> gOutput : register(u10); // 最終出力
RWTexture2D<float4> gCameraHistory : register(u11); // 前フレームのカメラ。0~3:VP行列の各行, 4:カメラ位置

// 蓄積数の上限。多いほど滑らかだが、動きに対して残像が出やすい
static const float MAX_HISTORY_LENGTH = 32.0f;
// 前フレームの法線とのずれの許容量(内積)
static const float REPROJECT_NORMAL_THRESHOLD = 0.9f;
// 前フレームの距離とのずれの許容量(距離に対する割合)
static const float REPROJECT_DEPTH_THRESHOLD = 0.05f;
// À-Trousの重みの鋭さ
static const float PHI_NORMAL = 128.0f;
static const float PHI_DEPTH = 0.02f;
static const float PHI_LUMINANCE = 4.0f;

float Luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// アルベドで割るときの値。0に近いと光が大きくなりすぎるため下限を設ける
float3 DemodulateAlbedo(float3 albedo)
{
    return max(albedo, float3(0.02f, 0.02f, 0.02f));
}

bool InBounds(int2 p, uint2 size)
{
    return p.x >= 0 && p.y >= 0 && p.x < (int) size.x && p.y < (int) size.y;
}

// 画素中心を通るカメラレイの向き(MainPathTraceCSと同じ)
float3 CameraRayDirection(uint2 pixel, float2 dims)
{
    float2 d = (pixel + 0.5f) / dims * 2.0f - 1.0f;
    float4 target = mul(float4(d.x, -d.y, 1, 1), gCamera.mtxProjInv);
    return normalize(mul(float4(target.xyz / target.w, 0), gCamera.mtxViewInv).xyz);
}

[numthreads(8, 8, 1)]
void MainTemporalCS(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 size;
    gNoisy.GetDimensions(size.x, size.y);
    uint2 pixel = dispatchID.xy;
    if (!InBounds(int2(pixel), size))
    {
        return;
    }

    float4 guide = gGuide[pixel];
    float3 albedo = DemodulateAlbedo(gAlbedo[pixel].rgb);
    float3 color = gNoisy[pixel].rgb / albedo;

    // 背景はノイズが無いのでそのまま
    if (guide.w <= 0.0f)
    {
        gHistory[pixel] = float4(color, 0.0f);
        gMoments[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        gFilterOutput[pixel] = float4(color, 0.0f);
        return;
    }

    float3 normal = guide.xyz;
    float3 origin = mul(float4(0, 0, 0, 1), gCamera.mtxViewInv).xyz;
    float3 worldPos = origin + CameraRayDirection(pixel, float2(size)) * guide.w;

    // 前フレームの画面上の位置を求める
    float4x4 prevVP = float4x4(gCameraHistory[uint2(0, 0)], gCameraHistory[uint2(1, 0)],
                               gCameraHistory[uint2(2, 0)], gCameraHistory[uint2(3, 0)]);
    float3 prevCameraPos = gCameraHistory[uint2(4, 0)].xyz;
    float4 prevClip = mul(float4(worldPos, 1.0f), prevVP);
    float2 prevNdc = prevClip.xy / prevClip.w;
    float2 prevPixel = float2(prevNdc.x * 0.5f + 0.5f, -prevNdc.y * 0.5f + 0.5f) * float2(size) - 0.5f;
    float expectedDist = length(worldPos - prevCameraPos);

    // 周囲4画素から、法線と距離が一致するものだけを双線形補間で集める
    float3 prevColor = float3(0.0f, 0.0f, 0.0f);
    float2 prevMoments = float2(0.0f, 0.0f);
    float prevLength = 0.0f;
    float weightSum = 0.0f;
    if (gResetHistory == 0 && prevClip.w > 0.0f)
    {
        int2 base = int2(floor(prevPixel));
        float2 f = prevPixel - float2(base);
        for (int i = 0; i < 4; ++i)
        {
            int2 offset = int2(i & 1, i >> 1);
            int2 p = base + offset;
            if (!InBounds(p, size))
            {
                continue;
            }
            float4 prevGuide = gPrevGuide[p];
            bool valid = prevGuide.w > 0.0f
                && dot(prevGuide.xyz, normal) > REPROJECT_NORMAL_THRESHOLD
                && abs(prevGuide.w - expectedDist) < expectedDist * REPROJECT_DEPTH_THRESHOLD;
            if (!valid)
            {
                continue;
            }
            float w = (offset.x == 1 ? f.x : 1.0f - f.x) * (offset.y == 1 ? f.y : 1.0f - f.y);
            float4 history = gPrevHistory[p];
            prevColor += history.rgb * w;
            prevLength += history.a * w;
            prevMoments += gPrevMoments[p].xy * w;
            weightSum += w;
        }
    }

    float lum = Luminance(color);
    float historyLength = 1.0f;
    float3 integratedColor = color;
    float2 moments = float2(lum, lum * lum);
    if (weightSum > 0.001f)
    {
        prevColor /= weightSum;
        prevMoments /= weightSum;
        prevLength /= weightSum;
        historyLength = min(prevLength + 1.0f, MAX_HISTORY_LENGTH);
        float alpha = 1.0f / historyLength;
        integratedColor = lerp(prevColor, color, alpha);
        moments = lerp(prevMoments, moments, alpha);
    }

    float variance = max(0.0f, moments.y - moments.x * moments.x);
    // 蓄積が少ない間は、周囲の画素から分散を見積もる
    if (historyLength < 4.0f)
    {
        float sum = 0.0f;
        float sumSq = 0.0f;
        float count = 0.0f;
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                int2 p = int2(pixel) + int2(x, y);
                if (!InBounds(p, size) || gGuide[p].w <= 0.0f)
                {
                    continue;
                }
                float l = Luminance(gNoisy[p].rgb / DemodulateAlbedo(gAlbedo[p].rgb));
                sum += l;
                sumSq += l * l;
                count += 1.0f;
            }
        }
        float mean = sum / count;
        variance = max(variance, sumSq / count - mean * mean);
    }

    gHistory[pixel] = float4(integratedColor, historyLength);
    gMoments[pixel] = float4(moments, 0.0f, 0.0f);
    gFilterOutput[pixel] = float4(integratedColor, variance);
}

[numthreads(1, 1, 1)]
void MainStoreCameraCS(uint3 dispatchID : SV_DispatchThreadID)
{
    gCameraHistory[uint2(0, 0)] = gCamera.vpMatrix[0];
    gCameraHistory[uint2(1, 0)] = gCamera.vpMatrix[1];
    gCameraHistory[uint2(2, 0)] = gCamera.vpMatrix[2];
    gCameraHistory[uint2(3, 0)] = gCamera.vpMatrix[3];
    gCameraHistory[uint2(4, 0)] = float4(mul(float4(0, 0, 0, 1), gCamera.mtxViewInv).xyz, 1.0f);
}

[numthreads(8, 8, 1)]
void MainAtrousCS(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 size;
    gFilterInput.GetDimensions(size.x, size.y);
    uint2 pixel = dispatchID.xy;
    if (!InBounds(int2(pixel), size))
    {
        return;
    }

    float4 center = gFilterInput[pixel];
    float4 guide = gGuide[pixel];

    float4 result = center;
    if (guide.w > 0.0f)
    {
        // 分散を3x3でぼかして、輝度の重みの基準にする
        float varianceBlur = 0.0f;
        float varianceWeight = 0.0f;
        for (int vy = -1; vy <= 1; ++vy)
        {
            for (int vx = -1; vx <= 1; ++vx)
            {
                int2 p = int2(pixel) + int2(vx, vy);
                if (!InBounds(p, size))
                {
                    continue;
                }
                float w = (vx == 0 ? 0.5f : 0.25f) * (vy == 0 ? 0.5f : 0.25f);
                varianceBlur += gFilterInput[p].a * w;
                varianceWeight += w;
            }
        }
        varianceBlur /= varianceWeight;

        float centerLum = Luminance(center.rgb);
        float lumDenominator = PHI_LUMINANCE * sqrt(max(varianceBlur, 0.0f)) + 1e-4f;
        float depthDenominator = max(guide.w * PHI_DEPTH, 1e-4f) * float(gStepSize);

        // B3スプラインの5x5カーネル
        const float kernel[3] = { 3.0f / 8.0f, 1.0f / 4.0f, 1.0f / 16.0f };
        float3 colorSum = float3(0.0f, 0.0f, 0.0f);
        float varianceSum = 0.0f;
        float weightSum = 0.0f;
        for (int y = -2; y <= 2; ++y)
        {
            for (int x = -2; x <= 2; ++x)
            {
                int2 p = int2(pixel) + int2(x, y) * int(gStepSize);
                if (!InBounds(p, size))
                {
                    continue;
                }
                float4 sampleGuide = gGuide[p];
                if (sampleGuide.w <= 0.0f)
                {
                    continue;
                }
                float4 sampleColor = gFilterInput[p];

                float wNormal = pow(saturate(dot(guide.xyz, sampleGuide.xyz)), PHI_NORMAL);
                float wDepth = exp(-abs(guide.w - sampleGuide.w) / depthDenominator);
                float wLum = exp(-abs(centerLum - Luminance(sampleColor.rgb)) / lumDenominator);
                float w = kernel[abs(x)] * kernel[abs(y)] * wNormal * wDepth * wLum;

                colorSum += sampleColor.rgb * w;
                varianceSum += sampleColor.a * w * w;
                weightSum += w;
            }
        }
        if (weightSum > 1e-6f)
        {
            result = float4(colorSum / weightSum, varianceSum / (weightSum * weightSum));
        }
    }

    if (gIsFinal != 0)
    {
        // アルベドを掛け戻して最終出力にする
        gOutput[pixel] = float4(result.rgb * DemodulateAlbedo(gAlbedo[pixel].rgb), 1.0f);
    }
    else
    {
        gFilterOutput[pixel] = result;
    }
}
