#include "ParticleCS.hlsli"
#include "../RandomUtils.hlsli"

static const uint kMaxParticles = 1024;
RWStructuredBuffer<ParticleCS> gParticle : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
ConstantBuffer<EmitterSphere> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{   
    if (gEmitter.emit != 0)
    {
        RandomGenerator generator;
        generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;
        for (uint countIndex = 0; countIndex < gEmitter.count; ++countIndex)
        {  
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
            if (0 <= freeListIndex && freeListIndex < kMaxParticles)
            {
                uint particleIndex = gFreeList[freeListIndex];
                gParticle[particleIndex] = (ParticleCS) 0;
                //gFreeList[particleIndex] = particleIndex;
                
                    // カウント分パーティクルを射出する
                gParticle[particleIndex].scale = float3(0.1f,0.1f,0.1f);
                gParticle[particleIndex].translate = generator.Generate3d();
                gParticle[particleIndex].color.rgb = generator.Generate3d();
                gParticle[particleIndex].color.a = 1.0f;
                
                gParticle[particleIndex].velocity = generator.Generate3d();
                
                gParticle[particleIndex].lifeTime = 1.0f;
                gParticle[particleIndex].currentTime = 0.0f;
            }
            else
            {
                InterlockedAdd(gFreeListIndex[0], 1);
                break;
            }
        }
    }
}