#include <Common_Uniform_PS.inl>
#include <Common_Function.inl>
#include <Common_ShadowFiltering.inl>
#include <Light_Function_PS.inl>
#include <EnvironmentLight_Function.inl>
#include <Common_Function_PS.inl>

#ifndef BUMP_MAP
	#undef RIPPLE
#endif

#ifdef SHADOWPASS
	#ifdef ALPHA_TEST
		float4	U_MaterialDiffuse;
		Texture2D	S_Diffuse;
		SamplerState S_DiffuseSamplerState;
		
		float4 texDiffuse(float2 vUV) 
		{ 
			return S_Diffuse.Sample(S_DiffuseSamplerState, vUV); 
		}
	#endif
#else

	float4	U_MaterialDiffuse;
	float4	U_MaterialSpecular;
	float4  U_MaterialFresnel;	
	float4	U_MaterialCustomParam;
	float4	U_MaterialCustomParam1;
	

	float4	U_LightPower;	//x for lightpower, y for specular offset
	
	float4	U_PSGeneralRegister0;
	float4	U_PSGeneralRegister1;
	float4	U_PSGeneralRegister2;
	float4	U_PSGeneralRegister3;
	float4	U_PSGeneralRegister4;
	float4	U_PSGeneralRegister5;
	float4	U_PSGeneralRegister6;
	float4	U_PSGeneralRegister7;
	float4	U_PSGeneralRegister8;
	float4	U_PSGeneralRegister30;

	float4	U_PSGeneralRegister29; 	//x ---current mask percentage
									//y --- if 1 on Lattice Cull Mask
									//z ---alpha range
									//w ---direction if w >= 0 Positive elseif w < 0 Reverse 
	
	Texture2D	S_Diffuse;
	SamplerState S_DiffuseSamplerState;
	
	#ifdef RIPPLE
		Texture2D	S_Ripple;
		SamplerState S_RippleSamplerState;
		
		float4 ComputeRipple(float2 uv, float t)
		{
			//波纹贴图采样，并把采样的高度值扩展到-1到1。
			float4 ripple = S_Ripple.Sample(S_RippleSamplerState, uv);
			float gloss = ripple.y;
			ripple.yz = ripple.yz * 2.0 - 1.0;
			//获取波纹的时间,从A通道获取不同的波纹时间,
			float dropFrac = frac(ripple.a + t);
			//把时间限制在R通道内	
			float timeFrac = dropFrac - 1.0 + ripple.x;
			//做淡出处理
			float dropFactor = 1.0f-saturate( dropFrac);
			//计算最终的高度，用一个sin计算出随时间的振幅	 
			float final = dropFactor* sin(clamp(timeFrac * 9.0, 0.0, 4.0) * 3.1415926);
			return float4(ripple.yz * final, 1.0, (final ) * 0.2);
		}
	#endif
	
	Texture2D	S_Specular;
	SamplerState S_SpecularSamplerState;

	#ifdef DISSOLVE
			Texture2D S_Custom7;
			SamplerState S_Custom7SamplerState;
	#endif	

	#ifdef SOFTBOARD
		Texture2D	S_Depth;
		SamplerState S_DepthSamplerState;
	#endif
	
	Texture2D	S_Bump;
	SamplerState S_BumpSamplerState;
	TextureCube	S_EnvironmentCubeMap;
	SamplerState S_EnvironmentCubeMapSamplerState;
	
	float4 texDiffuse(float2 vUV) 
	{ 
		return S_Diffuse.Sample(S_DiffuseSamplerState, vUV); 
	}

	#ifdef DETAIL_BUMP
		#undef TEXTURE_FLOW
		Texture2D	S_Custom;
		SamplerState S_CustomSamplerState;
	#endif
	
	#ifdef TEXTURE_FLOW
		#undef DETAIL_BUMP
		Texture2D	S_Custom;
		SamplerState S_CustomSamplerState;
	#endif	
		
	#ifdef DISTORT
		#undef DETAIL_BUMP
	#endif
	
	#ifdef TEXTURE_MASK
		Texture2D	S_Custom2;
		SamplerState S_Custom2SamplerState;
	#endif

	#ifdef CLUSTER_POINT_LIGHT
		#include "Shadow_Light_Function_PS.inl"
	#endif

	#ifdef SOFTBOARD
		float getSceneDepth(in float4 clipPos, in float2 uv)
		{
			float4 tempPos = float4(uv, 0.0, 1.0);
			tempPos.z = S_Depth.Sample(S_DepthSamplerState, uv).r;
			
			tempPos.x = clipPos.x;
			tempPos.y = clipPos.y;
			
			float4 viewSpacePos;
			viewSpacePos.x = dot(U_InvProjectMatrix[0], tempPos);
			viewSpacePos.y = dot(U_InvProjectMatrix[1], tempPos);
			viewSpacePos.z = dot(U_InvProjectMatrix[2], tempPos);
			viewSpacePos.w = dot(U_InvProjectMatrix[3], tempPos);
			float depth = viewSpacePos.z / viewSpacePos.w;
			
			return depth;
		}
		
		//Dx only need to sample one times.
		float calcSceneDepth(in float4 clipPos)
		{	
			float2 screenTC = clipPos.xy / clipPos.w;
			screenTC.x = (screenTC.x + 1.0) * 0.5  + U_ViewParam.z;
			screenTC.y = 1.0 - (screenTC.y + 1.0) * 0.5 + U_ViewParam.w;
			screenTC *= U_ViewParam.xy;
			float depth = getSceneDepth(clipPos / clipPos.w, screenTC);
			
			return depth;
		}
	#endif	
