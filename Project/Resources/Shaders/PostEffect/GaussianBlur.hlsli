
float Gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0f * sigma * sigma));
}

float4 Get3x3GaussianBlur(Texture2D<float4> tex, SamplerState smp, float2 uv, float dx, float dy, float4 rect)
{
    float4 ret = tex.Sample(smp, uv);
    float4 blurColor = float4(0, 0, 0, 0);

    float weights[3][3] =
    {
        { 1 / 16.0f, 2 / 16.0f, 1 / 16.0f },
        { 2 / 16.0f, 4 / 16.0f, 2 / 16.0f },
        { 1 / 16.0f, 2 / 16.0f, 1 / 16.0f }
    };

    float offsets[3] = { -1.0f, 0.0f, 1.0f };

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            float2 offset = float2(offsets[i] * dx, offsets[j] * dy);
            float2 sampleUV = uv + offset;

            sampleUV.x = clamp(sampleUV.x, rect.x + dx * 0.5f, rect.z - dx * 0.5f);
            sampleUV.y = clamp(sampleUV.y, rect.y + dy * 0.5f, rect.w - dy * 0.5f);

            blurColor += tex.Sample(smp, sampleUV) * weights[i][j];
        }
    }

    return float4(blurColor.rgb, ret.a);
}

static const float PI = 3.14159265f;

float gauss(float x, float y, float sigma)
{
    float exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}