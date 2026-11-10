uniform AtmosphereViewData    U_PSCustom0;
uniform ViewportRect          U_PSCustom1;
uniform AtmosphereParameters  U_CSCustom0;
uniform SkyAtmosphereSetting  U_CSCustom1;
uniform float4 U_MainLightDirection;
uniform float4 U_PBR_MainLightColor;

#define Atmosphere    U_CSCustom0
#define SkyAtmosphere U_CSCustom1
#define View          U_PSCustom0
#define Viewport      U_PSCustom1

struct SamplingSetup {
  bool variableSampleCount;
  float sampleCountIni;
  float minSampleCount;
  float maxSampleCount;
  float distanceToSampleCountMaxInv;
};

struct MediumSample {
  float3 scattering;
  float3 absorption;
  float3 extinction;

  float3 scatteringMie;
  float3 absorptionMie;
  float3 extinctionMie;

  float3 scatteringRay;
  float3 absorptionRay;
  float3 extinctionRay;

  float3 scatteringOzo;
  float3 absorptionOzo;
  float3 extinctionOzo;

  float3 albedo;
};

MediumSample SampleAtmosphereMedium(in float3 worldPos) {
  const float sampleHeight =
      max(0.f, length(worldPos) - Atmosphere.bottomRadius);
  const float densityMie = exp(Atmosphere.mieExp * sampleHeight);
  const float densityRay = exp(Atmosphere.rayleighExp * sampleHeight);

  const float densityOzo =
      sampleHeight < Atmosphere.ozoExtinction_width.w
          ? saturate(Atmosphere.ozoTerm.x * sampleHeight +
                     Atmosphere.ozoTerm.y)
          : saturate(Atmosphere.ozoTerm.z * sampleHeight +
                     Atmosphere.ozoTerm.w);

  MediumSample s;

  s.scatteringMie = densityMie * Atmosphere.mieScattering.rgb;
  s.absorptionMie =
      densityMie * (Atmosphere.mieExtinction - Atmosphere.mieScattering).rgb;
  s.extinctionMie = densityMie * Atmosphere.mieExtinction.rgb;

  s.scatteringRay = densityRay * Atmosphere.rayleighScattering.rgb;
  s.absorptionRay = 0.f;
  s.extinctionRay = s.scatteringRay + s.absorptionRay;

  s.scatteringOzo = 0.f;
  s.absorptionOzo = densityOzo * Atmosphere.ozoExtinction_width.rgb;
  s.extinctionOzo = s.scatteringOzo + s.absorptionOzo;

  s.scattering = s.scatteringMie + s.scatteringRay + s.scatteringOzo;
  s.absorption = s.absorptionMie + s.absorptionRay + s.absorptionOzo;
  s.extinction = s.extinctionMie + s.extinctionRay + s.extinctionOzo;

  s.albedo = GetAlbedo(s.scattering, s.extinction);
  return s;
}

float3 GetTranslatedCameraPlanetPos() {
  return View.cameraWorldPosKm_frameIdMod8.xyz -
         View.planetCenterPosAndViewHeightKm.xyz;
}

float4 GetNdcPos(in float2 svPos, in float deviceZ) {
  float2 pixelPos =
      svPos.xy - Viewport.rectMinAndMax.xy; // [0, viewRectSize]
  float2 xy =
      float2(2.f, -2.f) *
      (pixelPos.xy * Viewport.rectSizeAndInvSize.zw - 0.5f); // [-1. 1]
  return float4(xy, deviceZ, 1.f);
}

float3 GetScreenWorldDirection(in float4 svPos) {
  float4 NDCPos = GetNdcPos(svPos.xy, 1.f);
  float3 direction = normalize(float3(View.cameraXYRatio.xy * NDCPos.xy, -1.f));
  return normalize(mul(float4(direction, 0.f), View.cameraToWorldMatrix).xyz);
}

float3 GetScreenTranslatedWorldPos(float2 svPos, float deviceZ) {
  float4 NDCPos = GetNdcPos(svPos, deviceZ);
  float4 homoPos = mul(NDCPos, View.ndcToWorldMatrix);
  return homoPos.xyz / homoPos.w;
}

