float sharpShadow(float x, float mul)
{
	return x > 0.5?pow((x - 0.5) * 2.0 * pow(0.5, mul), 1 / mul) + 0.5 : -pow(-(x - 0.5) * 2 * pow(0.5, mul), 1.0 / mul) + 0.5;
}
	
float2 TranformToAtlasUv(float2 tileUv, float2 mul, float2 add)
{
	tileUv.x = saturate(tileUv.x);
	tileUv.y = saturate(tileUv.y);
	return tileUv * mul + add;
}

float3 CombineShadowcoordComponents(float2 baseUV, float2 deltaUV, float depth, float4 atlasTrans)
{
	float3 uv = float3(baseUV + deltaUV, depth);
	uv.xy = TranformToAtlasUv(uv.xy, atlasTrans.xy, atlasTrans.zw);
	return uv;
}

float GetReceiverPlaneDepthBias(float3 shadowCoord, float2 texelSize)
{
	if (U_ShadowUsePlaneDepthBias.x == 0)
	{
		return 0.0f;
	}

	float3 texCoordDX = ddx(shadowCoord);
	float3 texCoordDY = ddy(shadowCoord);
    float2 receiverPlaneDepthBias;
    receiverPlaneDepthBias.x = texCoordDY.y * texCoordDX.z - texCoordDX.y * texCoordDY.z;
    receiverPlaneDepthBias.y = texCoordDX.x * texCoordDY.z - texCoordDY.x * texCoordDX.z;
    receiverPlaneDepthBias *= 1.0f / ((texCoordDX.x * texCoordDY.y) - (texCoordDX.y * texCoordDY.x));

    // Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
    float fractionalSamplingError = dot(texelSize, abs(receiverPlaneDepthBias));
    float bias = -min(fractionalSamplingError, 0.01f);
#ifdef REVERSE_DEPTH
	bias *= -1;
#endif
	return bias;
}
	
float InternalGetAreaAboveFirstTexelUnderAIsocelesRectangleTriangle(float triangleHeight)
{
	return triangleHeight - 0.5;
}

void InternalGetAreaPerTexel_3TexelsWideTriangleFilter(float offset, out float4 computedArea, out float4 computedAreaUncut)
{
	//Compute the exterior areas
	float offset01SquaredHalved = (offset + 0.5) * (offset + 0.5) * 0.5;
	computedAreaUncut.x = computedArea.x = offset01SquaredHalved - offset;
	computedAreaUncut.w = computedArea.w = offset01SquaredHalved;

	//Compute the middle areas
	//For Y : We find the area in Y of as if the left section of the isoceles triangle would
	//intersect the axis between Y and Z (ie where offset = 0).
	computedAreaUncut.y = InternalGetAreaAboveFirstTexelUnderAIsocelesRectangleTriangle(1.5 - offset);
	//This area is superior to the one we are looking for if (offset < 0) thus we need to
	//subtract the area of the triangle defined by (0,1.5-offset), (0,1.5+offset), (-offset,1.5).
	float clampedOffsetLeft = min(offset,0);
	float areaOfSmallLeftTriangle = clampedOffsetLeft * clampedOffsetLeft;
	computedArea.y = computedAreaUncut.y - areaOfSmallLeftTriangle;

	//We do the same for the Z but with the right part of the isoceles triangle
	computedAreaUncut.z = InternalGetAreaAboveFirstTexelUnderAIsocelesRectangleTriangle(1.5 + offset);
	float clampedOffsetRight = max(offset,0);
	float areaOfSmallRightTriangle = clampedOffsetRight * clampedOffsetRight;
	computedArea.z = computedAreaUncut.z - areaOfSmallRightTriangle;
}

/**
 * Assuming a isoceles triangle of 1.5 texels height and 3 texels wide lying on 4 texels.
 * This function return the weight of each texels area relative to the full triangle area.
 */
void InternalGetWeightPerTexel_3TexelsWideTriangleFilter(float offset, out float4 computedWeight)
{
	float4 dummy;
    InternalGetAreaPerTexel_3TexelsWideTriangleFilter(offset, computedWeight, dummy);
    computedWeight *= 0.44444;//0.44 == 1/(the triangle area)
}

