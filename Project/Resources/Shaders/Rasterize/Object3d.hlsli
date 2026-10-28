
struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD1;
    float32_t3 normal : NORMAL1;
    float32_t3 worldPosition : POSITION1;
};

struct Skinned
{
    float32_t4 position;
    float32_t3 normal;
};