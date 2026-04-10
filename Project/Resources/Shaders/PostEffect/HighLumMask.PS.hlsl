#include"FullScreen.hlsli"
#include"../ColorConvert.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
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
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float32_t4 color = textureColor;
    
    float32_t3 texColorHSV = RGBToHSV(textureColor.rgb);
    
    // HSV上の輝度を使って輝度が低いと描画しないようにする
    if (texColorHSV.z < gBloomParameter.highLumMask)
    {
        color = float32_t4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
   // float32_t luminance = dot(float32_t3(0.299f, 0.587f, 0.114f), color.xyz);
   // // 光度が低いと描画しないようにする
   // if (luminance < gBloomParameter.highLumMask)
   // {
   //     color = float32_t4(0.0f, 0.0f, 0.0f, 0.0f);
   // }
    
    return color;
}