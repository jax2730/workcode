// Shadow_Light_Function_PS.inl

float calculateLightAttenuation(
    float sqrDist, 
    float sqrInnerRadii, 
    float invDsqrRadii, 
    float cosInnerSpotLightAngle, 
    float invDeltaCosInnerOuterAngle,
    float deltaAngleCos)
{
    float fAtten = 1.0 - saturate((sqrDist - sqrInnerRadii) * invDsqrRadii);
    fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
    return fAtten * fAtten;
}

#ifdef USE_DPSM
	#if MAX_POINT_LIGHT_PROBE_COUNT > 0
		Texture2D S_Dpsm0;
		SamplerComparisonState S_Dpsm0SamplerState;
	#endif

	#if MAX_POINT_LIGHT_PROBE_COUNT == 2
		static const float4 PointShadowAtlasTransform[2] = { float4(0.5, 1.0, 0.0, 0.0), float4(0.5, 1.0, 0.5, 0.0) };
	#elif MAX_POINT_LIGHT_PROBE_COUNT == 4
		static const float4 PointShadowAtlasTransform[4] = { float4(0.5, 0.5, 0.0, 0.0), float4(0.5, 0.5, 0.5, 0.0), float4(0.5, 0.5, 0.0, 0.5), float4(0.5, 0.5, 0.5, 0.5) };
	#endif

	#define FOR_EACH_PROBE(CAL_SHADOW_LIGHT)																	\
			[unroll]																							\
			for(int i = 0; i < MAX_POINT_LIGHT_PROBE_COUNT; ++i)												\
			{																									\
				float3 pointLightDir = U_ShadowLightWorldPosArray[i].xyz - worldPos.xyz;						\
				float sqrPointLightDist = dot(pointLightDir, pointLightDir);									\
				pointLightDir = normalize(pointLightDir);														\
																												\
				float cosAngle = dot(U_ShadowLightDirectionOuterAngleArray[i].xyz, -pointLightDir);				\
				[branch]																						\
				if (sqrPointLightDist < U_ShadowLightDiffuseColorArray[i].w && 									\
					cosAngle > U_ShadowLightDirectionOuterAngleArray[i].w)										\
				{																								\
					float4 lightSpacePosition = mul(worldPos, U_ShadowLightViewMatrixArray[i]);					\
																												\
					ParaboloidMapping(lightSpacePosition, U_ShadowLightMappingArray[i].x,						\
						U_ShadowLightMappingArray[i].z, U_ShadowLightMappingArray[i].w);						\
																												\
					lightSpacePosition.xy = lightSpacePosition.xy * float2(0.5, -0.5) + float2(0.5, 0.5);		\
					lightSpacePosition.z += U_ShadowLightMappingArray[i].y * 									\
						(1.0 - saturate(dot(N, U_ShadowLightDirectionOuterAngleArray[i].xyz)));					\
																												\
					float3 pointLightColor = U_ShadowLightDiffuseColorArray[i].xyz;								\
																												\
					pointLightColor *= calcPointLightShadowFactor(												\
						S_Dpsm0,																				\
						S_Dpsm0SamplerState,																	\
						lightSpacePosition.xyz,																	\
						U_ShadowTexOffset.w,																	\
						PointShadowAtlasTransform[i]);															\
																												\
					float lightAttn = calculateLightAttenuation(												\
						sqrPointLightDist,																		\
						U_ShadowLightWorldPosArray[i].w,														\
						U_ShadowLightInnerAngleArray[i].z,														\
						U_ShadowLightInnerAngleArray[i].x,														\
						U_ShadowLightInnerAngleArray[i].y,														\
						cosAngle);																				\
																												\
					pointLightColor *= lightAttn;																\
					CAL_SHADOW_LIGHT																			\
				}																								\
			}
#else
	#define FOR_EACH_PROBE(CAL_SHADOW_LIGHT)																	\
			[unroll]																							\
			for(int i = 0; i < MAX_POINT_LIGHT_PROBE_COUNT; ++i)												\
			{																									\
				float3 pointLightDir = U_ShadowLightWorldPosArray[i].xyz - worldPos.xyz;						\
				float sqrPointLightDist = dot(pointLightDir, pointLightDir);									\
				pointLightDir = normalize(pointLightDir);														\
																												\
				float cosAngle = dot(U_ShadowLightDirectionOuterAngleArray[i].xyz, -pointLightDir);				\
				[branch]																						\
				if (sqrPointLightDist < U_ShadowLightDiffuseColorArray[i].w && 									\
					cosAngle > U_ShadowLightDirectionOuterAngleArray[i].w)										\
				{																								\
					float3 pointLightColor = U_ShadowLightDiffuseColorArray[i].xyz;								\
																												\
					float lightAttn = calculateLightAttenuation(												\
						sqrPointLightDist,																		\
						U_ShadowLightWorldPosArray[i].w,														\
						U_ShadowLightInnerAngleArray[i].z,														\
						U_ShadowLightInnerAngleArray[i].x,														\
						U_ShadowLightInnerAngleArray[i].y,														\
						cosAngle);																				\
																												\
					pointLightColor *= lightAttn;																\
					CAL_SHADOW_LIGHT																			\
				}																								\
			}
