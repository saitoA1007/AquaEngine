#include"../FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint textureHandle;
    float time;
    float2 padding;
};
ConstantBuffer<Material> gMaterial : register(b0);

float Rand2dTo1d(float2 value, float2 dotDir = float2(12.9898, 78.233))
{
    float2 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    random = frac(sin(random) * 143758.5453);
    return random;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture[gMaterial.textureHandle].Sample(gSampler, input.texcoord);
    
    // 乱数生成
    float random = Rand2dTo1d(input.texcoord * gMaterial.time);
    // 色
    output.color = float4(random, random, random, 1.0f);
   
    return output;
}