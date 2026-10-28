
struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t3 worldPos : TEXCOORD0;
};

 // グリッドの間隔
static const float32_t kGridSpacing = 1.0f;

// 原点線の太さ
static const float32_t kOriginLineWidth = 3.0f; 
// 通常線の太さ
static const float32_t kNormalLineWidth = 2.0f; 