#include"ParticleCS.hlsli"

static const uint kMaxParticles = 1024;
RWStructuredBuffer<ParticleCS> gParticle : register(u0);
[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        // Particle構造体の全要素を0で埋める
        gParticle[particleIndex] = (ParticleCS) 0;
    }
    
    gParticle[particleIndex] = (ParticleCS) 0;
    gParticle[particleIndex].scale = float3(0.5f, 0.5f, 0.5f);
    gParticle[particleIndex].color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}