float2 RayIntersectSphere1(float3 rayOrigin, float3 rayDir, float4 sphere)
{
    float3 temp = rayOrigin - sphere.xyz;
    float b = dot(rayDir, temp);
    float c = dot(temp, temp) - sphere.w * sphere.w;

    float disc = b * b - c;

    // both t.x and t.y are negative means no intersection
    if(disc < 0.0) return float2(1e38f, 1e38f);

    float disc_sqrt = sqrt(disc);

    float t0 = -b - disc_sqrt;
    float t1 = -b + disc_sqrt;

    return float2(t0, t1);
}

bool RayHitSphere1(float2 t)
{
    return t.y > t.x && t.y > 0.f;
}

#define TRANSMITTANCE_TEXTURE_WIDTH 256
#define TRANSMITTANCE_TEXTURE_HEIGHT 64

#define SCATTERING_TEXTURE_R_SIZE 32
#define SCATTERING_TEXTURE_MU_SIZE 128
#define SCATTERING_TEXTURE_MU_S_SIZE 32
#define SCATTERING_TEXTURE_NU_SIZE 8

#define IRRADIANCE_TEXTURE_WIDTH 64
#define IRRADIANCE_TEXTURE_HEIGHT 16

/* #define COMPUTE_GROUP_SIZE_X 16
#define COMPUTE_GROUP_SIZE_Y 16
#define COMPUTE_GROUP_SIZE_Z 4
*/

struct DensityProfileLayer {
    float width;
    float exp_term;
    float exp_scale;
    float linear_term;
    float constant_term;
};

struct DensityProfile {
    DensityProfileLayer layers[2];     
};

struct AtmosphereParameters {
    float3 solar_irradiance;
    float mie_phase_function_g;
    float3 rayleigh_scattering;
    float mu_s_min;
    float3 mie_scattering;
    float sun_angular_radius;
    float3 mie_extinction;
    float bottom_radius;
    float3 absorption_extinction;
    float top_radius;
    float3 ground_albedo;
    DensityProfile rayleigh_density;
    DensityProfile mie_density;
    DensityProfile absorption_density;
};

AtmosphereParameters initAtmosphereWithBoundary(
    float top_radius,
    float bottom_radius)
{
    DensityProfileLayer defaultLayer;
    defaultLayer.width = 0.f;       
    defaultLayer.exp_term = 0.f;
    defaultLayer.exp_scale = 0.f;    
    defaultLayer.linear_term = 0.f;
    defaultLayer.constant_term = 0.f;

    DensityProfile default_profile;
    default_profile.layers[0] = defaultLayer;
    default_profile.layers[1] = defaultLayer;

    AtmosphereParameters atmosphere;
    atmosphere.top_radius = top_radius;
    atmosphere.bottom_radius = bottom_radius;
    atmosphere.rayleigh_scattering = 0.f;
    atmosphere.mie_scattering = 0.f;
    atmosphere.mie_extinction = 0.f;
    atmosphere.absorption_extinction = 0.f;
    atmosphere.rayleigh_density = default_profile;
    atmosphere.mie_density = default_profile;
    atmosphere.absorption_density = default_profile;
    atmosphere.sun_angular_radius = 0.f;
    atmosphere.mu_s_min = 0.f;
    atmosphere.ground_albedo = 0.f;

    return atmosphere;
}

float ComputeDitherRange(in float mu)
{
    float offsetStrength = smoothstep(-0.1f, 0.1f, mu);
    return 4.f * offsetStrength;
}

