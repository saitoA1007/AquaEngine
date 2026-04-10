#include"ParticleCS.hlsli"

static const uin32_t kMaxParticles = 1024;
RWStructuredBuffer<Particle> gParticle : register(u0);
[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint32_t particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        // Particle構造体の全要素を0で埋める
        gParticle[particleIndex] = (Particle) 0;
    }
    
    gParticle[particleIndex] = (Particle) 0;
    gParticle[particleIndex].scale = float32_t3(0.5f, 0.5f, 0.5f);
    gParticle[particleIndex].color = float32_t3(1.0f, 1.0f, 1.0f, 1.0f);
}