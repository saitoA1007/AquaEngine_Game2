
struct PSAccumIn
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    float clipDepth : TEXCOORD1;
    uint textureHandle : TEXCOORD2;
};

struct PSAccumOut
{
    float4 accumulation : SV_TARGET0; // weighted color + weighted alpha
    float revealage : SV_TARGET1; // transmittance product
};