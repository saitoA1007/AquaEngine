#include"Particle.hlsli"

struct ParticleForGPU
{
    float32_t4x4 World;
    float32_t4 color;
    uint32_t textureHandle;
    float32_t3 padding;
};
StructuredBuffer<ParticleForGPU> gParticle : register(t0);

struct Camera
{
    float32_t3 worldPosition;
    float32_t4x4 vpMatrix;
};
ConstantBuffer<Camera> gCamera : register(b0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    float32_t4 worldPos = mul(input.position, gParticle[instanceId].World);
    output.position = mul(worldPos, gCamera.vpMatrix);
    output.texcoord = input.texcoord;
    output.color = gParticle[instanceId].color;
    output.textureHandle = gParticle[instanceId].textureHandle;
    return output;
}