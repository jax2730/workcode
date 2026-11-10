// reference:
// https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/functions.glsl.html
//

#define M_PI 3.14159265358979323846
#define TRANSMITTANCE_TEXTURE_WIDTH 256
#define TRANSMITTANCE_TEXTURE_HEIGHT 64

#define SCATTERING_TEXTURE_R_SIZE 32
#define SCATTERING_TEXTURE_MU_SIZE 128
#define SCATTERING_TEXTURE_MU_S_SIZE 32
#define SCATTERING_TEXTURE_NU_SIZE 8

float RayleighPhase(float nu){
    float k = 3.f / (16.f * M_PI);
    return k * (1.f + nu * nu);
}

float MiePhase(float g, float nu){
    float k = 3.f / (8.f * M_PI) * (1.f - g * g) / (2.f + g * g);
    return k * (1.f + nu * nu) / pow(1.f + g * g - 2.f * g * nu, 1.5f); 
}

// sigma params
// ozone layer only has linearTerm and constantTerm
// rayleigh layer and mie layer only has expTerm and expScale
struct DensityProfileLayer{
    float width;            // only absorption profile will use this parameter for now
    float expTerm;          
    float expScale;         // expScale = -(1.f / heightScale)
    float linearTerm;
    float constantTerm;
};

struct DensityProfile{
    DensityProfileLayer layers[2]; 
};

struct AtmosphereParams{
    float rTop;
    float rBottom;
    float miePhaseG;
    float3 rayleighScattering;
    float3 mieScattering;
    float3 mieExtinction;
    float3 absorptionExtinction;
    DensityProfile rayleighDensity;
    DensityProfile mieDensity;
    DensityProfile absorptionDensity;
    float sunAngularRadius;
    float3 solarIrradiance;
    float musMin;
    float3 groundAlbedo;    //average albedo
};

AtmosphereParams initAtmosphereWithBoundary(
    float rTop,
    float rBottom)
{
    DensityProfileLayer defaultLayer;
    defaultLayer.width = 0.f;       
    defaultLayer.expTerm = 0.f;
    defaultLayer.expScale = 0.f;    
    defaultLayer.linearTerm = 0.f;
    defaultLayer.constantTerm = 0.f;

    DensityProfile defaultProfile;
    defaultProfile.layers[0] = defaultLayer;
    defaultProfile.layers[1] = defaultLayer;

    AtmosphereParams atmosphere;
    atmosphere.rTop = rTop;
    atmosphere.rBottom = rBottom;
    atmosphere.rayleighScattering = 0.f;
    atmosphere.mieScattering = 0.f;
    atmosphere.mieExtinction = 0.f;
    atmosphere.absorptionExtinction = 0.f;
    atmosphere.rayleighDensity = defaultProfile;
    atmosphere.mieDensity = defaultProfile;
    atmosphere.absorptionDensity = defaultProfile;
    atmosphere.sunAngularRadius = 0.f;
    atmosphere.musMin = 0.f;
    atmosphere.groundAlbedo = 0.f;

    return atmosphere;
}

AtmosphereParams initExponentialHeightFog(
    float rTop, 
    float rBottom, 
    float expScale,
    float expTerm)
{
    DensityProfileLayer defaultLayer;
    defaultLayer.width = 0.f;       
    defaultLayer.expTerm = 0.f;
    defaultLayer.expScale = 0.f;    
    defaultLayer.linearTerm = 0.f;
    defaultLayer.constantTerm = 0.f;

    DensityProfileLayer fogLayer;
    fogLayer.width = 0.f;
    fogLayer.expTerm = expTerm;
    fogLayer.expScale = expScale;
    fogLayer.linearTerm = 0.f;
    fogLayer.constantTerm = 0.f;

    DensityProfile fogProfile;
    fogProfile.layers[0] = fogLayer;
    fogProfile.layers[1] = fogLayer;

    DensityProfile defaultProfile;
    defaultProfile.layers[0] = defaultLayer;
    defaultProfile.layers[1] = defaultLayer;

    AtmosphereParams atmosphere;
    atmosphere.rTop = rTop;
    atmosphere.rBottom = rBottom;
    atmosphere.rayleighScattering = 1.f;    // use rayleigh 
    atmosphere.mieScattering = 0.f;
    atmosphere.mieExtinction = 0.f;
    atmosphere.absorptionExtinction = 0.f;
    atmosphere.rayleighDensity = fogProfile;
    atmosphere.mieDensity = defaultProfile;
    atmosphere.absorptionDensity = defaultProfile;
    atmosphere.sunAngularRadius = 0.f;
    atmosphere.musMin = 0.f;
    atmosphere.groundAlbedo = 0.f;

    return atmosphere;
}

