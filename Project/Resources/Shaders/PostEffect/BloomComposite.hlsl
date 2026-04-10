#include"FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t4> gTexBloomResult : register(t3);

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
    float32_t4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float32_t4 result = texColor;
	
    float32_t4 bloomColor = gTexBloomResult.Sample(gSampler, input.texcoord);
    result += bloomColor * gBloomParameter.intensity;

    return result;
}