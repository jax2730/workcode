// some functions used in gbuffer pass
float2 OctWrap(float2 v) {
  return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}

half3 EncodeNormal(float3 n) {
  n /= (abs(n.x) + abs(n.y) + abs(n.z));
  n.xy = n.z >= 0.0 ? n.xy : OctWrap(n.xy);
  n.xy = n.xy * 0.5 + 0.5;

  uint2 i = uint2(n.xy * 4095); // 2^12
  uint3 Out = uint3((i.x & 0xff0) >> 4,
                    ((i.x & 0xf) << 4) + (((i.y) & 0xf00) >> 8), i.y & 0xff);
  return Out / 255.0;
}

struct EncodedEmission {
  half2 emissionColor;
  half emissionPower;
};

// helper function, encode emission color
half2 EncodeEmissionR5G6B5(half3 emissionColor) {
  uint3 iEmission = uint3(
      floor(emissionColor *
            float3(31.h, 63.h, 31.h))); // encode emission into r5g6b5 unorm

  uint2 packedData = uint2(((iEmission.x << 3) & 0xF8) + (iEmission.y >> 3),
                           ((iEmission.y << 5) & 0xE0) + iEmission.z);
  return packedData / 255.0;
}

EncodedEmission EncodeEmission(half3 emission) {
  EncodedEmission enc;
  emission = clamp(emission, 0.0h, 9.0h);
  half power = max(max(emission.x, emission.y), emission.z);
  half3 color = emission / power;
  enc.emissionColor = EncodeEmissionR5G6B5(color);
  enc.emissionPower = log10(power + 1);
  return enc;
}

// for useRoleLight | emissive | receiveShadow | useEnviromentMap |
// usePointLight 0 means disabled 1 means enabled
//
// for lightModel(4 bits)
// 1 is set by default
// 1 means using PBR lighting and HDR IBL
// (IllumPBR2023, FacePBR2023) 2 means using PBR lighting and LDR EnvironmentMap
// (SpecialIllumPBR, Eye)
half EncodeShadingModel(uint useRoleLight, uint receiveShadow,
                        uint useEnvironmentMap, uint usePointLight,
                        uint lightModel = 1) {
  uint code = (useRoleLight & 0x01) | ((receiveShadow << 1) & 0x02) |
              ((useEnvironmentMap << 2) & 0x04) |
              ((usePointLight << 3) & 0x08) | ((lightModel << 4) & 0xF0);
  return half(code) / 255.h;
}

// https://web.archive.org/web/20180423091842/http://www.equasys.de/colorconversion.html
// Ranges:
// R/G/B[0,255]
// Y/Cb/Cr[0,255]
float3 YUVfromRGB(float3 RGB) {
  float3 colorTrans[3];
  colorTrans[0] = float3(0.299, 0.587, 0.114);
  colorTrans[1] = float3(-0.169, -0.331, 0.500);
  colorTrans[2] = float3(0.500, -0.419, -0.081);

  float3 YUV;
  YUV.x = dot(colorTrans[0], RGB);
  YUV.y = dot(colorTrans[1], RGB) + 0.5;
  YUV.z = dot(colorTrans[2], RGB) + 0.5;
  return YUV;
}

half3 dye(in half3 original, in half3 target, in half mask) {
  target = lerp(original, target, mask);
  return original * target;
}

// 染色
half3 dye(in half3 origin,
          in half3 target1, // 颜色1
          in half3 target2, // 颜色2
          in half3 target3, // 颜色3
          in half3 target4, // 颜色4
          in half4 mask     // xyzw分别对应颜色1234的Mask
) {
  half3 dyeColor1 = pow(target1, 2.2) * mask.x;
  half3 dyeColor2 = pow(target2, 2.2) * mask.y;
  half3 dyeColor3 = pow(target3, 2.2) * mask.z;
  half3 dyeColor4 = pow(target4, 2.2) * mask.w;

  half f = min(mask.r + mask.g + mask.b + mask.a, 1.h);
  half3 colorF = dyeColor1 + dyeColor2 + dyeColor3 + dyeColor4;
  return lerp(origin, origin * colorF, f);
}

float3 RotateWithQuaternion(float3 position, float4 quaternion) {
    float3 qVec = quaternion.xyz;
    float qW = quaternion.w;

    // 计算旋转后的向量
    float3 uv = cross(qVec, position);
    float3 uuv = cross(qVec, uv);
    return position + 2.0 * (qW * uv + uuv);
}

float GetGreyLuminance(float3 color)
{
    return 0.2126729f * color.x + 0.7151522f * color.y + 0.0721750f * color.z;
}