void InternalGetWeightPerTexel_5TexelsWideTriangleFilter(float offset, out float3 texelsWeightsA, out float3 texelsWeightsB)
{
	//See InternalGetAreaPerTexel_3TexelTriangleFilter for details.
	float4 computedArea_From3texelTriangle;
	float4 computedAreaUncut_From3texelTriangle;
	InternalGetAreaPerTexel_3TexelsWideTriangleFilter(offset, computedArea_From3texelTriangle, computedAreaUncut_From3texelTriangle);

	//Triangle slop is 45 degree thus we can almost reuse the result of the 3 texel wide computation.
	//the 5 texel wide triangle can be seen as the 3 texel wide one but shifted up by one unit/texel.
	//0.16 is 1/(the triangle area)
	texelsWeightsA.x = 0.16 * (computedArea_From3texelTriangle.x);
	texelsWeightsA.y = 0.16 * (computedAreaUncut_From3texelTriangle.y);
	texelsWeightsA.z = 0.16 * (computedArea_From3texelTriangle.y + 1);
	texelsWeightsB.x = 0.16 * (computedArea_From3texelTriangle.z + 1);
	texelsWeightsB.y = 0.16 * (computedAreaUncut_From3texelTriangle.z);
	texelsWeightsB.z = 0.16 * (computedArea_From3texelTriangle.w);
}

/**
* Assuming a isoceles triangle of 3.5 texel height and 7 texels wide lying on 8 texels.
* This function return the weight of each texels area relative to the full triangle area.
*  /           \
* _ _ _ _ _ _ _ _ <-- texels
* 0 1 2 3 4 5 6 7 <-- computed area indices (in texelsWeights[])
*/
// https://github.com/TwoTailsGames/Unity-Built-in-Shaders/blob/master/CGIncludes/UnityShadowLibrary.cginc#L473
void InternalGetWeightPerTexel_7TexelsWideTriangleFilter(float offset, out float4 texelsWeightsA, out float4 texelsWeightsB)
{
	float4 computedArea_From3texelTriangle;
    float4 computedAreaUncut_From3texelTriangle;
	InternalGetAreaPerTexel_3TexelsWideTriangleFilter(offset, computedArea_From3texelTriangle, computedAreaUncut_From3texelTriangle);

	//Triangle slop is 45 degree thus we can almost reuse the result of the 3 texel wide computation.
    //the 7 texel wide triangle can be seen as the 3 texel wide one but shifted up by two unit/texel.
    //0.081632 is 1/(the triangle area)
    texelsWeightsA.x = 0.081632 * (computedArea_From3texelTriangle.x);
    texelsWeightsA.y = 0.081632 * (computedAreaUncut_From3texelTriangle.y);
    texelsWeightsA.z = 0.081632 * (computedAreaUncut_From3texelTriangle.y + 1);
    texelsWeightsA.w = 0.081632 * (computedArea_From3texelTriangle.y + 2);
    texelsWeightsB.x = 0.081632 * (computedArea_From3texelTriangle.z + 2);
    texelsWeightsB.y = 0.081632 * (computedAreaUncut_From3texelTriangle.z + 1);
    texelsWeightsB.z = 0.081632 * (computedAreaUncut_From3texelTriangle.z);
    texelsWeightsB.w = 0.081632 * (computedArea_From3texelTriangle.w);
}

float PCF_Process3x3Tent(Texture2D shadowMap, SamplerComparisonState smSamplerState, float4 coord, float4 shadowMapSize, float4 atlasTrans)
{
	float shadow = 1;

	// tent base is 3x3 base thus covering from 9 to 12 texels, thus we need 4 bilinear PCF fetches
    float2 tentCenterInTexelSpace = coord.xy * shadowMapSize.zw;
    float2 centerOfFetchesInTexelSpace = floor(tentCenterInTexelSpace + 0.5);
    float2 offsetFromTentCenterToCenterOfFetches = tentCenterInTexelSpace - centerOfFetchesInTexelSpace;

	// find the weight of each texel based
    float4 texelsWeightsU, texelsWeightsV;
    InternalGetWeightPerTexel_3TexelsWideTriangleFilter(offsetFromTentCenterToCenterOfFetches.x, texelsWeightsU);
    InternalGetWeightPerTexel_3TexelsWideTriangleFilter(offsetFromTentCenterToCenterOfFetches.y, texelsWeightsV);
	
	// each fetch will cover a group of 2x2 texels, the weight of each group is the sum of the weights of the texels
    float2 fetchesWeightsU = texelsWeightsU.xz + texelsWeightsU.yw;
    float2 fetchesWeightsV = texelsWeightsV.xz + texelsWeightsV.yw;

	// move the PCF bilinear fetches to respect texels weights
    float2 fetchesOffsetsU = texelsWeightsU.yw / fetchesWeightsU.xy + float2(-1.5,0.5);
    float2 fetchesOffsetsV = texelsWeightsV.yw / fetchesWeightsV.xy + float2(-1.5,0.5);
    fetchesOffsetsU *= shadowMapSize.xx;
    fetchesOffsetsV *= shadowMapSize.yy;

	// fetch !
    float2 bilinearFetchOrigin = centerOfFetchesInTexelSpace * shadowMapSize.xy;
	float3 combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.x, fetchesOffsetsV.x), coord.z, atlasTrans);
	shadow = fetchesWeightsU.x * fetchesWeightsV.x * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);
	
	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.y, fetchesOffsetsV.x), coord.z, atlasTrans);
	shadow += fetchesWeightsU.y * fetchesWeightsV.x * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.x, fetchesOffsetsV.y), coord.z, atlasTrans);
	shadow += fetchesWeightsU.x * fetchesWeightsV.y * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.y, fetchesOffsetsV.y), coord.z, atlasTrans);
	shadow += fetchesWeightsU.y * fetchesWeightsV.y * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	return shadow;
}

