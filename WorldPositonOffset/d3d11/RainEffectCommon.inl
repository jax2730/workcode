#ifdef RIPPLE
	Texture2D S_RainEffectMap;
	SamplerComparisonState S_RainEffectMapSamplerState;

    Texture2D	S_Ripple;
	SamplerState S_RippleSamplerState;
		
    Texture2D S_RippleFlow;
    SamplerState S_RippleFlowSamplerState;

	Texture2D S_RippleDrops;
    SamplerState S_RippleDropsSamplerState;

	Texture2D S_RippleNoise;
    SamplerState S_RippleNoiseSamplerState;

#endif

#ifdef RIPPLE
float4 ComputeRipple(float2 uv, float t)
{
	//波纹贴图采样，并把采样的高度值扩展到-1到1。
	float4 ripple =  S_Ripple.Sample(S_RippleSamplerState, uv);
	float gloss = ripple.y;
	ripple.yz = ripple.yz * 2.0 - 1.0;
	//获取波纹的时间,从A通道获取不同的波纹时间,
	float dropFrac = frac(ripple.a + t);
	// dropFrac = (sin(frac(ripple.a + t) * 3.1415 * 2.0)) * 0.5 + 0.5;
	//把时间限制在R通道内	
	float timeFrac = dropFrac - 1.0 + ripple.x - 0.4;
	//做淡出处理
	float dropFactor = 1.0f-saturate(dropFrac);
	//计算最终的高度，用一个sin计算出随时间的振幅	 
	float final = dropFactor* sin(clamp(timeFrac * 20.0, 0.0, 3.0) * 3.1415926);
	// final *= 0.35;
	return float4(ripple.yz * final, 1.0, (final ) * 0.2);
}

void NormalStrength_float(inout float3 In, float Strength)
{
    In = float3(In.rg * Strength, lerp(1.0, In.b, saturate(Strength)));
}


// float sum(float3 v){return v.x + v.y + v.z;}


// void normal_blend_float(float3 A, float3 B, out float3 Out)
// {
//     float3 t = A.xyz + float3(0.0, 0.0, 1.0);
//     float3 u = B.xyz * float3(-1.0, -1.0, 1.0);
//     Out = (t / t.z) * dot(t, u) - u;
// }

// #define IndexRange 12.0
// float4 SampleNormalNoTile(in Texture2D tex, in SamplerState texSamplerState, 
//         in float2 UV, float noise)
// {
// 	// float noise = IndexNoise(UV.xy * frequency);//0.3 is a empirial number
// 	//float noise = valueNoise(UV.xy);

// 	float2 dx = ddx(UV.xy);
// 	float2 dy = ddy(UV.xy);

// 	float l = noise * IndexRange;
// 	float f = frac(l);

// 	float ia = floor(l);
// 	float ib = ia + 1.0;

// 	float2 offa = sin(float2(3.0, 7.0) * ia);
// 	float2 offb = sin(float2(3.0, 7.0) * ib);

// 	float4 texa = tex.SampleGrad(texSamplerState, float2(UV.xy + offa), dx, dy);
// 	float4 texb = tex.SampleGrad(texSamplerState, float2(UV.xy + offb), dx, dy);
// 	// normal_blend_float(texa, texb);
// 	float4 result = lerp(texa, texb, smoothstep(0.2, 0.8, f - 0.1 * sum((texa - texb).xyz)));

// 	return result;
// }


