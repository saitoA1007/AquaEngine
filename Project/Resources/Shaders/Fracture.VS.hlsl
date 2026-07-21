#include "Fracture.hlsli"

struct FractureForGPU
{
    float4x4 World;
    
    uint vertexOffset; // PackedGeometryBuffer内でのチャンクの頂点開始位置
    uint indexOffset; // PackedGeometryBuffer内でのチャンクのインデックス開始位置
    uint indexCount; // IndexCountPerInstance に相当
    uint chunkId; // シェーダー側で gParticle を引くためのID
};
StructuredBuffer<FractureForGPU> gFracture : register(t0);

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
};
ConstantBuffer<Camera> gCamera : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    
    VertexShaderOutput output;
    
    float4 worldPos = mul(input.position, gFracture[instanceId].World);
    output.position = mul(worldPos, gCamera.vpMatrix);
    output.texcoord = input.texcoord;
    output.color = float4(1.0f,1.0f,1.0f,1.0f);
    return output;
}