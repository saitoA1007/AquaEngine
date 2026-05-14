#include "FullScreen.hlsli"

Texture2D<float4> gRasterColor : register(t0);
Texture2D<float> gRasterDepth : register(t1);
Texture2D<float4> gRtColor : register(t2);
Texture2D<float> gRtDepth : register(t3);

SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float LinearizeDepth(float depth, float nearZ, float farZ)
{
    return nearZ * farZ / (farZ - depth * (farZ - nearZ));
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    float nearZ = 0.1f;
    float farZ = 200.0f;
    
    float rasterDepth = gRasterDepth.Sample(gSampler, input.texcoord).r;
    float rtDepth = gRtDepth.Sample(gSampler, input.texcoord).r;
    
    float linearRtDepth = LinearizeDepth(rtDepth, nearZ, farZ);
    linearRtDepth /= 100.0f;
    
    float linearRasterDepth = LinearizeDepth(rasterDepth, nearZ, farZ);
    linearRasterDepth /= 100.0f;
    
    // 深度値を比較して手前側を描画する
    if (linearRasterDepth < linearRtDepth)
    {
        output.color = gRasterColor.Sample(gSampler, input.texcoord);
    }
    else
    {
        output.color = gRtColor.Sample(gSampler, input.texcoord);
    }
    return output;
}