float DistanceToTopAtmosphereBoundary(in AtmosphereParameters atmosphere,
    float r, float mu) {
  float discriminant = r * r * (mu * mu - 1.0) +
      atmosphere.top_radius * atmosphere.top_radius;
  return max(0, -r * mu + sqrt(max(discriminant, 0.0)));
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

float2 GetTextureCoordFromUnitRange(float2 x, int2 texture_size) {
  return 0.5 / float2(texture_size) + x * (1.0 - 1.0 / float2(texture_size));
}

float2 GetTransmittanceTextureUvFromRMu(in AtmosphereParameters atmosphere,
    float r, float mu) 
{
    // Distance to top atmosphere boundary for a horizontal ray at ground level.
    float H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
        atmosphere.bottom_radius * atmosphere.bottom_radius);
    // Distance to the horizon.
    float rho =
        sqrt(max(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius, 0));
    // Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
    // and maximum values over all mu - obtained for (r,1) and (r,mu_horizon).
    float d = DistanceToTopAtmosphereBoundary(atmosphere, r, mu);
    float d_min = atmosphere.top_radius - r;
    float d_max = rho + H;
    float d_mu = (d - d_min) / (d_max - d_min);
    float d_x = rho / H;
    return GetTextureCoordFromUnitRange(float2(d_mu, d_x), int2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT));
}

float3 GetTransmittanceToTopAtmosphereBoundaryWithDithering(
    in AtmosphereParameters atmosphere,
    in Texture2D transmittance_texture,
    in SamplerState transmittance_sampler,
    float2 screenPosition,
    float r, float mu) 
{
    float ditherRange = ComputeDitherRange(mu);
    r = clamp(r, atmosphere.bottom_radius + 1e-4f, atmosphere.top_radius - 1e-4f);
    mu = clamp(mu, -1.f, 1.f);
    float2 uv = GetTransmittanceTextureUvFromRMu(atmosphere, r, mu);
    uv = DitherUVTransmittanceTexture(ditherRange, uv, screenPosition);
    return transmittance_texture.SampleLevel(transmittance_sampler, uv, 0).xyz;
}

Texture2D S_PreComputeTransmittanceMap;
SamplerState S_PreComputeTransmittanceMapSamplerState;

#define tTex S_PreComputeTransmittanceMap
#define tSampler S_PreComputeTransmittanceMapSamplerState

uniform float4 U_SphFogLowerLayerColor;
uniform float4 U_SphFogUpperLayerColor;
uniform float4 U_SphFogEarthCenter;
uniform float4 U_SphFogBlendParam;
uniform float4 U_SphFogParam0; //  BOTTOM_DENSITY_PER_KM | none | earthRadius | atmosphereRadius
uniform float4 U_SphFogParam1; //  enableBlend | none | fogHeight | belndRange
uniform float4 U_SphFogSkyParam;

#define BOTTOM_DENSITY_PER_KM U_SphFogParam0.x
    