float PCF_Process5x5Tent(Texture2D shadowMap, SamplerComparisonState smSamplerState, float4 coord, float4 shadowMapSize, float4 atlasTrans)
{
	float shadow = 1;

    // tent base is 5x5 base thus covering from 25 to 36 texels, thus we need 9 bilinear PCF fetches
    float2 tentCenterInTexelSpace = coord.xy * shadowMapSize.zw;
    float2 centerOfFetchesInTexelSpace = floor(tentCenterInTexelSpace + 0.5);
    float2 offsetFromTentCenterToCenterOfFetches = tentCenterInTexelSpace - centerOfFetchesInTexelSpace;

    // find the weight of each texel based on the area of a 45 degree slop tent above each of them.
    float3 texelsWeightsU_A, texelsWeightsU_B;
    float3 texelsWeightsV_A, texelsWeightsV_B;
    InternalGetWeightPerTexel_5TexelsWideTriangleFilter(offsetFromTentCenterToCenterOfFetches.x, texelsWeightsU_A, texelsWeightsU_B);
    InternalGetWeightPerTexel_5TexelsWideTriangleFilter(offsetFromTentCenterToCenterOfFetches.y, texelsWeightsV_A, texelsWeightsV_B);

    // each fetch will cover a group of 2x2 texels, the weight of each group is the sum of the weights of the texels
    float3 fetchesWeightsU = float3(texelsWeightsU_A.xz, texelsWeightsU_B.y) + float3(texelsWeightsU_A.y, texelsWeightsU_B.xz);
    float3 fetchesWeightsV = float3(texelsWeightsV_A.xz, texelsWeightsV_B.y) + float3(texelsWeightsV_A.y, texelsWeightsV_B.xz);

    // move the PCF bilinear fetches to respect texels weights
    float3 fetchesOffsetsU = float3(texelsWeightsU_A.y, texelsWeightsU_B.xz) / fetchesWeightsU.xyz + float3(-2.5,-0.5,1.5);
    float3 fetchesOffsetsV = float3(texelsWeightsV_A.y, texelsWeightsV_B.xz) / fetchesWeightsV.xyz + float3(-2.5,-0.5,1.5);
    fetchesOffsetsU *= shadowMapSize.xxx;
    fetchesOffsetsV *= shadowMapSize.yyy;

    // fetch !
    float2 bilinearFetchOrigin = centerOfFetchesInTexelSpace * shadowMapSize.xy;
	float3 combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.x, fetchesOffsetsV.x), coord.z, atlasTrans);
	shadow =  fetchesWeightsU.x * fetchesWeightsV.x * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.y, fetchesOffsetsV.x), coord.z, atlasTrans);
	shadow += fetchesWeightsU.y * fetchesWeightsV.x * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.z, fetchesOffsetsV.x), coord.z, atlasTrans);
	shadow += fetchesWeightsU.z * fetchesWeightsV.x * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);
	
   	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.x, fetchesOffsetsV.y), coord.z, atlasTrans);
	shadow += fetchesWeightsU.x * fetchesWeightsV.y * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

    combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.y, fetchesOffsetsV.y), coord.z, atlasTrans);
	shadow += fetchesWeightsU.y * fetchesWeightsV.y * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

    combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.z, fetchesOffsetsV.y), coord.z, atlasTrans);
	shadow += fetchesWeightsU.z * fetchesWeightsV.y * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.x, fetchesOffsetsV.z), coord.z, atlasTrans);
	shadow += fetchesWeightsU.x * fetchesWeightsV.z * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.y, fetchesOffsetsV.z), coord.z, atlasTrans);
	shadow += fetchesWeightsU.y * fetchesWeightsV.z * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.z, fetchesOffsetsV.z), coord.z, atlasTrans);
	shadow +=  fetchesWeightsU.z * fetchesWeightsV.z * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);
	

    return shadow;
}
	
