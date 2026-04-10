#include"FullScreen.hlsli"
#include"GaussianBlur.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t4> gTexHighLum : register(t1);

SamplerState gSampler : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    
    float32_t w, h, levels;
    gTexture.GetDimensions(0, w, h, levels);
    float32_t dx = 1.0f / w;
    float32_t dy = 1.0f / h;
    
    return Get3x3GaussianBlur(gTexHighLum, gSampler, input.texcoord, dx, dy, float32_t4(0, 0, 1, 1));
}