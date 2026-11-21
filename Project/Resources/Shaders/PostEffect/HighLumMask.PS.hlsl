#include "FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint textureHandle;
    float highLumMask; // 明るいさの範囲
    float softKnee;    // 閾値付近をなめらかにする幅
    float pad;
};
ConstantBuffer<Material> gMaterial : register(b0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 textureColor = gTexture[gMaterial.textureHandle].Sample(gSampler, input.texcoord);
    float3 color = max(textureColor.rgb, 0.0f);

    // 色の最大成分を明るさとする
    float brightness = max(color.r, max(color.g, color.b));

    float threshold = gMaterial.highLumMask;
    float knee = threshold * gMaterial.softKnee;

    // なめらかに補間する
    float soft = clamp(brightness - threshold + knee, 0.0f, 2.0f * knee);
    soft = soft * soft / (4.0f * knee + 1e-5f);

    // 閾値を超えた分だけを残す
    float contribution = max(soft, brightness - threshold) / max(brightness, 1e-5f);

    return float4(color * contribution, textureColor.a);
}
