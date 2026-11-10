#define HALF_MIN 6.10352e-5f
struct InputData {
  float3 positionWS;
  float4 positionCS;
  float3 normalWS;
#ifdef USESHADOWMAP
  float4 positionLS[SHADOWSPLITE];
  float viewZ;
#endif
  half3 viewDirectionWS;
  float2 uv;
  half visiableValue;
};

struct SurfaceData {
  half3 albedo;
  half3 specular;
  half metallic;
  half3 normalTS;
  half roughness;
  half3 emission;
  half occlusion;
  half alpha;
};

// Light

struct Light {
  half3 direction;
  half3 light_color;
  half3 ambient_color;
  half shadowFactor;
};

struct BRDFData {
  float3 albedo;
  float3 diffuse;
  float3 specular;
  float roughness;
  float roughness2;
  float metallic;
  // float normalizationTerm;  // roughness * 4.0 + 2.0
  // float roughness2MinusOne; // roughness^2 - 1.0
};

#ifdef ENVIRONMENT_MAP
    //glossy info
	TextureCube	S_GlobalEnvironmentCubeMap;
	SamplerState S_GlobalEnvironmentCubeMapSamplerState;
    float4 U_GlobalEnvironmentCubeMipMapLevels;

    //diffuse info
	float4x4 U_PBR_RMatrix;
	float4x4 U_PBR_GMatrix;
	float4x4 U_PBR_BMatrix;

    //total luminance
	float4  U_PBR_Environment_Explosure;
#endif

Light GetMainLight() {
  Light mLight = (Light)0;
  mLight.direction = half3(-U_MainLightDirection.xyz);
#ifdef ALONE_ROLE_LIGHT
  mLight.light_color = U_RoleMainLightColor;
// #else if
//     mLight.color = U_MainLightColor; // temp：default all light source color
//     is pbrMainlight
#else
  mLight.light_color = U_PBR_MainLightColor.xyz;
#endif
  return mLight;
}

Light GetMainLight(InputData inputData, float aoFactor)
{
    Light mLight = GetMainLight();
#ifdef USESHADOWMAP
    mLight.shadowFactor = calcShadowFactor(inputData.positionLS, inputData.viewZ);
    mLight.shadowFactor = min(mLight.shadowFactor, 1.0);
    mLight.light_color *= mLight.shadowFactor;
#endif // USESHADOWMAP
    mLight.light_color *= aoFactor;
    return mLight;
}

void InitBRDFDataDirect(SurfaceData sdata, out BRDFData bData) {
  bData = (BRDFData)0;
  bData.albedo = sdata.albedo;
  bData.diffuse = sdata.specular;
  bData.specular = sdata.specular;
  bData.metallic = sdata.metallic;
  bData.roughness =
      clamp(sdata.roughness, 0.0, 1.0);
  bData.roughness2 =
      max(bData.roughness * bData.roughness, HALF_MIN);
//   bData.normalizationTerm = bData.roughness * half(4.0) + half(2.0);
//   bData.roughness2MinusOne = bData.roughness2 - half(1.0);
}

half3 LightingStandardDirect(BRDFData brdfData, Light light, InputData iData) {

  // calculateLightPower(float3 N, float3 V, float3 L, float3 lightPower, float3
  // albedo, float roughness, float metallic)
  float3 specular = pbr_lighting_fresnel_weighted(float3(iData.normalWS),
               float3(light.direction), float3(iData.viewDirectionWS), brdfData.albedo, brdfData.metallic,
               brdfData.roughness, light.light_color.xyz  * (1.0 - iData.visiableValue));

  specular += pbr_lighting_fresnel_weighted(float3(iData.normalWS),
              float3(-U_PS_ViewDirection.xyz), float3(iData.viewDirectionWS), brdfData.albedo, brdfData.metallic,
              brdfData.roughness, light.light_color.xyz * iData.visiableValue);
  return specular;
}

half3 LightingStandardPoint(BRDFData brdfData, Light light, InputData iData)
{
    float3 specular = 0.0;
#ifdef CLUSTER_POINT_LIGHT
		specular.xyz += calculateShadowPBRPointLight(float4(iData.positionWS, 1.0), iData.normalWS, iData.viewDirectionWS, brdfData.albedo, brdfData.roughness, brdfData.metallic);

        float3 clusterLightColor =
        calculateClusterPBRPointLight(iData.positionCS.xy,
                                        iData.positionWS.xyz,
                                        iData.normalWS,
                                        iData.viewDirectionWS,
                                        brdfData.albedo,
                                        brdfData.roughness,
                                        brdfData.metallic);
        specular.xyz += clusterLightColor;
#endif
    return specular;
}

half3 LightingStandarIndirect(BRDFData brdfData, Light light, InputData iData)
{
    half3 envColor = 0.0;
    half3 ao = 1.0;
    #ifdef ENVIRONMENT_MAP
		envColor += CalculateGlossyEnvColor(iData.normalWS, iData.viewDirectionWS, brdfData.albedo, brdfData.metallic, brdfData.roughness, ao, 0.0,
            0.0, U_GlobalEnvironmentCubeMipMapLevels.x, S_GlobalEnvironmentCubeMap,
            S_GlobalEnvironmentCubeMapSamplerState);
        envColor *= U_PBR_Environment_Explosure.x;
        envColor += CalculateDiffuseEnvColor(iData.normalWS, brdfData.albedo, brdfData.metallic, ao, U_PBR_RMatrix, U_PBR_GMatrix, U_PBR_BMatrix,
            0.0);
    #endif
    return envColor;
}

half3 LightingStandard(BRDFData brdfData, Light light, InputData iData) {
    float3 direct_specular = LightingStandardDirect(brdfData, light, iData);
    float3 point_specular = LightingStandardPoint(brdfData, light, iData);
    float3 envDiffuse = LightingStandarIndirect(brdfData, light, iData);
    return direct_specular + point_specular + envDiffuse;
}

float4 Fragment_PBR_Lighting(InputData idata, SurfaceData sdata) {
    BRDFData brdfData;
    InitBRDFDataDirect(sdata, brdfData);
    Light mainLight = GetMainLight(idata, sdata.occlusion);
    float3 lightingColor = LightingStandard(brdfData, mainLight, idata);

    // add emission color
    lightingColor += sdata.emission;
    // Mix Fog Code to here

    return float4(lightingColor, sdata.alpha);
}