float4 ComputeSphFogBlocked(
    in float3 worldPosInMeter,
    in float3 cameraPosInMeter,
    in float3 viewDir,          // input viewDir pointing from Camera to shading point
    in float2 screenPosition)  
{
    float4 fogFinal = 0.f;
    if(U_SphFogParam1.y < 0.8f)
        return fogFinal;

    float3 cameraPos = cameraPosInMeter * 0.001f;
    float3 worldPos = worldPosInMeter * 0.001f;
    AtmosphereParameters atmosphere = 
        initAtmosphereWithBoundary(U_SphFogParam0.z, U_SphFogParam0.w);

    float3 earthCenter = U_SphFogEarthCenter.xyz - cameraPos;
    float4 earthSphere = float4(U_SphFogEarthCenter.xyz, U_SphFogParam0.w);
    float4 atmosSphere = float4(U_SphFogEarthCenter.xyz, U_SphFogParam0.z);

    float2 tAtmosphere = RayIntersectSphere1(
        cameraPos, viewDir, atmosSphere);
    float2 tEarth = RayIntersectSphere1(
        cameraPos, viewDir, earthSphere);

    do{
        if(!RayHitSphere1(tAtmosphere))break; 

        float tFromDepth = length(cameraPos - worldPos); // t from camera to shadingPoint
        if(tFromDepth < tAtmosphere.x)break;

        // begin to compute fog transmittance
        bool rayHitInner = RayHitSphere1(tEarth);
        
        float densityFactor = 0.f;
        float distanceFactor = 0.f;

        // compute fog density factor
        //---------------------------------------------------------------------------------------------------
            float tMid = (tAtmosphere.x + tAtmosphere.y) / 2.0f;

            // 线与球求交的函数替换成了RayHitSphere1，在没有交点时返回两个极大值
            float transmittance = 0;
            float3 radiusEnterP, radiusBlockedP;
            float tUnderBottom = max(0, tEarth.y) - max(0, tEarth.x) - (tFromDepth > 0 ? max(tFromDepth, tEarth.y) - max(tFromDepth, tEarth.x) : 0);

            // 第一个IF算两个采样点的半径
            // '前'表示更靠近相机，'相机前方'为相机朝向
            if (tMid < 0){
                // 中点在相机背后：相机位置采样(如果与内圈相交, 限制不小于内圈) / 障碍物位置采样(限制不超出大气层，不小于内圈)
				float tCam = (rayHitInner ? max(0, tEarth.y) : 0);
                radiusEnterP = tCam * viewDir - earthCenter;
                radiusBlockedP = clamp(tFromDepth, tCam, tAtmosphere.y) * viewDir - earthCenter;
            }
            else if (tMid > tFromDepth){
                // 障碍物在中点前：
                //      障碍物在相机前(与第一个case对称)：障碍物位置采样(如果与内圈相交, 限制不小于内圈) / 相机位置采样(限制不小于内圈)
				float tObstacle = (rayHitInner ? min(tFromDepth, tEarth.x) : tFromDepth);
                radiusEnterP = earthCenter - tObstacle * viewDir;
                radiusBlockedP = earthCenter - clamp(0, tAtmosphere.x, tEarth.x) * viewDir;
            }
            else {
                // 中点在相机前方且障碍物在中点后：内圈交点采样^2 / (障碍物位置采样(clamp到内外圈之间) * 相机位置采样())
                float t = (rayHitInner ? tEarth.y : tMid);
                radiusEnterP = t * viewDir - earthCenter;
                radiusBlockedP = clamp(tFromDepth, t, tAtmosphere.y) * viewDir - earthCenter;
            }

            float outerTransmittance = GetTransmittanceToTopAtmosphereBoundaryWithDithering(
                atmosphere, tTex, tSampler, screenPosition, length(radiusEnterP), dot(viewDir, normalize(radiusEnterP))).r;
            float blockedTransmittance = GetTransmittanceToTopAtmosphereBoundaryWithDithering(
                atmosphere, tTex, tSampler, screenPosition, length(radiusBlockedP), dot(viewDir, normalize(radiusBlockedP))).r;

            // 障碍物和相机不在中点同一侧时，需要outerTransmittance ^ 2去掉两段障碍物和相机后的两段transmittance
            // 第二个if计算额外要去掉的那部分transmittance
            if ((tMid > 0 && tMid < tFromDepth)){
                float3 radius = earthCenter - (rayHitInner ? min(0, tEarth.x) : 0) * viewDir;
				blockedTransmittance *= GetTransmittanceToTopAtmosphereBoundaryWithDithering(
                    atmosphere, tTex, tSampler, screenPosition, length(radius), dot(viewDir, normalize(radius))).r;
                outerTransmittance *= outerTransmittance;
            }

            transmittance = outerTransmittance / blockedTransmittance;
            transmittance *= exp(-tUnderBottom * BOTTOM_DENSITY_PER_KM);

            
            // compute fog color
            // ad-hoc function
            // fog reveal more sky color with larger transmittance value
            float density = -log(transmittance);

            float3 finalColor = lerp(
                pow(U_SphFogUpperLayerColor.xyz, 2.2f), 
                pow(U_SphFogLowerLayerColor.xyz, 2.2f), 
                smoothstep(0.2f, 0.6f, density * U_SphFogBlendParam.w));

            transmittance = clamp(transmittance, 0, 1.f);
            densityFactor = 1.f - transmittance;

            // // compute fog color
            // float density = -log(transmittance);
            // float skyParam = smoothstep(0.1f, 0.8f, density * colorGradient);
            // float3 finalColor = lerp(pow(skyColor, 2.2f), pow(fogColor, 2.2f), skyParam); 

        //---------------------------------------------------------------------------------------------------

        // compute fog distance factor
        //---------------------------------------------------------------------------------------------------
            // distance lerp from our exponential height fog
            float vRampDen = saturate(tFromDepth * U_SphFogBlendParam.x - U_SphFogBlendParam.y);
            distanceFactor = vRampDen * (2.f - vRampDen);
        //---------------------------------------------------------------------------------------------------
        
        float finalAlpha =  densityFactor * distanceFactor;
        half3 gravityDirCamera = normalize(-earthCenter);
        float cosTheta = saturate(dot(viewDir, gravityDirCamera));
        float2 skyParam = float2(
            clamp(U_SphFogSkyParam.x * 2.f, 0.1f, 1.f),           // map (0 - 0.5) to (0.1 - 1.0)
            clamp(U_SphFogSkyParam.x * 2.f - 1.f, 0.f, 1.f)       // map (0.5 - 1) to (0.0 - 1.0)
        );

        float2 skyAlpha = float2(
            smoothstep(1.f - skyParam.x - 1e-5f, 1.f, 1.f - cosTheta),
            smoothstep(-1e-5f, 1.f - skyParam.y, 1.f - cosTheta)
        );

        finalAlpha = finalAlpha * (U_SphFogSkyParam.x < 0.5f ? skyAlpha.x : skyAlpha.y);
        finalAlpha = min(U_SphFogBlendParam.z, finalAlpha);

        // blend with atmosphere
        {
            // Altitude > U_SphFogParam1.z
            float altitude = length(earthCenter -  0.f);
            float blendFactor = 1.f - smoothstep(0.f, 1.f, (altitude - U_SphFogParam1.z) / U_SphFogParam1.w);

            // if enabled != 0.f
            if(U_SphFogParam1.x < 0.8f)
                blendFactor = 1.f;
            finalAlpha *= blendFactor;
        }

        fogFinal = float4(pow(finalColor, 0.4545f), finalAlpha);
    } while(false);

    return fogFinal;
}

