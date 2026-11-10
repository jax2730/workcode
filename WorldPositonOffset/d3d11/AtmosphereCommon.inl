// according to comment of unreal shader
// value of this parameter should match one in FAtmosphereSetUp::ComputeViewData func in Unreal's .cpp file
// but unreal uses 0.005(5 meters) inside their .cpp file and 0.001(1 meter) in their shader
// here we just follow their code and try 0.001 
#define PLANET_RADIUS_OFFSET 0.001f
#define PreExposure 2.20682
#define PLANET_RADIUS_RATIO_SAFE_EDGE 1.00000155763f

// medium sampling code from unreal
// some referrence about medium rendering:
// https://www.pbr-book.org/4ed/Volume_Scattering/Volume_Scattering_Processes
float3 GetAlbedo(float3 scattering, float3 extinction)
{
    return scattering / max(1e-3f, extinction);
}

// Follows PBRT convention https://www.pbr-book.org/4ed/Volume_Scattering/Phase_Functions
float HenyeyGreensteinPhase(float g, float cosTheta)
{
	float denom = 1.f + g * g + 2.f * g * cosTheta;
	return (1.f - g * g) / (4.f * PI * denom * sqrt(denom));
}

float RayleighPhase(float cosTheta)
{
	float factor = 3.f / (16.f * PI);
	return factor * (1.f + cosTheta * cosTheta);
}

// near intersection in x, far intersection in y, both -1 if no interseciton
float2 RayIntersectSphere(float3 rayOrigin, float3 rayDirection, float4 sphere)
{
    float3 localPosition = rayOrigin - sphere.xyz;
    float localPositionSqr = dot(localPosition, localPosition);

    float A = dot(rayDirection, rayDirection);
    float B = 2.f * dot(rayDirection, localPosition); // why positive
    float C = localPositionSqr - sphere.w * sphere.w;

    float discriminant = B * B - 4 * A * C;
    float2 intersections = -1.f;

    [flatten]
    if(discriminant >= 0)
    {
        float sqrtDiscriminant = sqrt(discriminant);
        intersections = (-B + float2(-1.f, 1.f) * sqrtDiscriminant) / (2.f * A);
    }
    return intersections;
}

float RaySphereIntersectNearest(float3 rayOrigin, float3 rayDir, float3 sphereCenter, float sphereRadius)
{
    float2 sol = RayIntersectSphere(rayOrigin, rayDir, float4(sphereCenter, sphereRadius));

    // origin outside sphere, no intersection
    if(sol.x < 0.f && sol.y < 0.f)
    {
        return -1.f;
    }

    // origin inside sphere, one intersection
    if(sol.x < 0.f)
    {
        return max(0.f, sol.y);
    }
    else if(sol.y < 0.f)
    {
        return max(0.f, sol.x);
    }

    // origin outside sphere, two intersection
    return max(0.f, min(sol.x, sol.y));
}

float3x3 GetSkyViewReferential(in float4x4 viewSkyViewLutReferential)
{
    float3x3 skyViewLutReferential = (float3x3)viewSkyViewLutReferential;
    return skyViewLutReferential;
}

// mapping from [0, 1] to [0, uv] unit range
float2 FromUnitToSubUvs(float2 uv, float4 sizeAndInvSize)
{
    return (uv + 0.5f * sizeAndInvSize.zw) * (sizeAndInvSize.xy / (sizeAndInvSize.xy + 1.f));
}

// mapping from [0, uv] to [0, 1] uint range
float2 FromSubUvsToUnit(float2 uv, float4 sizeAndInvSize)
{
    return (uv - 0.5f * sizeAndInvSize.zw) *(sizeAndInvSize.xy / (sizeAndInvSize.xy - 1.f));
}

void SkyViewLutParamsToUv(
    in bool intersectGround,
    in float viewZenithCosAngle,
    in float3 viewDir,
    in float viewHeight,
    in float bottomRadius,
    in float4 skyViewLutSizeAndInvSize,
    out float2 uv
){
    float vHorizon = sqrt(viewHeight * viewHeight - bottomRadius * bottomRadius);
    float cosBeta = vHorizon / viewHeight;
    float beta = acosFast4(cosBeta);
    float zenithHorizonAngle = PI - beta;
    float viewZenithAngle = acosFast4(viewZenithCosAngle);
    if(!intersectGround)
    {
        float coord = viewZenithAngle / zenithHorizonAngle;
        coord = 1.f - coord;
        coord = sqrt(coord);
        coord = 1.f - coord;
        uv.y = coord * 0.5f;
    }
    else
    {
        float coord = (viewZenithAngle - zenithHorizonAngle) / beta;
        coord = sqrt(coord);
        uv.y = coord * 0.5 + 0.5f;
    }
    {
        uv.x = (atan2Fast(-viewDir.y, -viewDir.x) + PI) / (2.f * PI);
    }
    uv = FromUnitToSubUvs(uv, skyViewLutSizeAndInvSize);
}

