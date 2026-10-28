#include "ParticleCS.hlsli"

static const uint kMaxParticles = 1024;
RWStructuredBuffer<ParticleCS> gParticle : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
ConstantBuffer<PerFrame> gPerFrame : register(b0);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int particleIndex = DTid.x;
            
    if (particleIndex < kMaxParticles)
    {
        if (gParticle[particleIndex].color.a > 0.0f)
        {
            gParticle[particleIndex].translate += gParticle[particleIndex].velocity * gPerFrame.deltaTime;
            gParticle[particleIndex].currentTime += gPerFrame.deltaTime;

            float alpha = 1.0f - (gParticle[particleIndex].currentTime / gParticle[particleIndex].lifeTime);
            gParticle[particleIndex].color.a = saturate(alpha);
            
            
             // 寿命が尽きた瞬間だけフリーリストへ返却する
            if (gParticle[particleIndex].currentTime >= gParticle[particleIndex].lifeTime)
            {
                gParticle[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
                gParticle[particleIndex].color.a = 0.0f; 

                int freeListIndex;
                InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);

                if ((freeListIndex + 1) < kMaxParticles)
                {
                    gFreeList[freeListIndex + 1] = particleIndex;
                }
                else
                {
                    // オーバーフロー安全策
                    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
                }
            }
        }
    }
}