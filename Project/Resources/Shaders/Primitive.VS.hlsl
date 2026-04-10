#include"Primitive.hlsli"

struct TransformationMatrix
{
    float32_t4x4 VP;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t4 color : COLOR;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.VP);
    output.color = input.color;
    return output;
}