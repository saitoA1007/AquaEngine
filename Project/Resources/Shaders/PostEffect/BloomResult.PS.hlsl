#include"FullScreen.hlsli"
#include"GaussianBlur.hlsli"

Texture2D<float4> gTexHighLum : register(t1);
Texture2D<float4> gTexShrinkHighLum : register(t2);
SamplerState gSampler : register(s0);

struct BloomParameter
{
    float highLumMask; // 明るさの範囲
    float sigma; // ぼかしの強さ
    int bloomIteration;
    float intensity;
};
ConstantBuffer<BloomParameter> gBloomParameter : register(b0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float w, h, levels;
    gTexHighLum.GetDimensions(0, w, h, levels);

    float dx = 1.0f / w;
    float dy = 1.0f / h;

    float4 bloomAccum = float4(0, 0, 0, 0);
    float2 uvSize = float2(0.5, 0.5);
    float2 uvOffset = float2(0, 0);

    for (int i = 0; i < gBloomParameter.bloomIteration; ++i)
    {
        bloomAccum += Get3x3GaussianBlur(gTexShrinkHighLum, gSampler, input.texcoord * uvSize + uvOffset, dx, dy, float4(uvOffset, uvOffset + uvSize));
        uvOffset.y += uvSize.y;
        uvSize *= 0.5f;
    }

    float4 bloomColor = Get3x3GaussianBlur(gTexHighLum, gSampler, input.texcoord, dx, dy, float4(0, 0, 1, 1)) + saturate(bloomAccum);
    return bloomColor;
}