#endif

#ifdef HAVEPOINTLIGHT
	float4	U_LightWorldPosiArray[LightSimulation];
	float4	U_LightDiffuseColorArray[LightSimulation]; 

	float GetPointLightAttenuation(in float3 vDir, in float fInnerRadius, float fOuterRadius)
	{
		float fSqrLen = dot(vDir, vDir);
		float fSqrInnerRadius = fInnerRadius * fInnerRadius;
		float fSqrOuterRadius = fOuterRadius * fOuterRadius;
		float fAtten = 1.0 - saturate((fSqrLen - fSqrInnerRadius) / (fSqrOuterRadius - fSqrInnerRadius));
		return fAtten * fAtten;
	}
#endif

#ifdef DISTORT
 
float3 mod289(float3 x)
{
    return x - floor(x / 289.0) * 289.0;
}

float2 mod289(float2 x)
{
    return x - floor(x / 289.0) * 289.0;
}

float3 permute(float3 x)
{
    return mod289((x * 34.0 + 1.0) * x);
}

float3 taylorInvSqrt(float3 r)
{
    return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(float2 v)
{
    const float4 C = float4( 0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                             0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                            -0.577350269189626,  // -1.0 + 2.0 * C.x
                             0.024390243902439); // 1.0 / 41.0
    // First corner
    float2 i  = floor(v + dot(v, C.yy));
    float2 x0 = v -   i + dot(i, C.xx);

    // Other corners
    float2 i1;
    i1.x = step(x0.y, x0.x);
    i1.y = 1.0 - i1.x;

    // x1 = x0 - i1  + 1.0 * C.xx;
    // x2 = x0 - 1.0 + 2.0 * C.xx;
    float2 x1 = x0 + C.xx - i1;
    float2 x2 = x0 + C.zz;

    // Permutations
    i = mod289(i); // Avoid truncation effects in permutation
    float3 p =
      permute(permute(i.y + float3(0.0, i1.y, 1.0))
                    + i.x + float3(0.0, i1.x, 1.0));

    float3 m = max(0.5 - float3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0.0);
    m = m * m;
    m = m * m;

    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    float3 x = 2.0 * frac(p * C.www) - 1.0;
    float3 h = abs(x) - 0.5;
    float3 ox = floor(x + 0.5);
    float3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    m *= taylorInvSqrt(a0 * a0 + h * h);

    // Compute final noise value at P
    float3 g;
    g.x = a0.x * x0.x + h.x * x0.y;
    g.y = a0.y * x1.x + h.y * x1.y;
    g.z = a0.z * x2.x + h.z * x2.y;
    return 130.0 * dot(m, g);
}

float3 snoise_grad(float2 v)
{
    const float4 C = float4( 0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                             0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                            -0.577350269189626,  // -1.0 + 2.0 * C.x
                             0.024390243902439); // 1.0 / 41.0
    // First corner
    float2 i  = floor(v + dot(v, C.yy));
    float2 x0 = v -   i + dot(i, C.xx);

    // Other corners
    float2 i1;
    i1.x = step(x0.y, x0.x);
    i1.y = 1.0 - i1.x;

    // x1 = x0 - i1  + 1.0 * C.xx;
    // x2 = x0 - 1.0 + 2.0 * C.xx;
    float2 x1 = x0 + C.xx - i1;
    float2 x2 = x0 + C.zz;

    // Permutations
    i = mod289(i); // Avoid truncation effects in permutation
    float3 p =
      permute(permute(i.y + float3(0.0, i1.y, 1.0))
                    + i.x + float3(0.0, i1.x, 1.0));

    float3 m = max(0.5 - float3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0.0);
    float3 m2 = m * m;
    float3 m3 = m2 * m;
    float3 m4 = m2 * m2;

    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    float3 x = 2.0 * frac(p * C.www) - 1.0;
    float3 h = abs(x) - 0.5;
    float3 ox = floor(x + 0.5);
    float3 a0 = x - ox;

    // Normalise gradients
    float3 norm = taylorInvSqrt(a0 * a0 + h * h);
    float2 g0 = float2(a0.x, h.x) * norm.x;
    float2 g1 = float2(a0.y, h.y) * norm.y;
    float2 g2 = float2(a0.z, h.z) * norm.z;

    // Compute noise and gradient at P
    float2 grad =
      -6.0 * m3.x * x0 * dot(x0, g0) + m4.x * g0 +
      -6.0 * m3.y * x1 * dot(x1, g1) + m4.y * g1 +
      -6.0 * m3.z * x2 * dot(x2, g2) + m4.z * g2;
    float3 px = float3(dot(x0, g0), dot(x1, g1), dot(x2, g2));
    return 130.0 * float3(grad, dot(m4, px));
}
#endif

float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0-F0) * pow(1.0-cosTheta, 5.0);
}

float distributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;
	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = 3.141592654 * denom * denom;
	denom = max(denom, 0.00001);
	return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	
	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;
	
	return nom / denom;
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = geometrySchlickGGX(NdotV, roughness);
	float ggx1 = geometrySchlickGGX(NdotL, roughness);
	
	return ggx2 * ggx1;
}

float3 envBRDFApprox(float3 specularColor, float roughness, float NoV)
{
	float4 c0 = float4(-1.0,	-0.0275,	-0.572,		0.022);
	float4 c1 = float4(1.0,		0.0425,		1.04,		-0.04);
	
	float4 r = roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	float2 AB = float2(-1.04, 1.04) * a004 + r.zw;
	return specularColor * AB.x + AB.y;
}

//float3 envBRDFApprox(float3 specularColor, float roughness, float NoV)
//{
//   return pow(1.0f-max(roughness,NoV),3.f)+specularColor;
//}

float3 thinTranslucency(float3 N, float3 L, float3 transmittanceColor)
{
	float transmittance = saturate(dot(-N, L) + 0.2);
	float fresnel = (1.0 - transmittance) * (1.0 - transmittance);
	fresnel *= fresnel;
	
	return transmittance * (1.0 - fresnel) * transmittanceColor;
}

struct PS_INPUT
{
    float4	i_PosInClip		: SV_POSITION;
#ifdef SHADOWPASS
	#ifdef ALPHA_TEST
		float4 	i_UV		: TEXCOORD0;
	#endif
#else
	float4 	i_UV			: TEXCOORD0;	// w: ViewSpaceZ if USESHADOWMAP
	float4 	i_Fog			: TEXCOORD1;
	float4	i_Normal		: TEXCOORD2;
	float4	i_WorldPos		: TEXCOORD3;
	
	float4	i_Tangent		: TEXCOORD4;
	float4	i_Binormal		: TEXCOORD5;
	#ifdef USESHADOWMAP
		float4  i_LightSpacePos[SHADOWSPLITE]   : TEXCOORD6;
	#endif
	
