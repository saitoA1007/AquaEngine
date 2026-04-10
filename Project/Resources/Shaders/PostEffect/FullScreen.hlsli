struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

float LinearizeDepth(float z)
{
    float nearZ = 0.1;
    float farZ = 100.0;

    return (2.0 * nearZ) / (farZ + nearZ - z * (farZ - nearZ));
}