// assume atmosphere is a perfect sphere with radius 'rTop'
// distance of ray traveling from current height 'r' to atmosphere boundary 'rtop' along given direction 'mu'
float DistanceToAtmosphereBoundary(
    in AtmosphereParams atmosphere,      // top boundery height of atmosphere
    in float r,                          // current point height from earth center
    in float mu
){
    if(r > atmosphere.rTop) return -1.f;
    if(mu < -1.f || mu > 1.f )return -1.f;
    float discriminant = max(r * r * (mu * mu - 1.f) + atmosphere.rTop * atmosphere.rTop, 1e-5f); // clamp to avoid NaN
    return max(0.f,  -r * mu + sqrt(discriminant));
}

// assume earth is a perfect sphere with radius 'rBottom'
// distance of ray traveling from current height 'r' to earth surface 'rBottom' along given direction 'mu' 
float DistanceToEarth(
    in AtmosphereParams atmosphere,
    in float r,
    in float mu
){
    if(r < atmosphere.rBottom)return -1.f;
    if(mu < -1.f || mu > 1.f )return -1.f;
    float discriminant = max(r * r * (mu * mu - 1.f) + atmosphere.rBottom * atmosphere.rBottom, 1e-5f); // clamp to avoid NaN
    return max(0.f, -r * mu - sqrt(discriminant));
}

bool RayHitsGround(
    in AtmosphereParams atmosphere,
    in float r,
    in float mu
){
    if(r < atmosphere.rBottom)return false;
    if(mu < -1.f || mu > 1.f )return false;
    return (mu < 0.f) && // ray is shooting downward 
        (r * r * (mu * mu - 1.f) + atmosphere.rBottom * atmosphere.rBottom > 0.f); // determinant > 0.f, distance to earth has a solution
}

float GetLayerDensity(
    in DensityProfileLayer layer,
    in float altitude
){
    float density = layer.expTerm * exp(layer.expScale * altitude) + 
        layer.linearTerm * altitude + layer.constantTerm;
    // float density = layer.expTerm * exp(layer.expScale * altitude) + 
    //     layer.linearTerm * altitude + layer.constantTerm;
    return density;
}

// float GetOzoneLayerDensity(
//     in DensityProfileLayer layer,
//     in float altitude
// ){
//     // tent function mensioned in unreal
//     // https://onlinelibrary.wiley.com/doi/10.1111/cgf.14050
//     float density = max(0.f, 1.f - (altitude - 25.f) / 15.f);
//     return density;
// }

// float GetOzoneProfileDensity(
//     in DensityProfile profile,
//     in float altitude
// ){
//     return altitude < profile.layers[0].width ?
//     GetOzoneLayerDensity(profile.layer[0], altitude) :
//     GetOzoneLayerDensity(profile.layer[1], altitude);
// }

// get atmosphere profile density at given altitude
float GetProfileDensity(
    in DensityProfile profile,
    in float altitude
){
      return altitude < profile.layers[0].width ?
      GetLayerDensity(profile.layers[0], altitude) :
      GetLayerDensity(profile.layers[1], altitude);
}

float ComputeOpticalLengthToTopAtmosphereBoundary(
    in AtmosphereParams atmosphere,
    in DensityProfile profile,
    in float r,
    in float mu
){
    const int SAMPLE_COUNT = 500;
    float sampleStep = DistanceToAtmosphereBoundary(atmosphere, r, mu) / SAMPLE_COUNT;
    float result = 0.f;
    
    // numerical compute integral
    for(int i = 0; i <= SAMPLE_COUNT; ++i)
    {
        float d_i = i * sampleStep;  // current sampled point(distance from p)
        
        // float offset = sampleStep;
        // const float4 D2 = 0.25f * float4(3.f, 1.f, 0.f, 2.f);
        // const float4 target = float4(1.f, 2.f, 3.f, 4.f);
        // float2 ij = floor(fmod(screenPosition, float2(2.f, 2.f)));

        // float idx = ij.x + 2.f * ij.y;
        // float4 m = step(abs(idx - target), 0.5f) * D2;
        // float d = m.x + m.y + m.z + m.w;  

        float r_i = sqrt(d_i * d_i + 2.f * r * mu * d_i + r * r); // absolute height from earth center
        float altitude = r_i - atmosphere.rBottom;
        float currentDensity = GetProfileDensity(profile, altitude);

        float weight = i == 0.f || i == SAMPLE_COUNT ? 0.5f : 1.0f;
        result += currentDensity * weight * sampleStep;
    }
    return result;
}

