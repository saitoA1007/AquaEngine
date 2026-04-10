#include"../FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float> gDepth : register(t1);
SamplerState gSampler : register(s0);

struct Material
{
    float32_t2 center; // 中心点
    int32_t numSamles; // サンプリング処理。大きい程滑らか
    float32_t blurWidth;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
   
    PixelShaderOutput output;
    
    float2 uv = input.texcoord;
   
    // 深度値 [0,1] を取得
    float depth = gDepth.Sample(gSampler, uv);

    // 例: 深度をグレースケールにして可視化
    output.color = float4(depth, depth, depth, 1.0f);
    
    return output;
}

