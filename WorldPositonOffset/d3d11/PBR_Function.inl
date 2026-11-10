
//inl文件不支持嵌套引用，只能把光照相关的全部都写在一个文件里面
#define PI 3.14159265358979f

#define USE_DIFFUSE_AO
#define USE_APPROXED_DIRECT_LIGHTING

struct SurfaceData{
    float3   worldPos;
    float3   N;
    float3   V;
    float3   baseColor;
    float    NdotV;
    float    roughness;
    float    metalness;

    // indirect lighting related values
    float    diffuseAO;          // min(bakedAO, SSAO)
    float3   diffuseAlbedo;      // used by Spherical harmonics lighting
    float    specularAO;         // specular ao should be generated from diffuse ao or be approximated from bent normal
    float3   preIntegratedBRDF;  // used by IBL
};

struct LightData{
    float3   colorIntensity;
    float3   direction;
    float    NdotL;
    float    attenuation;        // point light attenuation
    float    shadowFactor;       // computed by shadowmap
};

struct EnvironmentData{
    float3 skyColor;
    float3 groundColor;
    float  blendThreshold; // angle that skyColor and groundColor start to blend
};

struct EnvironmentProbe{
    //glossy info
    TextureCube IBLCubeMap;
    SamplerState cubeMapSampler;
    float cubemapMipLevel;

    // diffuse info
    float4x4 SH_RMat;
    float4x4 SH_GMat;
    float4x4 SH_BMat;

    //TODO:: explosure of environment map is increct,
    //might be a good chance to fix it in deffered shading
    float environmentExplosure;

    float rotationAngle; // should be computed by Normal and a constant given direction of environment map
                    // this param is not used for now
};

float
D_GGX(float cosThetaM, float alpha){
    float a2 = alpha * alpha;

    float f = saturate((cosThetaM * a2 - cosThetaM) * cosThetaM + 1.f) + 1e-5f;
    return a2 / (PI * f * f);
}

float
V_SmithGGXHeightCorrelated(float NoV, float NoL, float roughness){
    float a2 = roughness * roughness;    //remap roughness to alpha as we did in Distribution function
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5h / max(GGXV + GGXL, 1e-5f);
}

float
Lambertian(){
    return 1.f / PI;
}

float3
F_Schlick(float cosThetaV, float3 f0){
    float invCosThta2 = (1.f - cosThetaV) * (1.f - cosThetaV);
    float invCosTheta5 = invCosThta2 * invCosThta2 * (1.f - cosThetaV);
    return f0 + (1.f - f0) * invCosTheta5;
}

float3
DirectLighting_FresnelWeighted(SurfaceData surface, LightData currentLight)
{
    float3 H = normalize(currentLight.direction + surface.V);
    float NoH = saturate(dot(surface.N, H));
    float VoH = saturate(dot(surface.V, H));

    // just remapping roughness to get linearly transforming effect, no physically meaning
    // see https://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
    float alpha = surface.roughness * surface.roughness;

    float3 f0 = lerp(0.04f, surface.baseColor, surface.metalness);
    float3 F = F_Schlick(VoH, f0);

    // diffuse brdf
    float3 diffuseAlbedo = surface.baseColor * (1.f - surface.metalness); // metal transfer absorbed irradiance into heat instead of scatterring visible light
    float3 lambertianDiffuse = Lambertian() * diffuseAlbedo * (1.f - F);

    // glossy brdf
    float3 cookTorranceGlossy = D_GGX(NoH, alpha) * V_SmithGGXHeightCorrelated(surface.NdotV, currentLight.NdotL, surface.roughness) * F;

    float3 lightReceived = currentLight.colorIntensity * currentLight.attenuation * currentLight.NdotL * currentLight.shadowFactor;
    return lightReceived * (cookTorranceGlossy + lambertianDiffuse);
}

// our current light function used in forward rendering
// See "Optimizing PBR for Mobile" from Siggraph 2015 moving mobile graphics course
// https://community.arm.com/events/1155
float3
DirectLightingApproxed(SurfaceData surface, LightData currentLight)
{
    float3 H = normalize(currentLight.direction + surface.V);
    float NoH = saturate(dot(surface.N, H));
    float LoH = saturate(dot(currentLight.direction, H));

    float a2 = surface.roughness * surface.roughness;
    float a4 = max(a2 * a2, 1e-5f);

    float NdotH2 = NoH * NoH;
    float LdotH2 = LoH * LoH;

    float denom = 4.f * 3.1415926f * pow(NdotH2 * (a4 - 1.f) + 1.f, 2.f) * (LdotH2 * (1.f - a2) + a2) + 1e-5f;
    float brdf  = a4/denom;

    float3 F0 = 0.04f;
	F0 = lerp(F0, surface.baseColor, surface.metalness);

    float3 kd = 1.0f - min(F0 / (LoH + 0.00001f), 1.0f);
    kd = kd * (1.f - surface.metalness);

    float3 lightReceived = currentLight.colorIntensity * currentLight.attenuation * 
        currentLight.NdotL * currentLight.shadowFactor;
    float3 bsdf = surface.baseColor * kd / 3.141592654f + brdf * F0;
    return lightReceived * bsdf;
}