// 不清楚如果baseColor,roughness,metallic如果是float类型的话，
// 传一个half 类型的inout会不会出错
void ComputeWeatherCover(
				in half4		weatherParam,	 		// 填U_WeatherParam0
				in half4		weatherParam1, 			// 填U_WeatherParam1 
				in float4		tsPos,  				// 填U_CoverColorParams
				in float4		coverColorRotation,  	// 填U_CoverColorRotation
				in float3		worldPos,
				in float 		coverMask,
				in half			customParam30,			//填U_PSGeneralRegister30.x
				in half			cosTheta,				//法线与重力方向夹角
				in half			customCoverScale,  	
				in half  		customCoverRange, 		
				inout half3		baseColor,
				inout half		roughness,
				inout half		metallic
){
	half coverLerp = cosTheta + customCoverRange;
	coverLerp = clamp(coverLerp, 0.h, 1.h);
	coverLerp = coverLerp * customCoverScale;
	coverLerp *= weatherParam.w * (1.h - customParam30);
	
	half3 weatherColor = weatherParam.xyz;
	half  weatherRoughness = weatherParam1.x;
	half  weatherMetallic = weatherParam1.y;

	roughness = clamp(lerp(roughness, weatherRoughness, coverLerp), 1e-4h, 1.h);
	metallic = lerp(metallic, weatherMetallic, coverLerp);
	if(tsPos.w > 0.00001)
	{
		half maskRes = 1.h;
		half enableCube = 1.0;

		float4 rotVec = float4(worldPos.xyz, 1.0);
		float4 qu = coverColorRotation;
		rotVec.xyz = RotateWithQuaternion(rotVec.xyz, qu);
		float dist = length(worldPos.xyz - tsPos.xyz);
		half sphere_maskRes = half(smoothstep(tsPos.w - 1.f, tsPos.w, dist));
	
		float rectSize = tsPos.w;
		float3 halfSize = rectSize * 0.5f;
		
		// float3 distToEdge = abs(worldPos - tsPos.xyz) - halfSize;
		float3 distToEdge = abs(rotVec.xyz - tsPos.xyz) - halfSize;
		distToEdge.y = (rotVec.y - tsPos.y) - 2.0 * halfSize.x;
		half cube_maskRes = half(clamp(
					length(max(distToEdge, 0.f)), 0.0, 1.0));
		
		maskRes = lerp(sphere_maskRes, cube_maskRes, enableCube);
		coverLerp *= (1.0 - maskRes);
	}
	coverLerp *= coverMask;
	baseColor = lerp(baseColor, weatherColor, coverLerp);
}

half3 ComputeEmission(
	half3 emissionColor,		// 自发光颜色
	half  emissionMask,			// 自发光mask
	half  todEmissionPower,		// 从tod中读取的全局自发光强度系数
	half  minEmissionPower,		// 自发光闪烁时的最低亮度
	half  maxEmissionPower,		// 自发光闪烁时的最亮亮度
	half  timeFactor,			// 自发光闪烁时的强度
	half  enableDynamicEmission // 动态自发光开关
){
	emissionColor *= emissionMask;
	half3 minEmission = emissionColor * minEmissionPower;
	half3 maxEmission = emissionColor * maxEmissionPower; 

	timeFactor = timeFactor * enableDynamicEmission;
	half3 currentEmission = lerp(minEmission, maxEmission, timeFactor);

	return currentEmission * todEmissionPower;
}

