#include "FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint textureHandle;
    float exposure; // 露出
    float2 pad;
};
ConstantBuffer<Material> gMaterial : register(b0);

// Khronos PBR Neutralのトーンマッピング
float3 PBRNeutralToneMapping(float3 color)
{
    const float startCompression = 0.8f - 0.04f;
    const float desaturation = 0.15f;

    // 暗部の持ち上がりを抑えるオフセット
    float x = min(color.r, min(color.g, color.b));
    float offset = (x < 0.08f) ? x - 6.25f * x * x : 0.04f;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression)
    {
        return color;
    }

    // 最大成分を1.0に収まるように圧縮する
    const float d = 1.0f - startCompression;
    float newPeak = 1.0f - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    // 強く圧縮された所ほど白に寄せる
    float g = 1.0f - 1.0f / (desaturation * (peak - newPeak) + 1.0f);
    return lerp(color, newPeak.xxx, g);
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 textureColor = gTexture[gMaterial.textureHandle].Sample(gSampler, input.texcoord);

    // 負の値やNaNが混ざっても破綻しないようにする
    float3 hdr = max(textureColor.rgb, 0.0f) * exp2(gMaterial.exposure);

    return float4(PBRNeutralToneMapping(hdr), textureColor.a);
}