#ifdef MULTISCATTERING_APPROX_SAMPLING_ENABLED
float3 GetMultipleScattering(float3 worldPos, float viewZenithCosAngle) {
  float2 uv =
      saturate(float2(viewZenithCosAngle * 0.5f + 0.5f,
                      (length(worldPos) - Atmosphere.bottomRadius) /
                          (Atmosphere.topRadius - Atmosphere.bottomRadius)));

  float3 multiScatteredLuminance =
      MultiScatteredLuminanceTex.SampleLevel(BilinearSampler, uv, 0).rgb;
  return multiScatteredLuminance;
}
#endif

float3 GetTransmittance(in float lightZenithCosAngle, in float pHeight) {

#ifdef WHITE_TRANSMITTANCE
  float3 transmittanceToLight = 1.f;
#else
  float2 uv =
      GetTransmittanceLutUVs(pHeight, lightZenithCosAngle,
                             Atmosphere.bottomRadius, Atmosphere.topRadius);
  float3 transmittanceToLight =
      TransmittanceTex.SampleLevel(BilinearSampler, uv, 0).rgb;
#endif
  return transmittanceToLight;
}

#define DEFAULT_SAMPLE_OFFSET 0.3f
float SkyAtmosphereNoise(float2 uv) {
#if defined(VIEWDATA_AVAILABLE) && defined(PER_PIXEL_NOISE)
  return InterleavedGradientNoise(uv.xy, View.cameraWorldPosKm_frameIdMod8.w);
#else
  return DEFAULT_SAMPLE_OFFSET;
#endif
}

struct SingleScatteringResult {
  float3 L;
  float3 LMieOnly;
  float3 LRayOnly;
  float3 opticalDepth;
  float3 transmittance;
  float3 transmittanceMieOnly;
  float3 transmittanceRayOnly;
  float3 multiScatAs1;
};