float3 ComputeTransmittanceToTopAtmosphereBoundary(
    in AtmosphereParams atmosphere,
    float r,
    float mu
){
    return exp(-(
        atmosphere.rayleighScattering *                             
            ComputeOpticalLengthToTopAtmosphereBoundary(
                atmosphere, atmosphere.rayleighDensity, r, mu) +
        atmosphere.mieExtinction * 
            ComputeOpticalLengthToTopAtmosphereBoundary(
                atmosphere, atmosphere.mieDensity, r, mu) +
        atmosphere.absorptionExtinction * 
            ComputeOpticalLengthToTopAtmosphereBoundary(
                atmosphere, atmosphere.absorptionDensity, r, mu)
    ));
}

// // map from [0.f, 1.f] uv coords to [0.5/n, 1 - 0.5/n] texture coords
// float GetTextureCoordFromUnitRange(float x, int textureSize)
// {
//     return 0.5f / float(textureSize) + x * (1.f - 1.f / float(textureSize));
// }

// // map from [0.5/n, 1 - 0.5f/ n] texture coords to [0.f, 1.f] uv coords
// float GetUnitRangeFromTextureCoord(float x, int textureSize){
//     return (x - 0.5f / float(textureSize)) / (1.f - 1.f / float(textureSize));
// }

// Note: uv are mapped in slope space
float2 GetTransmittanceTextureUvFromMuR(
    in AtmosphereParams atmosphere,
    float r,
    float mu
){
    float H = sqrt(max(0.f, atmosphere.rTop * atmosphere.rTop - atmosphere.rBottom * atmosphere.rBottom));
    float rho = sqrt(max(0.f, r * r - atmosphere.rBottom * atmosphere.rBottom));
    float d = DistanceToAtmosphereBoundary(atmosphere, r, mu);

    float dMin = atmosphere.rTop - r;
    float dMax = rho + H;
    d = clamp(d, dMin, dMax);
    float xMu = (d - dMin) / (dMax - dMin);
    float xR = rho / H;
    return float2(xMu, xR);
        // GetTextureCoordFromUnitRange(xMu, TRANSMITTANCE_TEXTURE_WIDTH),
        // GetTextureCoordFromUnitRange(xR, TRANSMITTANCE_TEXTURE_HEIGHT));
}

float2 GetMuRFromTransmittanceTextureUv(
    in AtmosphereParams atmosphere,
    in float2 uv
){
    float xMu = uv.x; // GetUnitRangeFromTextureCoord(uv.x, TRANSMITTANCE_TEXTURE_WIDTH);
    float xR = uv.y; // GetUnitRangeFromTextureCoord(uv.y, TRANSMITTANCE_TEXTURE_HEIGHT);

    float H = sqrt(max(0.f, atmosphere.rTop * atmosphere.rTop - atmosphere.rBottom * atmosphere.rBottom));
    float rho = H * xR;
    float r = sqrt(max(0.f, rho * rho + atmosphere.rBottom * atmosphere.rBottom));

    float dMin = atmosphere.rTop - r;
    float dMax = rho + H;

    float d = dMin + xMu * (dMax - dMin);
    float mu = d == 0.0f ? float(1.f) : (H * H - rho * rho - d * d) / (2.f * r * d); // d^2 + (2 * r * mu * d) + r^2 = rTop^2
    mu = clamp(mu, -1.f, 1.f);
    return float2(mu, r);
}

// compute transmittance
float3 ComputeTransmittanceToTopAtmosphereBoundaryTexture(
    in AtmosphereParams atmosphere,
    in float2 uv
){
    const float2 TRANSMITTANCE_TEXTURE_SIZE = 
        float2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    float2 Mu_R = GetMuRFromTransmittanceTextureUv(atmosphere, uv);
    return ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, Mu_R.y, Mu_R.x);
}

float ComputeDitherRange(in float mu)
{
    float offsetStrength = smoothstep(-0.1f, 0.1f, mu);
    return 4.f * offsetStrength;
}

