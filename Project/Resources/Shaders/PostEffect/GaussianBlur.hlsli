
float32_t Gaussian(float32_t x, float32_t sigma)
{
    return exp(-(x * x) / (2.0f * sigma * sigma));
}

float32_t4 Get3x3GaussianBlur(Texture2D<float32_t4> tex, SamplerState smp, float32_t2 uv, float32_t dx, float32_t dy, float32_t4 rect)
{
    float32_t4 ret = tex.Sample(smp, uv);
    float32_t4 blurColor = float32_t4(0, 0, 0, 0);

    float32_t weights[3][3] =
    {
        { 1 / 16.0f, 2 / 16.0f, 1 / 16.0f },
        { 2 / 16.0f, 4 / 16.0f, 2 / 16.0f },
        { 1 / 16.0f, 2 / 16.0f, 1 / 16.0f }
    };

    float32_t offsets[3] = { -1.0f, 0.0f, 1.0f };

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            float32_t2 offset = float32_t2(offsets[i] * dx, offsets[j] * dy);
            float32_t2 sampleUV = uv + offset;

            sampleUV.x = clamp(sampleUV.x, rect.x + dx * 0.5f, rect.z - dx * 0.5f);
            sampleUV.y = clamp(sampleUV.y, rect.y + dy * 0.5f, rect.w - dy * 0.5f);

            blurColor += tex.Sample(smp, sampleUV) * weights[i][j];
        }
    }

    return float32_t4(blurColor.rgb, ret.a);
}

static const float32_t PI = 3.14159265f;

float gauss(float x, float y, float sigma)
{
    float exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}