float3
DirectLighting(SurfaceData surface, LightData currentLight)
{
    #ifdef USE_APPROXED_DIRECT_LIGHTING
        return DirectLightingApproxed(surface, currentLight);
    #else
        return DirectLighting_FresnelWeighted(surface, currentLight);
    #endif
}

float3 
ComputeDirectionalLight(SurfaceData surface, LightData mainLight, LightData supplementLight)
{
    return DirectLighting(surface, mainLight) + DirectLighting(surface, supplementLight);
}

float3
ComputeAmbientColorInternal(
    float3 N, 
    float3 groundColor, 
    float3 skyColor, 
    float blendThreshold,
    float3 gravityDir)
{
    float normalY = dot(N, gravityDir);
    float blendParam = smoothstep(1e-5f, 1.f, 
        (normalY * 0.5f + 0.5f * blendThreshold) / (blendThreshold + 1e-5f));
    return lerp(groundColor, skyColor, blendParam);
}

float3
ComputeAmbientColor(in SurfaceData surface, in EnvironmentData environment, in float3 gravityDir)
{
	return ComputeAmbientColorInternal(
        surface.N, 
        environment.groundColor, 
        environment.skyColor,
        environment.blendThreshold,
        gravityDir
	);
}

// compute brdf integral inadvance so that they can be reused when ever needed in GI. (Although we dont have GI for now)
void
ApproxBrdfIntegral(inout SurfaceData surface)
{
    //diffuse
    surface.diffuseAlbedo = surface.baseColor * (1.f - surface.metalness);

    // specular
    // see https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
    // approx the integral of glossy brdf over the reflected lobe
    // Althrough our deffered shading code mainly runs on desktop devices, its still worth using this approximation
    // since it saves one texture binding(our rc is using a static binding of samplers and textuers,
    // so the number of sampler is really limited)
    float3 specularColor = lerp(0.04h, surface.baseColor, surface.metalness);
    const float4 c0 = {-1.0,    -0.0275h,   -0.572,    0.022};
    const float4 c1 = {1.0,     0.0425h,    1.04,      -0.04};
    float4 r = surface.roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * surface.NdotV)) * r.x + r.y;
	float2 AB = float2(-1.04, 1.04) * a004 + r.zw;
	surface.preIntegratedBRDF = specularColor * AB.x + AB.y;
    return;
};

// current specular ao in use
// from substance 3D painter
float
ApproxSpecularAO_Substance(in float diffuseAO, in float roughness, in float metalness){
    return lerp(diffuseAO, 1.0f, metalness * (1.0f - roughness) * (1.0f - roughness));
}

// little bit more expensive than our current approach but more accurate
// Lagarde and de Rousiers 2014, "Moving Frostbite to PBR"
float
ApproxSpecularAO_Lagarde(float NoV, float diffuseAO, float roughness){
    return saturate(pow(NoV + diffuseAO, exp2(16.0f * roughness - 1.0f)) - 1.0f + diffuseAO);
}


#define USE_SIMPLE_SPECULAR_AO // this matches the current effect of IllumPBR2023 in forward rendering
void
ApproxSpecularAO(in SurfaceData surface)
{
    #ifdef USE_DIFFUSE_AO
        surface.specularAO = surface.diffuseAO;
    #else // USE_DIFFUSE_AO
        #ifdef USE_SIMPLE_SPECULAR_AO
            surface.specularAO = ApproxSpecularAO_Substance(surface.diffuseAO, surface.roughness, surface.metalness);
        #else
            surface.specularAO = ApproxSpecularAO_Lagarde(surface.NdotV, surface.diffuseAO, surface.roughness);
        #endif // UUSE_SIMPLE_SPECULAR_AO
    #endif // USE_DIFFUSE_AO
}

// // Jimenez et al. 2016, "Practical Realtime Strategies for Accurate Indirect Occlusion"
// float ApproxSpecularAO_BentNormal(float3 bentNormal, float diffuseAO, float roughness){
//     float cosAv = sqrt(1.0 - diffuseAO);