	#ifdef VERTEX_COLOR
		float4	i_Diffuse	: COLOR0;
	#endif
	#ifdef SOFTBOARD
		float4 i_ClipPos    : TEXCOORD8;
	#endif
	#ifdef MOTIONBLUR
		float4  i_VelUV     : TEXCOORD15;
	#endif
#endif
#ifdef PARABOLOID_MAPPING
	float	i_ClipDepth			: TEXCOORD12;
#endif
	float4  i_PosInModel	: TEXCOORD13;
	float4  i_BoundBoxRange     : TEXCOORD14;
};

struct PS_OUTPUT
{
#ifndef SHADOWPASS
	float4 color : SV_TARGET;
#endif
};

PS_OUTPUT main(PS_INPUT IN)
{
	PS_OUTPUT psOut = (PS_OUTPUT)0;

#ifdef PARABOLOID_MAPPING
	clip(IN.i_ClipDepth);
#endif

#ifdef SHADOWPASS
	#ifdef ALPHA_TEST
		float4 color = texDiffuse(IN.i_UV.xy);
		clip(color.a - U_MaterialDiffuse.a);
	#endif
#else
	#ifdef MOTIONBLUR
		float2 vQX = IN.i_VelUV.xy;
	
		float fLenQX = length(vQX);
		vQX /= (fLenQX+0.01f);
		float fWeight = max(0.5f, min(fLenQX, 20.0f));
		vQX *= fWeight;
		psOut.color.xy = vQX;
		return psOut;
	#endif
	float2 uv = IN.i_UV.xy;
	
	#ifdef DISSOLVE
	
		if(U_MaterialCustomParam1.y > 0)
		{
			float4x4 thresholdMatrix =
			{  1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
			  13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
			   4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
			  16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
			};
			float2 pos = IN.i_PosInClip.xy;
			
			
			pos.xy = pos.xy/U_MaterialCustomParam1.z;
			
			int xindex = fmod(pos.x, 4);
			int yindex =fmod(pos.y, 4);
			
			float alpha = thresholdMatrix[xindex][yindex];//* _RowAccess[yindex];
			
			if(alpha < U_MaterialCustomParam1.y)
				discard;
		}
		else
		{
			if(S_Custom7.Sample(S_Custom7SamplerState, uv).a < U_MaterialFresnel.y)
			discard;	
		}

		bool needDiscard = ComputeBitmapCullMask(U_PSGeneralRegister29, IN.i_BoundBoxRange, 
		U_MaterialCustomParam1.z, IN.i_PosInModel.xyz, IN.i_PosInClip.xyz);
		if(needDiscard)
			discard;
	#endif
	#ifdef UV_ROLL
		uv += frac(U_PS_Time.z * float2(U_PSGeneralRegister1.z, U_PSGeneralRegister1.w));
	#endif
	
	#ifdef DISTORT
		float _distortAmt = U_PSGeneralRegister2.y;
		float _distortScale = U_PSGeneralRegister2.z;
		float _distortSpeed = U_PSGeneralRegister1.y;
		float Time = U_PS_Time.z;
 
		float noise = snoise(uv * _distortScale + float2(Time *_distortSpeed, - Time *_distortSpeed));
		
		uv += noise * _distortAmt;
	#endif

	#ifdef UV_TILLING
		uv *= U_PSGeneralRegister2.x;
	#endif
		
	half4 diffuseColor = S_Diffuse.Sample(S_DiffuseSamplerState, uv);
				
	#ifdef ALPHA_TEST
		clip(diffuseColor.a - U_MaterialDiffuse.a);
	#endif

	#ifdef VERTEX_COLOR
		diffuseColor.rgb *= IN.i_Diffuse.rgb;
	#endif
	
	half3 baseMap = pow(diffuseColor.rgb, 2.2);
	baseMap = baseMap *U_MaterialDiffuse.rgb;
		
	#ifdef TEXTURE_MASK
		half4 mask = S_Custom2.Sample(S_Custom2SamplerState, uv);
		half3 colorR = pow(U_PSGeneralRegister4.rgb, 2.2) * mask.r;
		half3 colorG = pow(U_PSGeneralRegister5.rgb, 2.2) * mask.g;
		half3 colorB = pow(U_PSGeneralRegister6.rgb, 2.2) * mask.b;
		float f = min(mask.r + mask.g + mask.b, 1.0);
		half3 colorF = colorR + colorG + colorB;
		//half3 lumWeight = half3(0.2125f, 0.7154f, 0.0721f);
		//float hslcolor = dot(baseMap.rgb, lumWeight);
		//hslcolor += mask.w;
		//hslcolor = min(hslcolor, 1.0);
		//half3 maskColor = lerp(baseMap, hslcolor * colorF, f);
		baseMap = lerp(baseMap, baseMap * colorF, f);

	#endif
	
	half3 N = normalize(IN.i_Normal.xyz);

	half3 V = normalize(U_PS_CameraPosition.xyz - IN.i_WorldPos.xyz);
	
	#ifdef BUMP_MAP
		half3 nn = normalize(IN.i_Normal.xyz);
		half3 tn = normalize(IN.i_Tangent.xyz);
		half3 bn = normalize(IN.i_Binormal.xyz);

		tn = normalize(tn - nn * dot(tn, nn));
		bn = normalize(bn - nn * dot(bn, nn));

		half3x3 tbn = 
		{
			tn.x, bn.x, nn.x,
			tn.y, bn.y, nn.y,
			tn.z, bn.z, nn.z 
		};
		
		half4 bumpColor = S_Bump.Sample(S_BumpSamplerState, uv); 

		half3 bump = bumpColor.rgb * 2.0 - 1.0;	
		bump *= half3(U_PSGeneralRegister2.w, U_PSGeneralRegister2.w, 1.0);
		bump.z = 1.h;

		#ifdef DETAIL_BUMP
			float2 detailUV = U_PSGeneralRegister2.y;
			
			#ifdef UV_TILLING
				detailUV /= U_PSGeneralRegister2.x;
			#endif
			
			half4 detailBumpColor = S_Custom.Sample(S_CustomSamplerState, uv * detailUV);
			bump.xy += (detailBumpColor.xy - half2(0.5, 0.5)) * U_PSGeneralRegister2.z;
		#endif
		//当有涟漪的时候，减弱模型原本的法线贴图效果，或者直接砍掉法线贴图的计算
		#ifdef RIPPLE
			float s = clamp(dot(N, float3(0.0,1.0,0.0)), 0.0, 1.0) ;
			s = pow(s, 5.0);
			s *= (1.f - U_PSGeneralRegister30.x);
			bump.xy  *= (1.0 - s * 0.9f);
			double time = U_PS_Time.x;
			float time01 = U_PS_Time.y * 0.1591549; //U_PS_Time.y / PI / 2
			//涟漪整体的透明度使用sin函数进行0-1的变化，当透明度变为0的时候，坐标进行偏移，以减轻雨滴的重复感。单张贴图透明度为0的时候会没有涟漪，所以采样两次，交替显示
			float2 rUV = IN.i_WorldPos.xz * 0.25f;
			
			float rOffset = floor(U_PS_Time.x * 0.5f) * 0.1f;
			float4 ripple = ComputeRipple(rUV + rOffset, time01 * 3.0f);
			
			rUV = IN.i_WorldPos.xz * 0.25f + 0.25f;
			rOffset = floor(U_PS_Time.x * 0.5f + 0.25f) * 0.1f;
			float4 ripple2 = ComputeRipple(rUV + float2(rOffset, -rOffset), time01 * 3.0f);
			
			float rLerp1 =  sin(time01 * 3.1415);
			float rLerp2 =  1.0 - rLerp1;
			bump.xy += rLerp1 * ripple.xy * s;
			bump.xy += rLerp2 * ripple2.xy * s;
		#endif

		bump = normalize(bump);		
		N = mul(tbn, bump);
	#endif
	
	half metallic = 0.0;
	half roughness = 1.0;
	
	#ifdef PBR_ROUGHNESS
		#ifdef BUMP_MAP
			roughness = bumpColor.b;
		#endif
	#else
		roughness = U_PSGeneralRegister6.w;
	#endif
	
	float wetFinal = U_PSGeneralRegister8.w;
	roughness -= wetFinal;
	//有涟漪的时候，给材质的高光整体加0.4		
	#ifdef RIPPLE
		roughness = clamp(roughness - 0.4f * (1.f - U_PSGeneralRegister30.x), 0.0, 1.0);
	#endif
	roughness = max(roughness , 1e-6);
	metallic = lerp(metallic, 0.5, wetFinal);
	
#ifdef COVER_WEATHER
	#ifdef COVER_WEATHER_ALPHA
		float coverLerp = diffuseColor.a;
	#else
		float coverLerp = dot(N, U_PS_GravityDirection.xyz);
	#endif
	coverLerp = clamp(coverLerp + U_PSGeneralRegister8.x, 0.0, 1.0);
	coverLerp *= U_WeatherParam0.w * (1.f - U_PSGeneralRegister30.x);
	baseMap = lerp(baseMap, U_WeatherParam0.xyz, coverLerp);
	roughness = lerp(roughness, U_WeatherParam1.x, coverLerp);
	metallic = lerp(metallic, U_WeatherParam1.y, coverLerp);		
#endif
	
	half3 color = 0.0;
	
	half3 L = normalize(-U_MainLightDirection.xyz);
	
	#ifdef	THIN_TRANSLUCENCY
		color += thinTranslucency(N, L, U_PSGeneralRegister7.xyz) * metallic;
		metallic = 0.0;
	#endif
	
	half3 albedo = baseMap * (1.0 - metallic);
	half3 specularColor = lerp(half3(0.04h, 0.04h, 0.04h), baseMap, metallic);	
	
	color += calculateLightPower(N, V, L, U_PBR_MainLightColor.xyz * (1.f - U_PS_ViewLightPercent.x), baseMap, roughness, metallic);

	half shadowOffset = 0.0;
	//half lightlum = GetLuminance(U_PBR_MainLightColor.xyz);
	
	#ifdef USESHADOWMAP
		float shadowFactor = calcShadowFactor(
				IN.i_LightSpacePos, 
				IN.i_UV.w);
			shadowFactor += shadowOffset;
			shadowFactor = min(shadowFactor, 1.0);
			color *= shadowFactor;
	#endif

	color += calculateLightPower(N, V, -U_PS_ViewDirection.xyz, U_PBR_MainLightColor.xyz * U_PS_ViewLightPercent.x, baseMap, roughness, metallic);
	
	#ifdef HAVEPOINTLIGHT
		[unroll]
		for(int i = 0; i < LightSimulation; ++i)
		{
			half3 pointLightDir = U_LightWorldPosiArray[i].xyz - IN.i_WorldPos.xyz;
			half lightattenua = 0;
			lightattenua = GetPointLightAttenuation(pointLightDir, U_LightWorldPosiArray[i].w, U_LightDiffuseColorArray[i].w);
			half3 pointLightColor = U_LightDiffuseColorArray[i].xyz * lightattenua;
			//shadowOffset += GetLuminance(pointLightColor) / lightlum;
			pointLightDir = normalize(pointLightDir);
			color += calculateLightPower(N, V, pointLightDir, pointLightColor, baseMap, roughness, metallic);	
		}
	#endif

#ifdef CLUSTER_POINT_LIGHT
	color += calculateShadowPBRPointLight(IN.i_WorldPos, N, V, baseMap, roughness, metallic);

        float3 clusterLightColor = 
            calculateClusterPBRPointLight(IN.i_PosInClip.xy,
                                          IN.i_WorldPos.xyz,
                                          N,
                                          V,
                                          baseMap,
                                          roughness,
                                          metallic);
        color += clusterLightColor;    
#endif

	
	half3 envBRDF = envBRDFApprox(specularColor, roughness, max(dot(N, V), 0.0));
	
	half3 ambientColor = ComputeAmbientLight(U_PBR_SkyLightColor.xyz, U_PBR_GroundLightColor.xyz, N, U_PS_AmbientBlendThreshold.y, U_PS_GravityDirection.xyz);
	#ifdef ENVIRONMENT_MAP
		half4 refl;
		refl.xyz = reflect(-V, N);
		refl.w = U_EnvironmentCubeMapMipLevels.x - 1.0 - (1.0 - 1.2 * log2(roughness));
		refl.w = max(0.0, refl.w);
		half3 reflColor = S_EnvironmentCubeMap.SampleLevel(S_EnvironmentCubeMapSamplerState, refl.xyz,refl.w).rgb;
		envBRDF *= reflColor;
	#else
		envBRDF *= ambientColor;
	#endif
	
	color += (albedo + envBRDF) * ambientColor;
	
	#ifdef EMISSIVE
		
		half3 missiveColor = diffuseColor.rgb * U_PSGeneralRegister0.xyz * diffuseColor.a;
		half3 minMissiveColor = missiveColor * U_PSGeneralRegister0.w;

		#ifdef TOD_EMISSIVE
			half todFactor = U_LightPower.x;		
			
			#ifdef DYNAMIC_EMISSIVE
				half3 maxMissiveColor = missiveColor * U_PSGeneralRegister1.x;
				half factor = U_MaterialSpecular.w;	
				color += lerp(minMissiveColor, maxMissiveColor, half3(factor, factor, factor)) * todFactor;
			#else
				color += minMissiveColor * todFactor;
			#endif
		#else
		
			#ifdef DYNAMIC_EMISSIVE
				half3 maxMissiveColor = missiveColor * U_PSGeneralRegister1.x;
				half factor = U_MaterialSpecular.w;	
				color += lerp(minMissiveColor, maxMissiveColor, half3(factor, factor, factor));
			#else
				color += minMissiveColor;
			#endif
			
		#endif
		
	#endif
	psOut.color.rgb = color;
	psOut.color.rgb *= U_MaterialCustomParam1.x;	
	
	#ifdef TEXTURE_FLOW
		uv *= U_PSGeneralRegister3.z;
		uv += frac(U_PS_Time.z * float2(U_PSGeneralRegister3.x, U_PSGeneralRegister3.y));
		half3 flowTextureColor = pow(S_Custom.Sample(S_CustomSamplerState, uv).rgb, 2.2);
		
		#ifdef TEXTURE_FLOW_WITHOUT_CHANNEL
			psOut.color.rgb += lerp(flowTextureColor, flowTextureColor * U_PSGeneralRegister3.w, U_LightPower.x);
		#else
			psOut.color.rgb += lerp(flowTextureColor, flowTextureColor * U_PSGeneralRegister3.w, U_LightPower.x) * diffuseColor.a;
		#endif
	#endif

	float alphaaa = 1.0f;
	#ifdef SOFTBOARD
		float pixelDepth = 1.0f;
		float sceneDepth = 1.0f;
		float4 tempPos = 1.0f;
		tempPos = float4(IN.i_ClipPos.xy/IN.i_ClipPos.w, 0.0, 1.0);
		tempPos.x = tempPos.x * 0.5 + 0.5;
		tempPos.y = (1.0 - tempPos.y) * 0.5;
		sceneDepth = calcSceneDepth(IN.i_ClipPos);
		float4 vp;
		vp.x = dot(U_InvProjectMatrix[0], IN.i_ClipPos);
		vp.y = dot(U_InvProjectMatrix[1], IN.i_ClipPos);
		vp.z = dot(U_InvProjectMatrix[2], IN.i_ClipPos);
		vp.w = dot(U_InvProjectMatrix[3], IN.i_ClipPos);
		
		pixelDepth = vp.z / vp.w;
			
		alphaaa = saturate(abs(pixelDepth - sceneDepth) / IN.i_UV.z);
	#endif
	
	psOut.color.rgb = psOut.color.rgb * IN.i_Fog.a + IN.i_Fog.rgb;
	psOut.color.rgb = pow(psOut.color.rgb, 1.0 / 2.2);
	psOut.color.a = diffuseColor.a * alphaaa;
	
	#ifdef EditorHelp
		psOut.color.rgb = psOut.color.rgb * 0.00001 + float3(1.0,0.0,0.0);
		psOut.color.a = 1.0;
	#endif
	
	#ifdef EDITOR_MODE
		if(U_MaterialCustomParam.w > 0)
			psOut.color.rgb = U_MaterialCustomParam.xyz;
		
	#endif
	float overdrawlerp = ceil(U_PS_Time.w);
	psOut.color = psOut.color * (1.0 - overdrawlerp) + float4(U_PS_Time.w * overdrawlerp, U_PS_Time.w * overdrawlerp, U_PS_Time.w * overdrawlerp, overdrawlerp); 
#endif

	return psOut;
}