bool ComputeBitmapCullMask(
	float4 updateParam, 		//x:进度值   
	float4 boundBox,        	//y:是否开启剔除和是否开启平滑，小于0为关闭剔除，数值为1.0时为开启剔除不开启平滑，1.1时为开启剔除并开启平滑  
	float  dissolveStrength,  	//z:表示轴向(0:Y轴，1:X轴，2:Z轴)
	float3 localPosition,     	//w:是否表示反向剔除和剔除颗粒度的大小，;负数表示反向，大于0表示正向，其绝对值用来调整颗粒度大小，范围为0.0-1.0
	float3 clipSpacePosition  
){
	if(updateParam.y <= 0.001)
		return false;

	int axis = updateParam.z;
	float localpos = localPosition.y;
	if(axis == 2){
		localpos = localPosition.z;
	}else if(axis == 1){
		localpos = localPosition.x;
	}
	
	float modelMin = boundBox.x;
	float modelMax = boundBox.y;
	float modelHeight = modelMax - modelMin;
	float normPos = saturate((localpos - modelMin) / (modelHeight + 0.0005f));
	float progress = saturate(updateParam.x); 

	bool isReverse = updateParam.w < 0.0;
	bool isSmoothEnabled = abs(frac(updateParam.y)) > 0.001;
	float cullParticleScale = abs(updateParam.w);

	{
		float4x4 thresholdMatrix =                              //定义 4x4 的 Bayer 阈值矩阵（经典的点阵抖动图案）。用于生成 pseudo-random 的 dissolve 图案
		{  1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
		  13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
		   4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
		  16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
		};
		float2 pos = clipSpacePosition.xy;
		pos = pos * cullParticleScale;
		pos.xy = pos.xy / dissolveStrength;

		int xindex = (int)fmod(abs(pos.x), 4.0);//通过 mod 4 映射到 4x4 的 Bayer 矩阵坐标
		int yindex = (int)fmod(abs(pos.y), 4.0);
		float threshold = thresholdMatrix[xindex][yindex];

		float LatticeCullRange = 0.2f;	//过渡范围大小

		if(!isReverse)
		{
			//不剔除
			//点阵剔除
			//全剔除

			if(!isSmoothEnabled)
			{
				float condition1 = step(normPos,progress - LatticeCullRange);
				float condition2 = step(normPos,progress) * step(threshold,progress);
				float discardFlag = condition1 + condition2;
				return discardFlag > 0.5;
			}
			else
			{
				float ratio = saturate((normPos - (progress - LatticeCullRange)) / LatticeCullRange);
				float t = lerp(1.0,0.0,ratio);
				float discardFlag = step(threshold,t) + step(1.0 - 0.0001,progress);
				return discardFlag > 0.5;
			}
		}
		else
		{	
			//全剔除
			//点阵剔除
			//不剔除

			if(!isSmoothEnabled)
			{
				float condition1 = step(progress,0.000001f);
				float condition2 = step(progress,normPos) * step(normPos, progress + LatticeCullRange) * step(threshold, 1.0 - progress);
				float condition3 = step(progress + LatticeCullRange,normPos);
				float discardFlag = condition1 + condition2 + condition3;
				return discardFlag > 0.5;
			}
			else
			{
				float ratio = saturate((normPos - progress) / LatticeCullRange);
				float t = lerp(0.0,1.0,ratio);
				float discardFlag = step(threshold, t) + step(progress,0.000001f);
				return discardFlag > 0.5;
			}
		}
	}
}

bool ComputeDitherCullMask(
	float2 clipPos,		
	float  ditherFade,		//点阵剔除强度
	float  ditherScale		//点阵缩放
)
{
	if(ditherFade <= 0.0)
		return false;

	float4x4 thresholdMatrix = float4x4(
	   1.0  / 17.0,   9.0  / 17.0,  3.0  / 17.0,   11.0 / 17.0,
	   13.0 / 17.0,   5.0  / 17.0,  15.0 / 17.0,   7.0  / 17.0,
	   4.0  / 17.0,   12.0 / 17.0,  2.0  / 17.0,   10.0 / 17.0,
	   16.0 / 17.0,   8.0  / 17.0,  14.0 / 17.0,   6.0  / 17.0
	);

	float2 pos = clipPos / ditherScale;

	int xindex = (int)fmod(pos.x, 4.0);
	int yindex = (int)fmod(pos.y, 4.0);

	float threshold = thresholdMatrix[xindex][yindex];

	return (threshold < ditherFade);
}

float3 RestoreZ(in float2 nrm_xy)
{
	float3 normal = float3(nrm_xy, 1.f);
	normal.z = sqrt(1 - dot(nrm_xy, nrm_xy));
	return normal;
}

float3 RestoreZSafe(in float2 nrm_xy)
{
	float3 normal = float3(nrm_xy, 1.f);
	normal.z = sqrt(1 - saturate(dot(nrm_xy, nrm_xy)));
	return normal;
}

void RestoreZ(inout float3 normal)
{
	normal.z = sqrt(1 - dot(normal.xy, normal.xy));
}

void RestoreZSafe(inout float3 normal)
{
	normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));
}

half3 RestoreZ(in half2 nrm_xy)
{
	half3 normal = half3(nrm_xy, 1.f);
	normal.z = sqrt(1 - dot(nrm_xy, nrm_xy));
	return normal;
}

half3 RestoreZSafe(in half2 nrm_xy)
{
	half3 normal = half3(nrm_xy, 1.f);
	normal.z = sqrt(1 - saturate(dot(nrm_xy, nrm_xy)));
	return normal;
}

void RestoreZ(inout half3 normal)
{
	normal.z = sqrt(1 - dot(normal.xy, normal.xy));
}

void RestoreZSafe(inout half3 normal)
{
	normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));
}