/**
* PCF tent shadowmap filtering based on a 7x7 kernel (optimized with 16 taps)
*/
// https://github.com/TwoTailsGames/Unity-Built-in-Shaders/blob/master/CGIncludes/UnityShadowLibrary.cginc#L624
float PCF_Process7x7Tent(Texture2D shadowMap, SamplerComparisonState smSamplerState, float4 coord, float4 shadowMapSize, float4 atlasTrans)
{
	float shadow = 1;

    // tent base is 7x7 base thus covering from 49 to 64 texels, thus we need 16 bilinear PCF fetches
    float2 tentCenterInTexelSpace = coord.xy * shadowMapSize.zw;
    float2 centerOfFetchesInTexelSpace = floor(tentCenterInTexelSpace + 0.5);
    float2 offsetFromTentCenterToCenterOfFetches = tentCenterInTexelSpace - centerOfFetchesInTexelSpace;
	
    // find the weight of each texel based on the area of a 45 degree slop tent above each of them.
    float4 texelsWeightsU_A, texelsWeightsU_B;
    float4 texelsWeightsV_A, texelsWeightsV_B;
    InternalGetWeightPerTexel_7TexelsWideTriangleFilter(offsetFromTentCenterToCenterOfFetches.x, texelsWeightsU_A, texelsWeightsU_B);
    InternalGetWeightPerTexel_7TexelsWideTriangleFilter(offsetFromTentCenterToCenterOfFetches.y, texelsWeightsV_A, texelsWeightsV_B);

    // each fetch will cover a group of 2x2 texels, the weight of each group is the sum of the weights of the texels
    float4 fetchesWeightsU = float4(texelsWeightsU_A.xz, texelsWeightsU_B.xz) + float4(texelsWeightsU_A.yw, texelsWeightsU_B.yw);
    float4 fetchesWeightsV = float4(texelsWeightsV_A.xz, texelsWeightsV_B.xz) + float4(texelsWeightsV_A.yw, texelsWeightsV_B.yw);

    // move the PCF bilinear fetches to respect texels weights
    float4 fetchesOffsetsU = float4(texelsWeightsU_A.yw, texelsWeightsU_B.yw) / fetchesWeightsU.xyzw + float4(-3.5,-1.5,0.5,2.5);
    float4 fetchesOffsetsV = float4(texelsWeightsV_A.yw, texelsWeightsV_B.yw) / fetchesWeightsV.xyzw + float4(-3.5,-1.5,0.5,2.5);
    fetchesOffsetsU *= shadowMapSize.xxxx;
    fetchesOffsetsV *= shadowMapSize.yyyy;

	// fetch !
	float2 bilinearFetchOrigin = centerOfFetchesInTexelSpace * shadowMapSize.xy;

	float3 combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.x, fetchesOffsetsV.x), coord.z, atlasTrans);
	shadow = fetchesWeightsU.x * fetchesWeightsV.x * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.y, fetchesOffsetsV.x), coord.z, atlasTrans);
	shadow += fetchesWeightsU.y * fetchesWeightsV.x * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.z, fetchesOffsetsV.x), coord.z, atlasTrans);
	shadow += fetchesWeightsU.z * fetchesWeightsV.x * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.w, fetchesOffsetsV.x), coord.z, atlasTrans);
	shadow += fetchesWeightsU.w * fetchesWeightsV.x * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.x, fetchesOffsetsV.y), coord.z, atlasTrans);
	shadow += fetchesWeightsU.x * fetchesWeightsV.y * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.y, fetchesOffsetsV.y), coord.z, atlasTrans);
	shadow += fetchesWeightsU.y * fetchesWeightsV.y * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.z, fetchesOffsetsV.y), coord.z, atlasTrans);
	shadow += fetchesWeightsU.z * fetchesWeightsV.y * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.w, fetchesOffsetsV.y), coord.z, atlasTrans);
	shadow += fetchesWeightsU.w * fetchesWeightsV.y * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.x, fetchesOffsetsV.z), coord.z, atlasTrans);
	shadow += fetchesWeightsU.x * fetchesWeightsV.z * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.y, fetchesOffsetsV.z), coord.z, atlasTrans);
	shadow += fetchesWeightsU.y * fetchesWeightsV.z * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.z, fetchesOffsetsV.z), coord.z, atlasTrans);
	shadow += fetchesWeightsU.z * fetchesWeightsV.z * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.w, fetchesOffsetsV.z), coord.z, atlasTrans);
	shadow += fetchesWeightsU.w * fetchesWeightsV.z * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.x, fetchesOffsetsV.w), coord.z, atlasTrans);
	shadow += fetchesWeightsU.x * fetchesWeightsV.w * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.y, fetchesOffsetsV.w), coord.z, atlasTrans);
	shadow += fetchesWeightsU.y * fetchesWeightsV.w * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.z, fetchesOffsetsV.w), coord.z, atlasTrans);
	shadow += fetchesWeightsU.z * fetchesWeightsV.w * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	combinedUV = CombineShadowcoordComponents(bilinearFetchOrigin, float2(fetchesOffsetsU.w, fetchesOffsetsV.w), coord.z, atlasTrans);
	shadow += fetchesWeightsU.w * fetchesWeightsV.w * shadowMap.SampleCmpLevelZero(smSamplerState, combinedUV.xy, combinedUV.z);

	return shadow;
}

