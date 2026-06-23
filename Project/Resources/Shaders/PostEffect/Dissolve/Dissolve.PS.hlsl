#include"../FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint textureHandle;
    uint dissolveTextureHandle;
    float threshold;
    float padding;
};
ConstantBuffer<Material> gMaterial : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture[gMaterial.textureHandle].Sample(gSampler, input.texcoord);
    
    float mask = gTexture[gMaterial.dissolveTextureHandle].Sample(gSampler, input.texcoord).r;
    // edge
    float edge = 1.0f - smoothstep(0.5f, 0.53f, mask);
    output.color.rgb += edge * float3(1.0f, 0.4f, 0.3f);
    if (mask < gMaterial.threshold)
    {
        // マスク部分は黒色
        output.color = float4(0, 0, 0, 1);

    }  
    return output;
}