void shading_rain_effect(in float3 uv, in float time, in float3 vertexNormal, in float3 blendWeight, in float2 ripple, out float4 color, out float rainMask)
{
	// Rain Flow
	uv = abs(uv);
	float3 flowUV = uv;
	uv *= 3.2; 
	flowUV *= 4.0;
	float2 lerpUV = 0.0;
	float2 flowLerpUV = 0.0;
	time *= 0.02;
	time *= 4.0;
	float4 colorYX = S_RippleFlow.Sample(S_RippleFlowSamplerState, float2(uv.x, uv.y + time));
    float4 colorYZ = S_RippleFlow.Sample(S_RippleFlowSamplerState, float2(uv.z, uv.y + time));

	float4 colorFlowYX = S_RippleFlow.Sample(S_RippleFlowSamplerState, float2(flowUV.x, flowUV.y + time * 2.0));
    float4 colorFlowYZ = S_RippleFlow.Sample(S_RippleFlowSamplerState, float2(flowUV.z, flowUV.y + time * 2.0));
	
	colorYZ.xy = (colorYZ.xy * 2.0 - 1.0);
	colorYX.xy = (colorYX.xy * 2.0 - 1.0);

	color.xy = colorYZ.xy * (blendWeight.x * colorFlowYZ.a) + colorYX.xy * (blendWeight.y * colorFlowYX.a);
	color.xy = lerp(color.xy, ripple, blendWeight.z);
	color.zw = 1.0;
	NormalStrength_float(color.xyz, 3.5);
	rainMask = 1.0;
	return ;
	float3 newUV = uv * U_WeatherParam2.y;
	// Rain Drops
	float4 dropsOut = S_RippleDrops.Sample(S_RippleDropsSamplerState, float2(newUV.x, newUV.z));
	// float4 dropsOut = SampleNormalNoTile(S_RippleDrops, S_RippleDropsSamplerState, float2(newUV.x, newUV.z), snoise(newUV.xz));

	float3 dropsNormal = float3(dropsOut.xy, 1.0);
	dropsNormal.xy = dropsNormal.xy * 2.0 - 1.0;
	float newTime = time * U_WeatherParam2.z;
	float dynamicDrops = frac(dropsOut.z - newTime * 0.2);
	dynamicDrops *= saturate(dropsOut.w * 2.0 - 1.0);
	
	float staticDrops = saturate(pow((dropsOut.w * 2.0 - 1.0) * -1.0, 10.0));
	float4 dropsAllNormal = float4(dropsNormal.xy * (dynamicDrops + staticDrops), 1.0, 1.0);
	// Rain Pool
	float2 magicLerpRes = lerp(float2(0.00001, 0.0), float2(0.1, 0.25), float2(0.5, 0.5));
	float3 newUV2 = uv * 0.032;

	float4 noiseColor =  S_RippleNoise.Sample(S_RippleNoiseSamplerState, float2(newUV2.x, newUV2.z));
	float wetnessMask = 1.0 - saturate((noiseColor.y - magicLerpRes.y) / (magicLerpRes.y + U_WeatherParam2.x));
	// rainMask = lerp((dynamicDrops + staticDrops), 1.0, wetnessMask);
	rainMask = 1.0;
	// ripple.xy = lerp(dropsAllNormal.xy, ripple, wetnessMask);
	NormalStrength_float(color.xyz, 1.5);
	color.xy = lerp(ripple, color.xy, blendWeight.z);
	color.zw = 1.0;
}



void shading_rain_wetness(inout float3 baseColor, inout float roughness, in float shadowfactor)
{
	float rainStrength = clamp(0.0, 1.0, U_MainLightColor.a);
	float3 simpleGray = 0.451;
	float3 white = 1.0;
	float3 newBaseColor = lerp(white, simpleGray, rainStrength);
	newBaseColor *= baseColor;
	float newRoughness = lerp(roughness, 0.01, rainStrength);
	baseColor = lerp(baseColor, newBaseColor, shadowfactor);
	roughness = lerp(roughness, newRoughness, shadowfactor);
}

void sampler_rain_shadow(float4 lightPos, out float shadowRes)
{
    // Rain Shadow Shading
	float4 Rain_ShadowMapTexture_TexelSize;
	S_RainEffectMap.GetDimensions(Rain_ShadowMapTexture_TexelSize.z, Rain_ShadowMapTexture_TexelSize.w);
	Rain_ShadowMapTexture_TexelSize.xy = 1.0 / Rain_ShadowMapTexture_TexelSize.zw;
	float4 ProjCoords;

	ProjCoords.xyz = lightPos.xyz / lightPos.w;
	ProjCoords.x = 0.5 * ProjCoords.x + 0.5;
	ProjCoords.y = 0.5 - 0.5 * ProjCoords.y;
	ProjCoords.w = 1.0;
	ProjCoords.z += 0.02;
	if(ProjCoords.x > 1.0 || ProjCoords.x < 0.0 || ProjCoords.y > 1.0 || ProjCoords.y < 0.0)
	{
		shadowRes = 1.0;
		return ;
	}
	shadowRes = PCF_Process3x3Tent(S_RainEffectMap, S_RainEffectMapSamplerState, ProjCoords, Rain_ShadowMapTexture_TexelSize, float4(1.0, 1.0, 0.0, 0.0));
	// shadowRes = S_RainEffectMap.SampleCmpLevelZero(S_RainEffectMapSamplerState, ProjCoords.xy + clamp(noiseColor.r, 0.0, 0.2), ProjCoords.z);
}

#endif