#include"../FullScreen.hlsli"

Texture2D<float32_t4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct GrayscaleData
{
    uint textureHandle;
};
ConstantBuffer<GrayscaleData> gGrayscaleData : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture[gGrayscaleData.textureHandle].Sample(gSampler, input.texcoord);
    // グレースケール化
    float value = dot(output.color.rgb, float3(0.2125f, 0.7154f, 0.0721f));
    output.color.rgb = float3(value, value, value);
    return output;
}