//     float cosAs = exp2(-3.321928 * sq(roughness));
//     float cosB  = dot(bentNormal, shading_reflected);

// }

// this function is left for further use
// distance lobe environment light should have a constant direction in scene
// as object rotate in world space, environment lighting should rotate with it
// but now we doesn't consider it
float3
WorldToEnvironment(float3 N, float rotateAngle){
	float3x3 rotateMatrix;
	float radianQ = radians(rotateAngle);
	rotateMatrix[0] = float3(cos(radianQ), 0, -sin(radianQ));
	rotateMatrix[1] = float3(0, 1, 0);
	rotateMatrix[2] = float3(sin(radianQ), 0, cos(radianQ));

	return mul(rotateMatrix, N);
}

float3
ApproxDiffuseIrradiance(in SurfaceData surface,in EnvironmentProbe envProbe)
{
    //TODO:: this rotation should be deleted after
    // adjusting the direction of SH lighting from file
    row_major float3x3 rotateYaxisMatrix = {
        0.f,  0.f,  1.f,
        0.f,  1.f,  0.f,
        -1.f, 0.f,  0.f
    };

    //https://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf
    float4 n = float4(mul(rotateYaxisMatrix, surface.N).xzy, 1.0f);
    return max(0.f, float3(
        dot(n, mul(envProbe.SH_RMat, n)),
        dot(n, mul(envProbe.SH_GMat, n)),
        dot(n, mul(envProbe.SH_BMat, n))
    ));
}

float3
EnvironmentLighting_distantProbe_HDR(
    in SurfaceData surface, 
    in EnvironmentProbe envProbe, 
    in EnvironmentData environment,
    float3 gravityDir,
	bool accumulate_glossy = true)
{
    float3 ambientColor = ComputeAmbientColor(surface, environment, gravityDir);

	float3 sh_DiffuseIrradiance = ApproxDiffuseIrradiance(surface, envProbe);
    float3 envi_diffuse = sh_DiffuseIrradiance * surface.diffuseAlbedo;

	if (!accumulate_glossy) 
        return ambientColor * envProbe.environmentExplosure * envi_diffuse;

    float4 refl;
    refl.xyz = reflect(-surface.V, surface.N);
    refl.w = clamp(envProbe.cubemapMipLevel * surface.roughness, 0.0f, envProbe.cubemapMipLevel);

    // approximate irradiance from environment map
    float4 rgbmColor =
        envProbe.IBLCubeMap.SampleLevel(envProbe.cubeMapSampler, refl.xyz, refl.w);
    float3 ibl_GlossyIrradiance = rgbmColor.xyz * rgbmColor.w * 10.f;    // IBL map is encoded in rgbm with the maximum value 10

    // float3 envi_diffuse = sh_DiffuseIrradiance * surface.diffuseAlbedo * surface.diffuseAO;
    // float3 envi_glossy = ibl_GlossyIrradiance * surface.preIntegratedBRDF * surface.specularAO;

    float3 envi_glossy = ibl_GlossyIrradiance * surface.preIntegratedBRDF;

    return ambientColor * envProbe.environmentExplosure * (envi_diffuse + envi_glossy);
}

float3
EnvironmentLighting_distantProbe_LDR(
    in SurfaceData surface, 
    in EnvironmentProbe envProbe, 
    in EnvironmentData environment,
    in float3 gravityDir)
{
    float3 ambientColor = ComputeAmbientColor(surface, environment, gravityDir);

    float4 refl;
    refl.xyz = reflect(-surface.V, surface.N);
    refl.w = envProbe.cubemapMipLevel.x - 1.f - (1.f - 1.2 * log2(surface.roughness));
    refl.w = max(0.f, refl.w);

    half3 reflColor = envProbe.IBLCubeMap.SampleLevel(envProbe.cubeMapSampler, refl.xyz, refl.w).rgb;
    return ambientColor * (surface.diffuseAlbedo + reflColor * surface.preIntegratedBRDF);
}

float3
EnvironmentLighting_DefaultAmbient(
    in SurfaceData surface, 
    in EnvironmentData environment,
    in float3 gravityDir)
{
    float3 ambientColor = ComputeAmbientColor(surface, environment, gravityDir);

    // float3 envi_diffuse = ambientColor * surface.diffuseAlbedo * surface.diffuseAO;
    // float3 envi_glossy = ambientColor * surface.preIntegratedBRDF * surface.specularAO;

    float3 envi_diffuse = ambientColor * surface.diffuseAlbedo;
    float3 envi_glossy = ambientColor * surface.preIntegratedBRDF;

    return ambientColor * (envi_diffuse + envi_glossy);
}