#ifdef USESHADOWMAP	
	#define FILTER_ACROSS_CASCADES 1

	// ---------------Textures and Samplers----------------
	#if SHADOWSPLITE > 0
		Texture2D S_ShadowMap0;
		SamplerComparisonState S_ShadowMap0SamplerState;
	#endif

	float sampleCascadeShadowMap(float4 mapSize, float4 lightSpacePos, float ShadowBias, int index)
	{
		float4 ProjCoords;
		ProjCoords.xyz = lightSpacePos.xyz;
		ProjCoords.z += ShadowBias;
		ProjCoords.w = 1.0;

		float4 atlasTransform;
#if SHADOWSPLITE == 0
		return 1.0;
#elif SHADOWSPLITE == 2
		atlasTransform.xy = float2(0.5, 1.0);
		atlasTransform.zw = float2((index % 2) * 0.5, 0.0);
#elif SHADOWSPLITE == 4
		atlasTransform.xy = float2(0.5, 0.5);
		atlasTransform.zw = float2((index % 2) * 0.5, (index >> 1) * 0.5);
#endif
		int PCFKernal = int(U_CascadeShadowSettings[0].w);
		switch (PCFKernal)
		{
			case 0:
				return PCF_Process7x7Tent(S_ShadowMap0, S_ShadowMap0SamplerState, ProjCoords, mapSize, atlasTransform);
			case 1:
				return PCF_Process5x5Tent(S_ShadowMap0, S_ShadowMap0SamplerState, ProjCoords, mapSize, atlasTransform);
			case 2:
				return PCF_Process3x3Tent(S_ShadowMap0, S_ShadowMap0SamplerState, ProjCoords, mapSize, atlasTransform);
			default:
				return 1.0;
		}

		return 1.0;
	}

	//-----------------------------------------------------
	float calcShadowFactor(float4 lightSpacePosArry[SHADOWSPLITE], float viewZ, float4 ShadowBias
#ifdef REVERSE_DEPTH
	= (+1e-05).xxxx
#else
	= (-1e-05).xxxx
#endif
	)
	{
		float4 _ShadowMapTexture_TexelSize;
		_ShadowMapTexture_TexelSize.zw = U_CascadeShadowSettings[0].xx;
		_ShadowMapTexture_TexelSize.xy = U_CascadeShadowSettings[0].yy;
		
		[unroll]
		for (int i = 0; i < SHADOWSPLITE; i++)
		{
			ShadowBias[i] += GetReceiverPlaneDepthBias(lightSpacePosArry[i].xyz, _ShadowMapTexture_TexelSize.xy);
		}

		const float BlendThreshold = 0.1f;
		float shadowFactor = 1.0f;
		float fadeDis = 0.0;

	#if SHADOWSPLITE > 0
		if(viewZ < U_ShadowSplit.x)
		{
			shadowFactor = sampleCascadeShadowMap(_ShadowMapTexture_TexelSize, lightSpacePosArry[0], ShadowBias[0], 0);
			fadeDis = 1.0 - viewZ / U_ShadowSplit.x;

		#if SHADOWSPLITE > 1 && FILTER_ACROSS_CASCADES
			if (fadeDis <= BlendThreshold)
			{
				float lerpAmt = smoothstep(0.0f, BlendThreshold, fadeDis);
				float shadowFactorNext = sampleCascadeShadowMap(_ShadowMapTexture_TexelSize, lightSpacePosArry[1], ShadowBias[1], 1);
				shadowFactor = lerp(shadowFactorNext, shadowFactor, lerpAmt);
			}
		#endif
		}
	#endif
	#if SHADOWSPLITE > 1
		else if(viewZ < U_ShadowSplit.y)
		{
			shadowFactor = sampleCascadeShadowMap(_ShadowMapTexture_TexelSize, lightSpacePosArry[1], ShadowBias[1], 1);
			fadeDis = 1.0 - (viewZ - U_ShadowSplit.x) / (U_ShadowSplit.y - U_ShadowSplit.x);

		#if SHADOWSPLITE > 2 && FILTER_ACROSS_CASCADES
			if (fadeDis <= BlendThreshold)
			{
				float lerpAmt = smoothstep(0.0f, BlendThreshold, fadeDis);
				float shadowFactorNext = sampleCascadeShadowMap(_ShadowMapTexture_TexelSize, lightSpacePosArry[2], ShadowBias[2], 2);
				shadowFactor = lerp(shadowFactorNext, shadowFactor, lerpAmt);
			}
		#endif
		}
	#endif
	#if SHADOWSPLITE > 2
		else if(viewZ < U_ShadowSplit.z)
		{
			shadowFactor = sampleCascadeShadowMap(_ShadowMapTexture_TexelSize, lightSpacePosArry[2], ShadowBias[2], 2);
			fadeDis = 1.0 - (viewZ - U_ShadowSplit.y) / (U_ShadowSplit.z - U_ShadowSplit.y);

		#if SHADOWSPLITE > 3 && FILTER_ACROSS_CASCADES
			if (fadeDis <= BlendThreshold)
			{
				float lerpAmt = smoothstep(0.0f, BlendThreshold, fadeDis);
				float shadowFactorNext = sampleCascadeShadowMap(_ShadowMapTexture_TexelSize, lightSpacePosArry[3], ShadowBias[3], 3);
				shadowFactor = lerp(shadowFactorNext, shadowFactor, lerpAmt);
			}
		#endif
		}
	#endif
	#if SHADOWSPLITE > 3
		else if(viewZ < U_ShadowSplit.w)
		{
			shadowFactor = sampleCascadeShadowMap(_ShadowMapTexture_TexelSize, lightSpacePosArry[3], ShadowBias[3], 3);
			fadeDis = 1.0 - (viewZ - U_ShadowSplit.z) / (U_ShadowSplit.w - U_ShadowSplit.z);

		#if FILTER_ACROSS_CASCADES
			if (fadeDis <= BlendThreshold)
			{
				float lerpAmt = smoothstep(0.0f, BlendThreshold, fadeDis);
				float shadowFactorNext = 1.0;
				shadowFactor = lerp(shadowFactorNext, shadowFactor, lerpAmt);
			}
		#endif
		}
	#endif
	
		//shadowFactor = sharpShadow(shadowFactor, lerp(1.0, 1.8, fadeDis));
		float strength = U_ShadowTexOffset.w;	

		return (shadowFactor * strength + (1.0f - strength));
	}
#endif

#ifdef USE_DPSM
	float calcPointLightShadowFactor(
		Texture2D shadowMap, 
		SamplerComparisonState cmpSampler, 
		float3 coord,
		float strength,
		float4 atlasTranform)
	{
		uint uw = 512, uh = 512;
		float4 mapSize = float4(1.0 / float(uw), 1.0 / float(uh), float(uw), float(uh));
		float dpsmShadowFactor = PCF_Process5x5Tent(shadowMap, cmpSampler, float4(coord, 1.0), mapSize, atlasTranform);
		//dpsmShadowFactor = sharpShadow(dpsmShadowFactor, 1.0);
		return (dpsmShadowFactor * strength + (1.0f - strength));
	}
#endif
