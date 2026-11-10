float4 taylorInvSqrt(float4 r)
{
    return 1.79284291400159 - r * 0.85373472095314;
}

int mod289(int x) { return (x % 289 + 289) % 289; }
int3 mod289(int3 v) { return int3(mod289(v.x), mod289(v.y), mod289(v.z)); }
int4 mod289(int4 v) { return int4(mod289(v.x), mod289(v.y), mod289(v.z), mod289(v.w)); }
int4 permute(int4 v) 
{
	return mod289((v * 34 + 10) * v);
}

float4 SimplexNoiseGrad(float3 v, float4 seed = float4(0, 0, 0, 0))
{
    const float2 C = float2(1.0 / 6.0, 1.0 / 3.0);
    const float4 D = float4(0.0, 0.5, 1.0, 2.0);

    // First corner
    float3 fi  = floor(v + dot(v, C.yyy));
	int3 i = int3(fi);
    float3 x0 = v - fi + dot(fi, C.xxx);

    // Other corners
    float3 g  = step(x0.yzx, x0.xyz);
    float3 l  = 1.0 - g;
    float3 f1 = min(g.xyz, l.zxy);
    float3 f2 = max(g.xyz, l.zxy);
	int3 i1 = int3(f1);
	int3 i2 = int3(f2);

    //   x0 = x0 - 0.0 + 0.0 * C.xxx;
    //   x1 = x0 - i1  + 1.0 * C.xxx;
    //   x2 = x0 - i2  + 2.0 * C.xxx;
    //   x3 = x0 - 1.0 + 3.0 * C.xxx;
    float3 x1 = x0 - f1 + C.xxx;
    float3 x2 = x0 - f2 + C.yyy; // 2.0*C.x = 1/3 = C.y
    float3 x3 = x0 - D.yyy; // -1.0+3.0*C.x = -0.5 = -D.y

    // https://github.com/ashima/webgl-noise/issues/9
    // Permutations
    //i       = mod289(i);
	int3 iSeed = int3(seed.xyz);
    int4 ip = permute(permute(permute(
                int4(0, i1.z, i2.z, 1) + i.z + iSeed.z) +
            int4(0, i1.y, i2.y, 1) + i.y + iSeed.y) +
        int4(0, i1.x, i2.x, 1) + i.x + iSeed.x);
	float4 p = float4(ip);

    // Gradients: 7x7 points over a square, mapped onto an octahedron.
    // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float n_  = 0.142857142857; // 1.0/7.0
    float3 ns = n_ * D.wyz - D.xzx;

    float4 j = p - 49.0 * floor(p * ns.z * ns.z); //  mod(p,7*7)

    float4 x_ = floor(j * ns.z);
    float4 y_ = floor(j - 7.0 * x_); // mod(j,N)

    float4 x = x_ * ns.x + ns.yyyy;
    float4 y = y_ * ns.x + ns.yyyy;
    float4 h = 1.0 - abs(x) - abs(y);

    float4 b0 = float4(x.xy, y.xy);
    float4 b1 = float4(x.zw, y.zw);

    //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
    //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
    float4 s0 = floor(b0) * 2.0 + 1.0;
    float4 s1 = floor(b1) * 2.0 + 1.0;
    float4 sh = -step(h, float4(0.0, 0.0, 0.0, 0.0));

    float4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    float4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    float3 p0 = float3(a0.xy, h.x);
    float3 p1 = float3(a0.zw, h.y);
    float3 p2 = float3(a1.xy, h.z);
    float3 p3 = float3(a1.zw, h.w);

    //Normalise gradients
    float4 norm = taylorInvSqrt(float4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    float4 m     = max(0.5 - float4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    float4 m2    = m * m;
    float4 m4    = m2 * m2;
    float4 pdotx = float4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3));

    // Determine noise gradient
    float4 temp     = m2 * m * pdotx;
    float3 gradient = -8.0 * (temp.x * x0 + temp.y * x1 + temp.z * x2 + temp.w * x3);
    gradient += m4.x * p0 + m4.y * p1 + m4.z * p2 + m4.w * p3;

    return 105.0 * float4(gradient, dot(m4, pdotx));
}

float SimplexNoise(float3 v, float4 seed = float4(0, 0, 0, 0))
{
    return SimplexNoiseGrad(v, seed).w;
}

float SimplexNoise01(float3 v, float4 seed = float4(0, 0, 0, 0))
{
    return SimplexNoise(v, seed) * 0.5 + 0.5;
}

float FBMSum(int octaves)
{
	float sum = 0.0;
	float amp = 1.0;
	for (int i = 0; i < octaves; i++)
	{
		sum += amp;
		amp *= 0.5;
	}
	return sum;
}

float SimplexFBM(float3 v, int octaves = 3, float4 seed = float4(0, 0, 0, 0))
{
	float noise = 0.0;
	float amp = 1.0 / FBMSum(octaves);
	float freq = 1.0;
	for (int i = 0; i < octaves; i++)
	{
		noise += amp * SimplexNoise(v * freq, seed);
		freq *= 2.0;
		amp *= 0.5;
		seed += float4(1, 1, 1, 1);
	}
	return noise;
}
