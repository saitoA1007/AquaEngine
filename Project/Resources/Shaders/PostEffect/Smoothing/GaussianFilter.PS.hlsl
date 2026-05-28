#include"../FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint textureHandle;   
    float sd; // 標準偏差
};
ConstantBuffer<Material> gMaterial : register(b0);

static const float PI = 3.14159265f;

static const float2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
};

float Gauss(float x, float y, float sigma)
{
    float exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    uint width, height;
    gTexture[gMaterial.textureHandle].GetDimensions(width, height);
    float2 uvStepSize = float2(rcp(width), rcp(height));
    
    // kenrnelを求める
    float weight = 0.0f;
    float kernel3x3[3][3];
    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            kernel3x3[x][y] = Gauss(kIndex3x3[x][y].x, kIndex3x3[x][y].y, gMaterial.sd);
            weight += kernel3x3[x][y];
        }

    }
    
    PixelShaderOutput output;
    output.color.rgb = float3(0, 0, 0);
    output.color.a = 1.0f;
    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            float3 fetchColor = gTexture[gMaterial.textureHandle].Sample(gSampler, texcoord).rgb;
            output.color.rgb += fetchColor * kernel3x3[x][y];
        }

    }
    output.color.rgb *= rcp(weight);
    return output;
}