float2 DitherUVTransmittanceTexture(
    in float ditherRange,
    in float2 uv, 
    in float2 screenPosition)
{
    float2 pixelSize = 1.f / float2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    float2 offset = float2(ditherRange, 0.f) * pixelSize;
    const float4 D2 = 0.25f * float4(3.f, 1.f, 0.f, 2.f);
    const float4 target = float4(1.f, 2.f, 3.f, 4.f);
    float2 ij = floor(fmod(screenPosition, float2(2.f, 2.f)));

    float idx = ij.x + 2.f * ij.y;
    float4 m = step(abs(idx - target), 0.5f) * D2;
    float d = m.x + m.y + m.z + m.w;
    uv = clamp(0.f, 1.f, uv + d * offset);
    return uv;
}
float3 GetTransmittanceToTopAtmosphereBoundary(
    in AtmosphereParams atmosphere,
    in Texture2D transmittanceTex,
    in SamplerState transmittanceSampler,
    in float r,
    in float mu
){
    r = clamp(r, atmosphere.rBottom + 1e-4f, atmosphere.rTop - 1e-4f);
    mu = clamp(mu, -1.f, 1.f);
    float2 uv = GetTransmittanceTextureUvFromMuR(atmosphere, r, mu);
    return transmittanceTex.Sample(transmittanceSampler, uv).xyz;
}

float3 GetTransmittanceToTopAtmosphereBoundary(
    in AtmosphereParams atmosphere,
    in Texture2D transmittanceTex,
    in SamplerState transmittanceSampler,
    in float2 screenPosition,
    in float r,
    in float mu
){
    float ditherRange = ComputeDitherRange(mu);
    r = clamp(r, atmosphere.rBottom + 1e-4f, atmosphere.rTop - 1e-4f);
    mu = clamp(mu, -1.f, 1.f);
    float2 uv = GetTransmittanceTextureUvFromMuR(atmosphere, r, mu);
    uv = DitherUVTransmittanceTexture(ditherRange, uv, screenPosition);
    return transmittanceTex.Sample(transmittanceSampler, uv).xyz;
}

float3 GetTransmittance(
    in AtmosphereParams atmosphere,
    in Texture2D transmittanceTex,
    in SamplerState transmittanceSampler,
    in float r, 
    in float mu, 
    in float d,
    in bool currentRayHitsGround
){
    float rD = clamp(sqrt(d * d + 2.f * r * mu * d + r * r), atmosphere.rBottom, atmosphere.rTop);
    float muD = clamp((r * mu + d) / rD, -1.f, 1.f);

    if(currentRayHitsGround){
        return min(
            GetTransmittanceToTopAtmosphereBoundary(
                atmosphere, transmittanceTex, transmittanceSampler, rD, -muD) /
            GetTransmittanceToTopAtmosphereBoundary(
                atmosphere, transmittanceTex, transmittanceSampler, r, -mu),
                1.f);
    }else {
        return min(
            GetTransmittanceToTopAtmosphereBoundary(
                atmosphere, transmittanceTex, transmittanceSampler, r, mu) / 
            GetTransmittanceToTopAtmosphereBoundary(
                atmosphere, transmittanceTex, transmittanceSampler, rD, muD),
                1.f);
    }
}

float3 GetTransmittanceToSun(
    in AtmosphereParams atmosphere,
    in Texture2D transmittanceTex,
    in SamplerState transmittanceSampler,
    in float r,
    in float mu_s
){
    // thetaH -> horizon zenith angle of point (mu, r)
    float sinThetaH = atmosphere.rBottom / r;
    float cosThetaH = -sqrt(max(1.0 - sinThetaH * sinThetaH, 0.f));

    return GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittanceTex, transmittanceSampler, r, mu_s) *
            smoothstep(
                -sinThetaH * atmosphere.sunAngularRadius, 
                sinThetaH * atmosphere.sunAngularRadius,
                mu_s - cosThetaH);
}

struct ScatteringInte{
    float3 rayleighScattered;
    float3 mieScattered;
};

// // consider current point 'p(r, mu)' and a point 'q(r_d, mu_d)' inside 
// // atmosphere with distance 'd' to 'p' along ray direction 'mu' 
// // compute scattering integrand about radiance originating from sun, being scatterred at q, reaching p. 
// // sun irradiance and phase function will be accounted latter 
// void ComputeSingleScatteringIntegrand(
//     in AtmosphereParams atmosphere,
//     in Texture2D transmittanceTex,
//     in SamplerState transmittanceSampler,
//     in float r,
//     in float mu,
//     in float mu_s,
//     in float nu, // nu = cos(AngleOf(direction_pd, direction_sun))
//     in float d,
//     in bool rayRMuHitsGround,
//     inout ScatteringInte scatterInte
// ){
//     // height 'r' at point q
//     float r_d = clamp(
//         sqrt(d * d + 2.f * r * mu * d + r * r), atmosphere.rBottom, atmosphere.rTop);
    