float4 ComputeSphFogUnBlocked(
    in float3 cameraPosInMeter,
    in float3 viewDir,          // input viewDir pointing from Camera to shading point
    in float2 screenPosition) 
{
    float4 fogFinal = 0.f;
    if(U_SphFogParam1.y < 0.8f)
        return fogFinal;

    float3 cameraPos = cameraPosInMeter * 0.001f;
    AtmosphereParameters atmosphere = 
        initAtmosphereWithBoundary(U_SphFogParam0.z, U_SphFogParam0.w);

    float3 earthCenter = U_SphFogEarthCenter.xyz - cameraPos;
    float4 earthSphere = float4(U_SphFogEarthCenter.xyz, U_SphFogParam0.w);
    float4 atmosSphere = float4(U_SphFogEarthCenter.xyz, U_SphFogParam0.z);

    float2 tAtmosphere = RayIntersectSphere1(
        cameraPos, viewDir, atmosSphere);
    float2 tEarth = RayIntersectSphere1(
        cameraPos, viewDir, earthSphere);

    do {
        if(!RayHitSphere1(tAtmosphere))break; 

        // begin to compute fog transmittance
        bool rayHitInner = RayHitSphere1(tEarth);
        
        float densityFactor = 0.f;
        float distanceFactor = 0.f;

        // compute fog density factor
        //---------------------------------------------------------------------------------------------------
            float tMid = (tAtmosphere.x + tAtmosphere.y) / 2.0f;

            // 线与球求交的函数替换成了RayHitSphere1，在没有交点时返回两个极大值
            float transmittance = 0;
            float3 radiusEnterP, radiusBlockedP;
            float tUnderBottom = max(0, tEarth.y) - max(0, tEarth.x);

            // 第一个IF算两个采样点的半径
            // '前'表示更靠近相机，'相机前方'为相机朝向
            if (tMid < 0) {
                // 中点在相机背后：相机位置采样(如果与内圈相交, 限制不小于内圈) / 障碍物位置采样(限制不超出大气层，不小于内圈)
				float tCam = (rayHitInner ? max(0, tEarth.y) : 0);
                radiusEnterP = tCam * viewDir - earthCenter;
                radiusBlockedP = tAtmosphere.y * viewDir - earthCenter;
            }
            else {
                // 中点在相机前方且障碍物在中点后：内圈交点采样^2 / (障碍物位置采样(clamp到内外圈之间) * 相机位置采样())
                float t = (rayHitInner ? tEarth.x : tMid);
                radiusEnterP = earthCenter - t * viewDir;
                radiusBlockedP = earthCenter - clamp(0, tAtmosphere.x, t) * viewDir;
            }

            float outerTransmittance = GetTransmittanceToTopAtmosphereBoundaryWithDithering(
                atmosphere, tTex, tSampler, screenPosition, length(radiusEnterP), dot(viewDir, normalize(radiusEnterP))).r;
            float blockedTransmittance = GetTransmittanceToTopAtmosphereBoundaryWithDithering(
                atmosphere, tTex, tSampler, screenPosition, length(radiusBlockedP), dot(viewDir, normalize(radiusBlockedP))).r;

            transmittance = (tMid > 0 ? outerTransmittance * outerTransmittance : outerTransmittance) / blockedTransmittance;
            transmittance *= exp(-tUnderBottom * BOTTOM_DENSITY_PER_KM);

            
            // compute fog color
            // ad-hoc function
            // fog reveal more sky color with larger transmittance value
            float density = -log(transmittance);

            float3 finalColor = lerp(
                pow(U_SphFogUpperLayerColor.xyz, 2.2f), 
                pow(U_SphFogLowerLayerColor.xyz, 2.2f), 
                smoothstep(0.2f, 0.6f, density * U_SphFogBlendParam.w));
        
            transmittance = clamp(transmittance, 0, 1.f);
            densityFactor = 1.f - transmittance;

            // // compute fog color
            // float density = -log(transmittance);
            // float skyParam = smoothstep(0.1f, 0.8f, density * colorGradient);
            // float3 finalColor = lerp(pow(skyColor, 2.2f), pow(fogColor, 2.2f), skyParam); 

        //---------------------------------------------------------------------------------------------------
        
        float finalAlpha =  densityFactor;
        half3 gravityDirCamera = normalize(-earthCenter);
        float cosTheta = saturate(dot(viewDir, gravityDirCamera));
        float2 skyParam = float2(
            clamp(U_SphFogSkyParam.x * 2.f, 0.1f, 1.f),           // map (0 - 0.5) to (0.1 - 1.0)
            clamp(U_SphFogSkyParam.x * 2.f - 1.f, 0.f, 1.f)       // map (0.5 - 1) to (0.0 - 1.0)
        );

        float2 skyAlpha = float2(
            smoothstep(1.f - skyParam.x - 1e-5f, 1.f, 1.f - cosTheta),
            smoothstep(-1e-5f, 1.f - skyParam.y, 1.f - cosTheta)
        );

        finalAlpha = finalAlpha * (U_SphFogSkyParam.x < 0.5f ? skyAlpha.x : skyAlpha.y);
        finalAlpha = min(U_SphFogBlendParam.z, finalAlpha);

        // blend with atmosphere
        {
            // Altitude > U_SphFogParam1.z
            float altitude = length(earthCenter -  0.f);
            float blendFactor = 1.f - smoothstep(0.f, 1.f, (altitude - U_SphFogParam1.z) / U_SphFogParam1.w);

            // if enabled != 0.f
            if(U_SphFogParam1.x < 0.8f)
                blendFactor = 1.f;
            finalAlpha *= blendFactor;
        }

        fogFinal = float4(pow(finalColor, 0.4545f), finalAlpha);
    }while(false);

    return fogFinal;
}