// assume planet center located at world origin, all world position are relative
// to the planet center
SingleScatteringResult IntegrateSingleScatteredLuminance(
    in float4 svPos,           // pixel pos to sample transmittance
    in float3 worldPos,        // sampling start pos (camera pos)
    in float3 worldDir,        // view dirction
    in bool ground,            // account ground reflection
    in SamplingSetup sampling, // samples
    in float deviceZ,          // depth value
    in bool mieRayPhase, // compute phase for each sample or use uniform phase
    in float3 lightDir,  // sun light direction
    in float3 lightIlluminance,                  // sun light illuminance
    in float aerialPerspectiveViewDistanceScale, // aerial perspective scaler
    in float tMaxMax = 9000000.f // max distance that sample light can travel
) {
  SingleScatteringResult result;
  result.L = 0.f;
  result.LMieOnly = 0.f;
  result.LRayOnly = 0.f;
  result.opticalDepth = 0.f;
  result.transmittance = 1.f;
  result.transmittanceMieOnly = 1.f;
  result.transmittanceRayOnly = 1.f;
  result.multiScatAs1 = 0;

  // camera under planet surface
  if (dot(worldPos, worldPos) <=
      Atmosphere.bottomRadius * Atmosphere.bottomRadius) {
    return result;
  }

  float2 pixelPos = svPos.xy;

  // Compute next intersection with atmosphere or ground
  float3 planetO = float3(0.f, 0.f, 0.f);
  float tMax = 0.f; // stop point

#if 0
        float tBottom = RaySphereIntersectNearest(worldPos, worldDir, planetO, Atmosphere.bottomRadius);
        float tTop = RaySphereIntersectNearest(worldPos, worldDir, planetO, Atmosphere.topRadius);

        // no intersection with planet
        if(tBottom < 0.f)
        {
            // no intersection with atmosphere nor planet
            if(tTop < 0.f)
            {
                tMax = 0.f;
                return result;
            }

            // intersect atmosphere but no intersection with plant
            else
            {
                tMax = tTop;
            }
        }
        // intersect planet
        else{

            // interect both atmosphere and planet, find closer intersection
            if(tTop > 0.f)
            {
                tMax = min(tTop, tBottom);
            }
        }
#else
  float tBottom = 0.f;
  float2 solB = RayIntersectSphere(worldPos, worldDir,
                                   float4(planetO, Atmosphere.bottomRadius));
  float2 solT = RayIntersectSphere(worldPos, worldDir,
                                   float4(planetO, Atmosphere.topRadius));

  const bool bNoBotIntersection = all(solB < 0.f);
  const bool bNoTopIntersection = all(solT < 0.f);

  if (bNoTopIntersection) {
    // No intersection with planet or its atmosphere.
    tMax = 0.f;
    return result;
  } else if (bNoBotIntersection) {
    // No intersection with planet but has intersection with atmosphere
    tMax = max(solT.x, solT.y);
  } else {
    // intersection with planet and atmosphere.
    // trace up to the planet ground
    tBottom =
        max(0.0f, min(solB.x,
                      solB.y)); // setting tBottom = 0 when cam is inside sphere
    tMax = tBottom;
  }
#endif

  float planetOnOpaque = 1.f;
#ifdef VIEWDATA_AVAILABLE
  // account for opaque objects from depth buffer
  if (deviceZ != FAR_DEPTH_VALUE) {
    float3 depthBufferTranslatedWorldPosKm =
        GetScreenTranslatedWorldPos(svPos.xy, deviceZ).xyz *
        0.001f; // world position from depth
    depthBufferTranslatedWorldPosKm *= SkyAtmosphere.skyLuminanceFactor_distanceScale.w;
    const float3 traceStartTranslatedWorldPosKm =
        worldPos +
        View.planetCenterPosAndViewHeightKm
            .xyz; // transform camera from planetSpace pos to worldSpace pos
    const float3 traceStartToSurfaceVectorKm =
        depthBufferTranslatedWorldPosKm - traceStartTranslatedWorldPosKm;
    float tDepth = length(traceStartToSurfaceVectorKm);

    // blocked by surfaces inside atmosphere
    if (tDepth < tMax) {
      tMax = tDepth;
    }

    // do not apply atmosphere on surfaces are in front of atmosphere
    if (dot(worldDir, traceStartToSurfaceVectorKm) < 0.0f) {
      return result;
    }
  }
#endif
  tMax = min(tMax, tMaxMax); // clamp to max distance

  // prepare samples
  float sampleCount = sampling.sampleCountIni;
  float sampleCountFloor = sampling.sampleCountIni;
  float tMaxFloor = tMax;
  if (sampling.variableSampleCount) {
    // compute sample count according light travel distance instead of using
    // static sample count
    sampleCount = lerp(sampling.minSampleCount, sampling.maxSampleCount,
                       saturate(tMax * sampling.distanceToSampleCountMaxInv));
    sampleCountFloor = floor(sampleCount);
    tMaxFloor = tMax * sampleCountFloor / sampleCount;
  }
  float dt = tMax / sampleCount;

  const float uniformPhase = 1.f / (4.f * PI);
  const float3 wi = lightDir;
  const float3 wo = worldDir;
  float cosTheta = dot(wi, wo);
  float miePhaseValue = HenyeyGreensteinPhase(Atmosphere.miePhaseG, -cosTheta);
  float rayleighPhaseValue = RayleighPhase(cosTheta);

  // ray marching
  float3 L = 0.f;
  float3 LMieOnly = 0.f;
  float3 LRayOnly = 0.f;
  float3 throughput = 1.f; // transmittance from camera to sample point
  float3 throughputMieOnly = 1.f;
  float3 throughputRayOnly = 1.f;
  float3 opticalDepth = 0.f;
  float t = 0.f;
  float tPrev = 0.f; // distance from last sample to camera

  float3 exposedLightIlluminance =
      lightIlluminance *
      1.f; // since we don't support pre-exposure in echo, pre-exposure = 1.f

  // note: local referencial is used to get surface shadows in sky view lut pass
  // #if SKYVIEWLUT_PASS
  //     float3x3 localReferencial =
  //     GetSkyViewLutReferential(ViewSetting.SkyViewLutReferential);
  // #endif

#if PER_PIXEL_NOISE
  float pixelNoise = SkyAtmosphereNoise(pixelPos.xy);
#else  // PER_PIXEL_NOISE
  float pixelNoise = DEFAULT_SAMPLE_OFFSET;
#endif // PER_PIXEL_NOISE

  for (float sampleI = 0.f; sampleI < sampleCount; sampleI += 1.f) {
    // compute current ray t and sample point p
    if (sampling.variableSampleCount) {
      float t0 = sampleI / sampleCountFloor;
      float t1 = (sampleI + 1.f) / sampleCountFloor;

      // non linear distribution of samples within the range
      // closer volume is more denserly sampled
      t0 = t0 * t0;
      t1 = t1 * t1;

      t0 = tMaxFloor * t0;
      if (t1 > 1.0f) {
        t1 = tMax;
      } else {
        t1 = tMaxFloor * t1;
      }

      t = t0 + (t1 - t0) * pixelNoise; // dither sample position
      dt = t1 - t0;
    } else {
      t = tMax * (sampleI + pixelNoise) / sampleCount;
    }

    float3 p = worldPos + t * worldDir;
    float pHeight = length(p); // planetCenter to p distance

    // sample medium
    MediumSample medium = SampleAtmosphereMedium(p);
    const float3 sampleOpticalDepth =
        medium.extinction * dt * aerialPerspectiveViewDistanceScale;
    const float3 sampleTransmittance = exp(-sampleOpticalDepth);
    opticalDepth += sampleOpticalDepth;

    // total transmittance from camera to sample point
    throughputMieOnly *= exp(-(medium.extinctionMie + medium.extinctionOzo) *
                             dt * aerialPerspectiveViewDistanceScale);
    throughputRayOnly *= exp(-(medium.extinctionRay + medium.extinctionOzo) *
                             dt * aerialPerspectiveViewDistanceScale);

    const float3 upVector = p / pHeight;
    float lightZenithCosAngle = dot(lightDir, upVector);
    float3 transmittanceToLight =
        GetTransmittance(lightZenithCosAngle, pHeight);

    float3 phaseTimesScattering;
    float3 phaseTimesScatteringMieOnly;
    float3 phaseTimesScatteringRayOnly;
    if (mieRayPhase) {
      phaseTimesScatteringMieOnly = medium.scatteringMie * miePhaseValue;
      phaseTimesScatteringRayOnly = medium.scatteringRay * rayleighPhaseValue;
      phaseTimesScattering =
          phaseTimesScatteringMieOnly + phaseTimesScatteringRayOnly;
    } else {
      phaseTimesScatteringMieOnly = medium.scatteringMie * uniformPhase;
      phaseTimesScatteringRayOnly = medium.scatteringRay * uniformPhase;
      phaseTimesScattering = medium.scattering * uniformPhase;
    }

    float3 multiscatteredLuminance = 0.f;

#ifdef MULTISCATTERING_APPROX_SAMPLING_ENABLED
    multiscatteredLuminance = GetMultipleScattering(p, lightZenithCosAngle);
#endif

    // planet shadow
    float tPlanet0 = RaySphereIntersectNearest(
        p, lightDir, planetO + PLANET_RADIUS_OFFSET * upVector,
        Atmosphere.bottomRadius);
    float planetShadow = tPlanet0 >= 0.f ? 0.f : 1.f;
    bool bUnused = false;

    // if we sample shadow map、 vuolume shadow here, shadowp is needed
    // float3 shadowp = p;
    // if skyview lut pass, shadowp need to be updated

    // note: opaque shadow, volume shadow skyao are emitted here
    {
      // sample opaque object shadow...

      // sample cloud sky ao...

      // sample volumetic cloud shadow...

      // planetShadow *= opaqueShaodw * cloudShadow
      // multipleScattering * cloudSkyAO
    }

    // multiscattering luminance and atmosphere light luminance must be
    // preexposed planet shadow does not affect multiplescattering
    float3 s = exposedLightIlluminance *
               (planetShadow * transmittanceToLight * phaseTimesScattering +
                multiscatteredLuminance * medium.scattering);
    float3 sMieOnly =
        exposedLightIlluminance *
        (planetShadow * transmittanceToLight * phaseTimesScatteringMieOnly +
         multiscatteredLuminance * medium.scatteringMie);
    float3 sRayOnly =
        exposedLightIlluminance *
        (planetShadow * transmittanceToLight * phaseTimesScatteringRayOnly +
         multiscatteredLuminance * medium.scatteringRay);

    // See slide 28 at
    // http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
#define MULTI_SCATTEIRNG_POWER_SERIE 0
#if MULTI_SCATTERING_POWER_SERIE == 0
    result.multiScatAs1 += throughput * medium.scattering * 1.f * dt;
#else
    float3 Ms = medium.scattering * 1;
    float3 msInt = (ms - ms * sampleTransmittance) / medium.extinction;
    result.multicatAs1 += throughput * msInt;
#endif

    // See slide 28 at
    // http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
    float3 sInt = (s - s * sampleTransmittance) / medium.extinction;
    float3 sIntMieOnly =
        (sMieOnly - sMieOnly * sampleTransmittance) / medium.extinction;
    float3 sIntRayOnly =
        (sRayOnly - sRayOnly * sampleTransmittance) / medium.extinction;
    L += throughput * sInt;
    LMieOnly += throughput * sIntMieOnly;
    LRayOnly += throughput * sIntRayOnly;
    throughput *= sampleTransmittance;

    tPrev = t;
  }

  // ray stop at planet ground
  if (ground && tMax == tBottom) {
    float3 p = worldPos + tBottom * worldDir;
    float pHeight = length(p);

    const float3 upVector = p / pHeight;
    float lightZenithCosAngle = dot(lightDir, upVector);
    float3 transmittanceToLight =
        GetTransmittance(lightZenithCosAngle, pHeight);

    const float nDotL = saturate(dot(upVector, lightDir));
    // reflectedLight = groundBRDF * receivedLight * T
    // T = T_toLight * T_camToPoint = transmittanceToLight * throughput
    // receivedLight = lightIlluminance * nDotL
    // groundBRDF = groundAlbedo / PI
    L += lightIlluminance * transmittanceToLight * throughput * nDotL *
         Atmosphere.groundAlbedo_multiScatteringFactor.xyz / PI;
  }

  result.L = L;
  result.LMieOnly = LMieOnly;
  result.LRayOnly = LRayOnly;
  result.opticalDepth = opticalDepth;
  result.transmittance = throughput * planetOnOpaque;
  result.transmittanceMieOnly = throughputMieOnly * planetOnOpaque;
  result.transmittanceRayOnly = throughputRayOnly * planetOnOpaque;

  return result;
}