#endif

float3 calculateShadowPBRPointLight(
	float4 worldPos,
	float3 N,
	float3 V,
	float3 albedo,
	float roughness,
	float metallic)
{
	float3 result = float3(0.0, 0.0, 0.0);
	FOR_EACH_PROBE(result += calculateLightPower(N, V, pointLightDir, pointLightColor, albedo, roughness, metallic);)	
	return result;
}

PBRLightWithSpecularOutput calculateShadowPBRPointLightAndSpecular(
	float4 worldPos,
	float3 N,
	float3 V,
	float3 albedo,
	float roughness,
	float metallic)
{
	PBRLightWithSpecularOutput result;
	result.color = float3(0,0,0);
	result.specular = 0.0;

	FOR_EACH_PROBE(																								\
		float4 lightPower = calculateLightPowerWithSpecularPower(												\
			N, V, pointLightDir, pointLightColor, albedo, roughness, metallic, lightAttn);						\
		result.color += lightPower.xyz;																			\
		result.specular += lightPower.w;)
	return result;
}

PointLightAndSpecularOutput calculateShadowPointLightAndSpecular(
	float4 worldPos,
	float3 N,
	float3 V,
	float glossiness)
{
	PointLightAndSpecularOutput result;
	result.color = float3(0.0, 0.0, 0.0);
	result.specular = float3(0.0, 0.0, 0.0);
	FOR_EACH_PROBE(																								\
		float pfNDotL = saturate(dot(N, pointLightDir));														\
		result.color += pfNDotL * pointLightColor;																\
		float pfNdotH = saturate(dot(normalize(V + pointLightDir), N));											\
		result.specular += pow(pfNdotH, glossiness) * pointLightColor;)
	return result;
}

PointLightAndSpecularOutput calculateShadowPointLightAndSpecularByHair(
	float4 worldPos,
	float3 N,
	float3 V,
	float3 tangent,
	float3 tangentSec,
	float3 specColor,
	float3 specColorSec,
	float glossiness)
{
	PointLightAndSpecularOutput result;
    result.color = float3(0,0,0);
    result.specular = float3(0,0,0);
	FOR_EACH_PROBE(																								\
        float NdotL = saturate(dot(N, pointLightDir));															\
        float NdotV = dot(N, V);																				\
        pointLightColor *= NdotL;																				\
        float3 halfVec = normalize(pointLightDir + V);															\
        float hairSpec = KajiyaKaySpec(tangent, halfVec, glossiness);											\
        hairSpec += KajiyaKaySpec(tangentSec, halfVec, glossiness);												\
        result.specular += hairSpec * pointLightColor * NdotV * NdotL;)
	return result;
}

PointLightAndSpecularFactorOutput calculateShadowPointLightAndSpecularFactor(
	float4 worldPos,
	float3 N,
	float3 V,
	float glossiness)
{
	PointLightAndSpecularFactorOutput result;
	result.color = float3(0.0, 0.0, 0.0);
	result.specularFactor = 0.0;
																												
	FOR_EACH_PROBE(																								\
		float pfNDotL = saturate(dot(N, pointLightDir));														\
		result.color += pfNDotL * pointLightColor;																\
		float pfNdotH = saturate(dot(normalize(V + pointLightDir), N));											\
		result.specularFactor += pow(pfNdotH, glossiness);)
	return result;
}

float3 calculateShadowPointLight(float4 worldPos, float3 N)
{
	float3 result = float3(0.0, 0.0, 0.0);
	FOR_EACH_PROBE(																								\
		float pfNDotL = saturate(dot(N, pointLightDir));														\
		result += pfNDotL * pointLightColor;)
	return result;
}

float3 calculateShadowPointLightForVolumetricCloud(float4 worldPos, float3 N)
{
	float3 result = float3(0.0, 0.0, 0.0);
	FOR_EACH_PROBE(result += pointLightColor;)
	return result;
}

#undef FOR_EACH_PROBE
