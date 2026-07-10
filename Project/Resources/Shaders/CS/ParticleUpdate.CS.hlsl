#include "ParticleCS.hlsli"

static const uint kMaxParticles = 1024;
RWStructuredBuffer<ParticleCS> gParticle : register(u0);
ConstantBuffer<PerFrame> gPerFrame : register(b0);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int particleIndex = DTid.x;
            
    if (particleIndex < kMaxParticles)
    {
        if (gParticle[particleIndex].color.a != 0)
        {
            gParticle[particleIndex].translate += gParticle[particleIndex].velocity;
            gParticle[particleIndex].currentTime += gPerFrame.deltaTime;
            float alpha = 1.0f - (gParticle[particleIndex].currentTime / gParticle[particleIndex].lifeTime);
            gParticle[particleIndex].color.a = saturate(alpha);

        }
    }
}