//     //zenith angle of sun direction 'mu' at point q
//     float mu_s_d = clamp((r * mu_s + d * nu) / r_d, -1.f, 1.f);

//     // transmittance of ray scattering from d to p equal to T(d, p) * T(sun, d)
//     float3 transmittance = 
//         GetTransmittance(atmosphere, transmittanceTex, transmittanceSampler,
//              r, mu, d, rayRMuHitsGround) *
//         GetTransmittanceToSun(atmosphere, transmittanceTex, transmittanceSampler, 
//              r_d, mu_s_d);

//     ScatteringInte singleScatter;

//     // inscatter density at q
//     scatterInte.rayleighScattered = transmittance * GetProfileDensity(
//         atmosphere.rayleighDensity, r_d - atmosphere.rBottom);
//     scatterInte.mieScattered = transmittance * GetProfileDensity(
//         atmosphere.mieDensity, r_d - atmosphere.rBottom);
// }

// float DistanceToNearestAtmosphereBoundary(
//     in AtmosphereParams atmosphere,
//     in float r,
//     in float mu,
//     bool rayRMuHitsGround
// ){
//     if(rayRMuHitsGround)
//         return DistanceToEarth(atmosphere, r, mu);
//     else
//         return DistanceToAtmosphereBoundary(atmosphere, r, mu);
// }

// // compute single scattering integral without phase function
// ScatteringInte ComputeSingleScattering(
//     in AtmosphereParams atmosphere,
//     in Texture2D transmittanceTex,
//     in SamplerState transmittanceSampler,
//     in float r,
//     in float mu,
//     in float mu_s,
//     in float nu,
//     in bool rayRMuHitsGround
// ){
//     const int SAMPLE_COUNT = 50;
//     float sampleStep = 
//         DistanceToNearestAtmosphereBoundary(atmosphere, r, mu, rayRMuHitsGround) / float(SAMPLE_COUNT);

//     ScatteringInte scatterSum;
//     for(int i = 0; i <= SAMPLE_COUNT; i++)
//     {
//         ScatteringInte singleScattering_i;
//         float sample_i = (0.5f + i) * sampleStep;

//         ComputeSingleScatteringIntegrand(atmosphere, transmittanceTex, transmittanceSampler,
//             r, mu, mu_s, nu, sample_i, rayRMuHitsGround, singleScattering_i);

//         float weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5f : 1.f;
//         scatterSum.rayleighScattered += singleScattering_i.rayleighScattered * weight_i;
//         scatterSum.mieScattered += singleScattering_i.mieScattered * weight_i;
//     }

//     // phase(omega_sun, omega_p_i) are computed at run time
//     scatterSum.rayleighScattered *= sampleStep * atmosphere.solarIrradiance * atmosphere.rayleighScattering;
//     scatterSum.mieScattered      *= sampleStep * atmosphere.solarIrradiance * atmosphere.mieScattering;
    
//     return scatterSum;
// }

// // mapping from (r, mu, mu_s, nu) to (u, v, w, z)
// float4 GetScatteringTextureUvwzFromRMuMusNu(
//     in AtmosphereParams atmosphere,
//     in float r,
//     in float mu,
//     in float mu_s,
//     in float nu,
//     in bool rayRMuHitsGround 
// ){
//     // distance to atmosphere boundary for a horizontal ray at ground level
//     float H = sqrt(atmosphere.rTop * atmosphere.rTop - atmosphere.rBottom * atmosphere.rBottom);
//     float rho = sqrt(r * r - atmosphere.rBottom * atmosphere.rBottom);  // distance to horizon
//     float u_r = clamp(rho / H, 0.f, 1.f);