// mapping from Bruneton 2017
// reference to GetTransmittanceTextureUvFromRMu func in 
// https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/functions.glsl.html
void FromTransmittanceLutUvs(
    out float viewHeight,
    out float viewZenithCosAngle,
    in float bottomRadius,
    in float topRadius,
    in float2 uv
){
    float xMu = uv.x;
    float xR = uv.y;
    
    float H = sqrt(topRadius * topRadius - bottomRadius * bottomRadius);
    float rho = H * xR;
    viewHeight = sqrt(rho * rho + bottomRadius * bottomRadius); 

    float dMin = topRadius - viewHeight;
    float dMax = rho + H;
    float D = dMin + xMu * (dMax - dMin);
    viewZenithCosAngle = D == 0.f ? 1.f : (H * H - rho * rho - D * D) / (2.f * viewHeight * D);
    viewZenithCosAngle = clamp(viewZenithCosAngle, -1.f, 1.f);
}

// reverse mapping of above
float2 GetTransmittanceLutUVs(
    in float viewHeight,
    in float viewZenithCosAngle,
    in float bottomRadius,
    in float topRadius
){
    float H = sqrt(max(0.f, topRadius * topRadius - bottomRadius * bottomRadius));
    float rho = sqrt(max(0.f, viewHeight * viewHeight - bottomRadius * bottomRadius));

    // compute distance to atmosphere top
    // reference to GetTransmittanceTextureUvFromRMu func in 
    // https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/functions.glsl.html
    float discriminant = viewHeight * viewHeight * (viewZenithCosAngle * viewZenithCosAngle - 1.f) + topRadius * topRadius;
    float d = max(0.0f, (-viewHeight * viewZenithCosAngle + sqrt(discriminant)));

    float dMin = topRadius - viewHeight;
    float dMax = rho + H;
    float xMu = (d - dMin) / (dMax - dMin);
    float xR = rho / H;

    float2 uv = float2(xMu, xR);
    return uv;
}



// from unreal Random.ush
// high frequency dither pattern appearing almost random without banding steps
// note: from "NEXT GENERATION POST PROCESSING IN CALL OF DUTY: ADVANCED WARFARE"
//      http://advances.realtimerendering.com/s2014/index.html
// Epic extended by FrameId
// ~7 ALU operations (2 frac, 3 mad, 2 *)
// @return 0..1
float InterleavedGradientNoise(float2 uv, float frameId)
{
    uv += frameId * (float2(47, 17) * 0.695f);
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(uv, magic.xy)));
}



bool MoveToTopAtmosphere(inout float3 worldPos, in float3 worldDir, in float atmosphereTopRadius)
{
    float viewHeight = length(worldPos);
    if(viewHeight > atmosphereTopRadius)
    {
        float tTop = RaySphereIntersectNearest(worldPos, worldDir, float3(0.f, 0.f, 0.f), atmosphereTopRadius);
        if(tTop >= 0.f)
        {
            float3 upVector = worldPos / viewHeight;
            float3 upOffset = upVector * -PLANET_RADIUS_OFFSET;
            worldPos = worldPos + worldDir * tTop + upOffset;
        }
        else{
            // ray is not intersecting the atmosphere
            return false;
        }
    }
    return true;
}

float3 GetAtmosphereTransmittance(
    float3 planetCenterToWorldPos,
    float3 worldDir,
    float bottomRadius,
    float topRadius,
    Texture2D<float4> transmittanceTex,
    SamplerState transmittanceSampler
){
    const float2 sol = RayIntersectSphere(planetCenterToWorldPos, worldDir, float4(float3(0.f, 0.f, 0.f), bottomRadius));
    if(sol.x > 0.f || sol.y > 0.f)return 0.f;

    const float pHeight = length(planetCenterToWorldPos);
    const float3 upVector = planetCenterToWorldPos / pHeight;
    const float lightZenithCosAngle = dot(worldDir, upVector);
    float2 transmittanceUV = GetTransmittanceLutUVs(pHeight, lightZenithCosAngle, bottomRadius, topRadius);
    const float3 transmittanceToLight = transmittanceTex.SampleLevel(transmittanceSampler, transmittanceUV, 0).rgb;
    return transmittanceToLight;
}

float3 GetLightDiskLuminance(
    float3 planetCenterToWorldPos,
    float3 worldDir,
    float bottomRadius,
    float topRadius,
    Texture2D<float4> transmittanceTex,
    SamplerState transmittanceSampler,
    float3 atmosphereLightDirection,
    float atmosphereLightDiscCosHalfApexAngle, // assuming a cone light
    float3 atmosphereLightDiscLuminance
){
    const float viewDotLight = dot(worldDir, atmosphereLightDirection);
    const float cosHalfApex = atmosphereLightDiscCosHalfApexAngle;
    if(viewDotLight > cosHalfApex)
    {
        const float3 transmittanceToLight = GetAtmosphereTransmittance(
            planetCenterToWorldPos,
            atmosphereLightDirection,
            bottomRadius,
            topRadius,
            transmittanceTex,
            transmittanceSampler
        );

        const float softEdge = saturate(2.f * (viewDotLight - cosHalfApex) / (1.f - cosHalfApex));
        return atmosphereLightDiscLuminance * transmittanceToLight * softEdge;
    }
    return 0.f;
}