float4 GetAerialPerspectiveLuminanceTransmittance(
    bool viewIsRealTimeReflectionCapture,               
    float4 cameraAerialPerspectiveVolumeSizeAndInvSize,
    float4 NDC,
    float3 worldPositionRelativeToCamera,
    Texture3D<float4> aerialPerspectiveVolumeTexture,
    SamplerState aerialPerspectiveVolumeSampler,
    float aerialPerspectiveVolumeDepthResolutionInv,
    float aerialPerspectiveVolumeDepthResolution,
    float aerialPerspectiveVolumeStartDepth,
    float aerialPerspectiveVolumeDepthSliceLength,
    float aerialPerspectiveVolumeDepthSliceLengthInv,
    float oneOverExposure,
    float nearFadeOutRangeInvDepth
)
{
    float2 screenUV = (NDC.xy / NDC.ww) * float2(0.5f, -0.5f) + 0.5f;

    float tDepth = max(0.f, length(worldPositionRelativeToCamera) - aerialPerspectiveVolumeStartDepth);

    float linearSlice = tDepth * aerialPerspectiveVolumeDepthSliceLengthInv;
    float linearW = linearSlice * aerialPerspectiveVolumeDepthResolutionInv;
    float nonLinW = sqrt(linearW);
    float nonLinSlice = nonLinW * aerialPerspectiveVolumeDepthResolution;

    const float halfSliceDepth = 0.70710678118654752440084436210485f;// sqrt(0.5f)
    float weight = 1.f;
    if(nonLinSlice < halfSliceDepth)
    {
        // we multiply by weight to fade to 0 at depth 0. it works for luminance and opacity
        weight = saturate(nonLinSlice * nonLinSlice * 2.f);
    }
    weight *= saturate(tDepth * nearFadeOutRangeInvDepth);
    
    #if AP_SUPPORT_REALTIME_REFLECTION_CAPTURE
        // todo: compute real time capture uv
    #endif

    float4 AP = aerialPerspectiveVolumeTexture.SampleLevel(aerialPerspectiveVolumeSampler, float3(screenUV, nonLinW), 0.f);

    // lerp to no contribution near the camera
    AP.rgb *= weight;
    AP.a = 1.f - (weight * (1.f - AP.a));


    // todo: 
    // debug code from ue, cant compile here
    #if 0
	    AP.rgba *= frac(clamp(NonLinSlice, 0, AerialPerspectiveVolumeDepthResolution));
	    AP.r += LinearW <= 0.0f ? 0.5f : 0.0f;
	    AP.g += LinearW >= 1.0f ? 0.5f : 0.0f;
	    AP.b += Weight  <  1.0f ? 0.2f+0.2f*Weight : 0.0f;        
    #endif

    AP.rgb *= oneOverExposure;
    return AP;
}