//     // discriminant of quadratic intersections of ray(r, mu) with ground
//     float r_mu = r * mu;
//     float discriminant = max(0.f, r_mu * r_mu - r * r + atmosphere.rBottom * atmosphere.rBottom);
//     float u_mu;
//     if(rayRMuHitsGround){
//         float d = -r_mu - sqrt(discriminant);   // distance to ground
//         float dMin = r - atmosphere.rBottom;    // mu = -1
//         float dMax = rho;                       // mu = mu_horizon
//         // compute u_mu[0, 1] from d, then map to [0, 0.5]
//         u_mu = 0.5f - 0.5f * (dMax == dMin ? 0.0 : (d - dMin) / (dMax - dMin));
//     }
//     else{
//         // Distance to the top atmosphere boundary for the ray(r, mu), and its
//         // minimum and maxmum values over all mu - obtained for (r, 1) and (r, mu_horizon)
//         float d = -r_mu + sqrt(discriminant + H * H);   // '+ H^2' to map rBottom^2 in discriminant to rTop^2
//         float dMin = atmosphere.rTop - r;               // mu = 1
//         float dMax = rho + H;                           // mu = horizon

//         // compute u_mu[0, 1] from d, then map to [0.5, 1]
//         u_mu = 0.5f  + 0.5 * ((d - dMin) / (dMax - dMin));
//     }

//     // distance from ground to atmosphere boundary along sun direction
//     float d = DistanceToAtmosphereBoundary(atmosphere, atmosphere.rBottom, mu_s);
//     float dMin = atmosphere.rTop - atmosphere.rBottom;
//     float dMax = H;
//     float a = (d - dMin) / (dMax - dMin);

//     // distance from ground to atmosphere boundary along maximum mu of sun light direction
//     float D = DistanceToAtmosphereBoundary(
//         atmosphere, atmosphere.rBottom, atmosphere.musMin);
//     float A = (D - dMin) / (dMax - dMin);

//     // ad-hoc function equal to 0 for mu_s == mu_s_min, 
//     // equal to 1 for mu_s = 1,
//     // with large slope around mu_s = 0
//     float u_mu_s = max(1.f - a / A, 0.f) / (1.f + a);

//     float u_nu = (nu + 1.f) / 2,f; // map cos(ray, sun) from [-1, 1] to [0, 1]
//     return float4(u_nu, u_mu_s, u_mu, u_r);
// }

// // inverse mapping 
// void GetRMuMuSNuFromScatteringTextureUvwz(
//     in AtmosphereParams atmosphere,
//     in float4 uvwz,
//     out float r,
//     out float mu,
//     out float mu_s,
//     out float nu,
//     out bool rayRMuHitsGround
// ){

//     float H = sqrt(atmosphere.rTop * atmosphere.rTop - atmosphere.rBottom * atmosphere.rBottom);
//     float rho = H * uvwz.w;
//     r = sqrt(rho * rho + atmosphere.rBottom * atmosphere.rBottom);
//     if(uvwz.z < 0.5){
//         float dMin = r - atmosphere.rBottom;
//         float dMax = rho;
//         float d = dMin + (dMax - dMin) * GetUnitRangeFromTextureCoord(
//             2.f * uvwz.z - 1.f, SCATTERING_TEXTURE_MU_SIZE / 2);
//         mu = d == 0.f ? 1.f :
//             clamp(-(rho * rho + d * d) / (2.f * r * d), -1.f, 1.f);
//         rayRMuHitsGround = true;
//     } else{
//         float dMin = atmosphere.rTop - r;
//         float dMax = rho + H;
//         float d = dMin + (dMax - dMin) * GetUnitRangeFromTextureCoord(
//             2.f * uvwz.z - 1.f, SCATTERING_TEXTURE_MU_SIZE / 2.f);
//         mu = d == 0.f ? 1.f :
//             clamp((H * H - rho * rho - d * d) / (2.f * r * d), -1.f, 1.f);
//         rayRMuHitsGround = false;
//     }

//     float x_mu_s = 
//         GetUnitRangeFromTextureCoord(uvwz.y, SCATTERING_TEXTURE_MU_S_SIZE);
//     float dMin = atmosphere.rTop - atmosphere.rBottom;
//     float dMax = H;
//     float D = DistanceToAtmosphereBoundary(
//         atmosphere, atmosphere.rBottom, atmosphere.musMin);
//     float A = (D - dMin) / (dMax - dMin);

//     // inverse map of the ad-hoc function of GetScatteringTextureUvwzFromRMuMusNu
//     float a = (A - x_mu_s * A) * (1.f + x_mu_s * A);
//     float d = dMin + min(a, A) * (dMax - dMin);
//     mu_s = d == 0.f ? 1.f :
//         clamp((H * H - d * d) / (2.f * atmosphere.rBottom * d), -1.f, 1.f);

//     nu = clamp(uvwz.x * 2.f - 1.f, -1.f, 1.f);
// }

