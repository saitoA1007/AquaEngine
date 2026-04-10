#include"FullScreen.hlsli"
#include"GaussianBlur.hlsli"

Texture2D<float32_t4> gTexHighLum : register(t1);
Texture2D<float32_t4> gTexShrinkHighLum : register(t2);
SamplerState gSampler : register(s0);

struct BloomParameter
{
    float32_t highLumMask; // 明るさの範囲
    float32_t sigma; // ぼかしの強さ
    int32_t bloomIteration;
    float32_t intensity;
};
ConstantBuffer<BloomParameter> gBloomParameter : register(b0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float32_t w, h, levels;
    gTexHighLum.GetDimensions(0, w, h, levels);

    float32_t dx = 1.0f / w;
    float32_t dy = 1.0f / h;

    float32_t4 bloomAccum = float32_t4(0, 0, 0, 0);
    float32_t2 uvSize = float32_t2(0.5, 0.5);
    float32_t2 uvOffset = float32_t2(0, 0);

    for (int i = 0; i < gBloomParameter.bloomIteration; ++i)
    {
        bloomAccum += Get3x3GaussianBlur(gTexShrinkHighLum, gSampler, input.texcoord * uvSize + uvOffset, dx, dy, float32_t4(uvOffset, uvOffset + uvSize));
        uvOffset.y += uvSize.y;
        uvSize *= 0.5f;
    }

    float32_t4 bloomColor = Get3x3GaussianBlur(gTexHighLum, gSampler, input.texcoord, dx, dy, float32_t4(0, 0, 1, 1)) + saturate(bloomAccum);
    return bloomColor;
}

