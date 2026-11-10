#define PI 3.1415926535897932

float Square(float x)
{
    return x * x;
}

float2 Square(float2 x)
{
    return x * x;
}

float3 Square(float3 x)
{
    return x * x;
}

float4 Square(float4 x)
{
    return x * x;
}

float Min2(float2 x)
{
    return min(x.x, x.y);
}

float Min3(float3 x)
{
    return min(x.x, min(x.y, x.z));
}

float Min4(float4 x)
{
    return min(x.x, min(x.y, min(x.z, x.w)));
}

float Max2(float2 x)
{
    return max(x.x, x.y);
}

float Max3(float3 x)
{
    return max(x.x, max(x.y, x.z));
}

float Max4(float4 x)
{
    return max(x.x, max(x.y, max(x.z, x.w)));
}

float Pow2(float x)
{
    return x * x;
}

float2 Pow2(float2 x)
{
    return x * x;
}

float3 Pow2(float3 x)
{
    return x * x;
}

float4 Pow2(float4 x)
{
    return x * x;
}

float Pow3(float x)
{
    return x * x * x;
}

float2 Pow3(float2 x)
{
    return x * x * x;
}

float3 Pow3(float3 x)
{
    return x * x * x;
}

float4 Pow3(float4 x)
{
    return x * x * x;
}

float Pow4(float x)
{
    float xx = x * x;
    return xx * xx;
}

float2 Pow4(float2 x)
{
    float2 xx = x * x;
    return xx * xx;
}

float3 Pow4(float3 x)
{
    float3 xx = x * x;
    return xx * xx;
}

float4 Pow4(float4 x)
{
    float4 xx = x * x;
    return xx * xx;
}

float Pow5(float x)
{
    float xx = x * x;
    return xx * xx * x;
}

float2 Pow5(float2 x)
{
    float2 xx = x * x;
    return xx * xx * x;
}

float3 Pow5(float3 x)
{
    float3 xx = x * x;
    return xx * xx * x;
}

float4 Pow5(float4 x)
{
    float4 xx = x * x;
    return xx * xx * x;
}

float Pow6(float x)
{
    float xx = x * x;
    return xx * xx * xx;
}

float2 Pow6(float2 x)
{
    float2 xx = x * x;
    return xx * xx * xx;
}

float3 Pow6(float3 x)
{
    float3 xx = x * x;
    return xx * xx * xx;
}

float4 Pow6(float4 x)
{
    float4 xx = x * x;
    return xx * xx * xx;
}

float2 Mod289(float2 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float3 Mod289(float3 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float4 Mod289(float4 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float4 QuaternionMultiply(float4 q1, float4 q2)
{
    return float4(q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz), q1.w * q2.w - dot(q1.xyz, q2.xyz));
}

float3 QuaternionRotate(float4 q, float3 v)
{
    float3 b = q.xyz;
    float b2 = dot(b, b);
    return (v * (q.w * q.w - b2) + b * (dot(v, b) * 2.f) + cross(b, v) * (q.w * 2.f));
}

float4 QuaternionFromAxisAngle(float3 axis, float angle)
{
    return float4(axis * sin(angle * 0.5f), cos(angle * 0.5f));
}

float Luminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

// "Next Generation Post Processing in Call of Duty: Advanced Warfare"
// http://advances.realtimerendering.com/s2014/index.html
float InterleavedGradientNoise(float2 uv, uint frameCount)
{
    const float2 magicFrameScale = float2(47, 17) * 0.695;
    uv += frameCount * magicFrameScale;
    const float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(magic.z * frac(dot(uv, magic.xy)));
}

// Removes error from the color to properly store it in lower precision formats (error = 2^(-mantissaBits))
float3 QuantizeColor(float3 color, float noise, float3 error)
{
    float3 delta = color * error;
    delta.x = asfloat(asuint(delta.x) & ~0x007fffff);
    delta.y = asfloat(asuint(delta.y) & ~0x007fffff);
    delta.z = asfloat(asuint(delta.z) & ~0x007fffff);
    return color + delta * noise;
}