// void GetRMuMuSNuFromScatteringTextureFragCoord(
//     in AtmosphereParams atmosphere,
//     in float3 fragCoord,
//     inout float r,
//     inout float mu,
//     inout float mu_s,
//     inout float nu,
//     inout bool rayRMuHitsGround
// ){
//     const float4 SCATTERING_TEXTURE_SIZE = {
//         SCATTERING_TEXTURE_NU_SIZE - 1,
//         SCATTERING_TEXTURE_MU_S_SIZE,
//         SCATTERING_TEXTURE_MU_SIZE,
//         SCATTERING_TEXTURE_R_SIZE
//     };

//     float fragCoordNu = 
//         floor(fragCoord.x / float(SCATTERING_TEXTURE_MU_S_SIZE));
//     float fragCoordMuS = 
//         fmod(fragCoord.x, float(SCATTERING_TEXTURE_MU_S_SIZE));
//     float4 uvwz = 
//         float4(fragCoordNu, fragCoordMuS, fragCoord.y, fragCoord.z) / SCATTERING_TEXTURE_SIZE;
    
//     GetRMuMuSNuFromScatteringTextureUvwz(
//         atmosphere, uvwz, r, mu, mu_s, nu, rayRMuHitsGround);

//     // Clamp nu to its valid range of values, given mu and mu_s.
//     nu = clamp(
//         nu, 
//         mu * mu_s - sqrt((1.f - mu * mu) * (1.f - mu_s * mu_s)),
//         mu * mu_s + sqrt((1.f - mu * mu) * (1.f - mu_s * mu_s))
//     );
// }

// ScatteringInte ComputeSingleScatteringTexture(
//     in AtmosphereParams atmosphere,
//     in Texture2D transmittanceTex,
//     in SamplerState transmittanceSampler,
//     in float3 fragCoord
// ){
//     float r;
//     float mu;
//     float mu_s;
//     float nu;
//     bool rayRMuHitsGround;
//     GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, fragCoord,
//         r, mu, mu_s, nu, rayRMuHitsGround);
//     return ComputeSingleScattering(atmosphere, transmittanceTex, transmittanceSampler,
//         r, mu, mu_s, nu, rayRMuHitsGround);
// }

// float3 GetScattering(
//     in AtmosphereParams atmosphere,
//     in Texture3D scatteringTex,
//     in SamplerState scatteringSampler, 
//     float r,
//     float mu,
//     float mu_s,
//     float nu,
//     bool rayRMuHitsGround
// ){
//     float4 uvwz = GetScatteringTextureUvwzFromRMuMusNu(
//         atmosphere, r, mu, mu_s, nu, rayRMuHitsGround);
//     float texCoordX = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
//     float texX = floor(texCoordX);
//     float lerp = texCoordX - texX;
//     float3 uvw0 = float3((texX + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
//         uvwz.z, uvwz.w);
//     float3 uvw1 = float3((texX + 1.f + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
//         uvwz.z, uvwz.w);
    
//     return scatteringTex.Sample(scatteringSampler, uvw0).xyz * (1.f - lerp) + 
//         scatteringTex.Sample(scatteringSampler, uvw1).xyz * lerp;
// }

// float3 GetScattering(
//     in AtmosphereParams atmosphere,
//     in Texture3D singleRayleighScatteringTex,
//     in SamplerState singleRayleighScatteringSampler,
//     in Texture3D singleMieScatteringTex,
//     in SamplerState singleMieScatteringSampler,
//     in Texture3D multipleScatteringTex,
//     in SamplerState multipleScatteringSampler,
//     in float r,
//     in float mu,
//     in float mu_s,
//     in float nu,
//     bool rayRMuHitsGround,
//     int scatteringOrder)
// {
//     if(scatteringOrder == 1){
//         float3 rayleigh = GetScattering(
//             atmosphere, singleRayleighScatteringTex, singleRayleighScatteringSampler, 
//             r, mu, mu_s, nu, rayRMuHitsGround);
//         float3 mie = GetScattering(
//             atmosphere, singleMieScatteringTex, singleMieScatteringSampler, 
//             r, mu, mu_s, nu, rayRMuHitsGround);
//         return rayleigh * RayleighPhase(nu) + mie * MiePhase(atmosphere.miePhaseG, nu);
//     }
//     else{
//         return GetScattering(
//             atmosphere, multipleScatteringTex, multipleScatteringSampler, 
//             r, mu, mu_s, nu, rayRMuHitsGround);
//     }
// }

// float3 GetIrradiance(
//     in AtmosphereParams atmosphere,
//     in Texture2D transmittanceTex,
//     in SamplerState transmittanceSampler,
//     in Texture3D singleRayleighScatteringTex,
//     in SamplerState singleRayleighScatteringSampler,
//     in Texture3D singleMieScatteringTex,
//     in SamplerState singleMieScatteringSampler,
//     in Texture3D multipleScatteringTex,
//     in SamplerState multipleScatteringSampler,
//     in Texture2D irradianceTex,
//     in SamplerState irradianceSampler,
//     in float r,
//     in float mu,
//     in float mu_s,
//     in float nu,
//     in int scatteringOrder
// ){
//     // Compute unit direction vectors for the zenith, the view direction omega and
//     // and the sun direction omega_s, such that the cosine of the view-zenith
//     // angle is mu, the cosine of the sun-zenith angle is mu_s, and the cosine of
//     // the view-sun angle is nu. The goal is to simplify computations below
//     float3 zenithDirection = float3(0.f, 0.f, 1.f);
//     float3 omega = float3(sqrt(1.f - mu * mu), 0.f, mu);
//     float sunDirX = omega.x == 0.f ? 0.0f : (nu - mu * mu_s) / omega.x;
//     float sunDirY = sqrt(max(1.f - sunDirX * sunDirX - mu_s * mu_s, 0.f));
//     float3 omegaS = float3(sunDirX, sunDirY, mu_s);

//     const int SAMPLE_COUNT = 16;
//     const float dPhi = M_PI / float(SAMPLE_COUNT);
//     const float dTheta = M_PI / float(SAMPLE_COUNT);
//     float3 rayleigh_mie = 1.f; // (watt/(m^3 * sr * nm))

//     // loop over theta
//     for(int l = 0; l < SAMPLE_COUNT; ++l)
//     {
//         float theta = (l + 0.5) * dTheta;
//         float cosTheta = cos(theta);
//         float sinTheta = sin(theta);

//         bool rayRThetaHitsGround = RayHitsGround(atmosphere, r, cosTheta);
        
//         // The discance and transmittance to the ground onlu depend on theta,
//         // compute it in the outter loop
//         float distanceToGround = 0.f;
//         float3 transmittanceToGround = 0.f;
//         float3 groundAlbedo = 0.f;

//         if(rayRThetaHitsGround){
//             distanceToGround = DistanceToAtmosphereBoundary(atmosphere, r, cosTheta);
//             transmittanceToGround = GetTransmittance(
//                 atmosphere, transmittanceTex, transmittanceSampler,
//                 r, cosTheta, distanceToGround, true);
//             groundAlbedo = atmosphere.groundAlbedo;
//         }

//         for(int m = 0; m < 2 * SAMPLE_COUNT; ++m){
//             float phi = float(m + 0.5f) * dPhi;
//             float3 omegaI = float3(cos(phi) + sinTheta, sin(phi) * sinTheta, cosTheta);

//             float nul = dot(omegaS, omegaI);

//             // incidentRadiance scattered from atmosphere
//             float3 incidentRadiance = GetScattering(atmosphere,
//                 singleRayleighScatteringTex, singleRayleighScatteringSampler,
//                 singleMieScatteringTex, singleMieScatteringSampler,
//                 multipleScatteringTex, multipleScatteringSampler,
//                 r, omegaI.z, mu_s, nul, rayRThetaHitsGround, scatteringOrder - 1);

//             // incidentRadiance reflected from ground
//             float3 groundNormal = normalize(zenithDirection * r + omegaI * distanceToGround);
//             float3 groundIrradiance = GetIrradiance(atmosphere, irradianceTex, irradianceSampler,
//                 atmosphere.rBottom, dot(groundNormal, omegaS));
//             float3 groundIrradiance = 0.f;
//             incidentRadiance += transmittanceToGround * groundAlbedo * (1.f / M_PI) * groundIrradiance;

//             float nu2 = dot(omega, omegaI);
//             float rayleighDensity = GetProfileDensity(atmosphere.rayleighDensity, r - atmosphere.rBottom);
//             float mieDensity = GetProfileDensity(atmosphere.mieDensity, r - atmosphere.rBottom);
//             rayleigh_mie += incidentRadiance * 
//                 (atmosphere.rayleighScattering * rayleightDensity * RayleighPhase(nu2) +
//                 atmosphere.mieScattering * mieDensity * MiePhase(atmosphere.miePhaseG, nu2)) * dOmegaI;
//         }
//     }
//     return raylei_mie;
// }