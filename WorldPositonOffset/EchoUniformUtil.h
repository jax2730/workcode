//-----------------------------------------------------------------------------
// File:   EchoUniformUtil.h
//
// Author: chenanzhi
// Desc:
// Date:  2022-3-10
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoUniformUtil_H__
#define __EchoUniformUtil_H__

#include <vector>
#include <map>
#include <string>
#include <unordered_map>
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	//=============================================================================
	enum ECHO_UNIFORM_NAME_ID
	{
		//-----------------------------------------------------------------------------
		//	scene
		U_MainLightDirection,		//主光朝向 在PS中使用
		U_MainLightColor,			//主光颜色 在PS中使用
		U_VS_MainLightDirection,	//主光朝向 在VS中使用
		U_VS_MainLightColor,		//主光颜色 在VS中使用

		U_AuxiliaryLightDirection,	//辅助光朝向
		U_AuxiliaryLightColor,		//辅助光颜色

		U_SkyLightColor,			//天光颜色 在PS中使用
		U_GroundLightColor,			//地光颜色 在PS中使用
		U_VS_SkyLightColor,			//天光颜色 在VS中使用   
		U_VS_GroundLightColor,		//地光颜色 在VS中使用

		U_PS_AmbientBlendThreshold, //天光地光混合的起始角度
		U_VS_AmbientBlendThreshold,

		U_PBR_MainLightColor,
		U_PBR_SkyLightColor,
		U_PBR_GroundLightColor,

		U_VS_PBR_MainLightColor,	// PBR主光颜色，在VS中使用
		U_VS_PBR_SkyLightColor,		// PBR天光颜色，在VS中使用
		U_VS_PBR_GroundLightColor,	// PBR地光颜色，在VS中使用
		
		U_PBR_RMatrix,				//球谐编码的环境光照
		U_PBR_GMatrix,
		U_PBR_BMatrix,

		U_PBR_Environment_Explosure,
		//U_PBR_Environment_Brightness,		//PBR环境图亮度参数
		//U_PBR_Environment_MaxIlluminance,	//PBR环境图最大亮度

		U_RoleMainLightColor,
		U_RoleSkyLightColor,
		U_RoleGroundLightColor,

		U_SunColor,
		U_FogColorNear,
		U_FogColorFar,
		U_FogParam,
		U_FogRampParam,

		U_SphFogLowerLayerColor,
		U_SphFogUpperLayerColor,
		U_SphFogEarthCenter,
		U_SphFogBlendParam,
		U_SphFogParam0,
		U_SphFogParam1,
		U_SphFogSkyParam,

		U_WaterShallowColor,
		U_WaterDeepColor,
		U_WaterFoamColor,
		U_WaterFoamParams,
		U_WaterReflectColor,
		U_UnderWaterColor,

		U_ShadowWeight,

		U_CameraParam,
		U_VS_ViewParam,		// Viewport参数，在VS中使用
		U_ViewParam,		// Viewport参数，在PS中使用
		U_VS_RenderWindowParam,	//渲染窗口参数, 在VS中使用
		U_EnvironmentCubeMapMipLevels, //全局环境反射贴图的mipLevels

		U_ReflectCubeMapMipLevels,		//各个材质中指定的反射贴图的mipLevel

		U_GlobalEnvironmentCubeMipMapLevels, //HDR环境图中反射贴图的mipmapLevels(IllumPBR2023中使用)

		U_DepthRTParam,

		U_ForwardSSREnable,

		U_PS_ViewLightPercent,

		//-----------------------------------------------------------------------------
		//	effect
		U_ViewProjectMatrix,
		U_InvViewProjectMatrix,
		U_ViewMatrix,
		U_ProjectMatrix,
		U_InvProjectMatrix,
		U_VS_InvProjectMatrix,
		U_ZeroViewProjectMatrix,	// view-projection matrix without camera position
		U_VS_WorldViewMatrix,
		U_VS_InvWorldViewMatrix,
		U_VS_CameraPosition,
		U_PS_CameraPosition,
		U_CameraPositionLocalSpace,

		U_ViewDirection,
		U_ViewRightVector,
		U_ViewUpVector,
		U_PS_ViewDirection,
		//-----------------------------------------------------------------------------
		//	object

		U_WorldViewProjectMatrix,
		U_LastWorldViewProjectMatrix,
		U_WorldMatrix,
		U_InvWorldMatrix,
		U_WorldMatrixBaseView,
		//U_InvWorldMatrixBaseView,
		U_InvTransposeWorldMatrix,
		U_PS_InvTransposeWorldMatrix,
		U_WorldRotationMatrix,				// only rotation, for billboard leaf
		U_WorldViewProjectNoRotationMatrix,	// no rotation in world matrix, for billboard leaf
		U_InvWorldViewProjectMatrix,
		U_WorldPosition,
		U_WorldScale,
		U_WorldOrientation,
		U_ObjectMirrored,					// 物体三个轴的镜像情况，xyz：0，没有镜像，1，镜像
											// 若三个轴中有奇数个轴发生镜像，则需要反转副切线。
											// w: 反转副切线的系数，1 or -1.

		U_InstanceWorldMatrix,
		U_InstanceLightWorldPosiArray,
		U_InstanceLightDiffuseColorArray,

		//skeleton obj
		U_BoneMatrix,
		U_LastBoneMatrix,
		U_MaterialDiffuse,	//material param0	//	alpha channel : alpha test factor
		U_MaterialSpecular,	//material param1	//	alpha channel : emissiveFrequance & emissiveBaseOffset
		U_MaterialFresnel,	//material param2	//	opacity FresnelPower Glossiness SpecularLevel
		U_MaterialEmission, //material param3
		U_MaterialCustomParam,//material param4
		U_MaterialCustomParam1,//material param5
		U_GlobalBuildingColor,//material param6
		U_AnimationParams, //material param7

		U_LightPower,        //material param9
		U_VS_LightPower,

		U_ClusterLightParam, // ScreenSize clusterBlockSize
		U_ClusterLightWorldPosArray,
		U_ClusterLightDiffuseColorArray,
		U_ClusterLightDirectionOuterAngleArray,
		U_ClusterLightInnerAngleArray,

		U_VS_Time,		// 由于Shader传参的机制, 在VS用过的参数无法传入的PS中, 因此提供两个时间的参数给VS和PS中都用的Time的情况
		U_PS_Time,
		U_WindParams,
		U_VS_WindParams,
		U_PS_WindParams,
		U_WindParamsObjSpace,
		U_WindParamsGravitySpace,
		U_GlobalAnimationParams,
		U_VS_FPS,

		// shadows
		U_LightVPMatrix0,
		U_LightVPMatrix1,
		U_LightVPMatrix2,
		U_LightVPMatrix3,
		U_LightVPMatrixView0,
		U_LightVPMatrixView1,
		U_LightVPMatrixView2,
		U_LightVPMatrixView3,
		U_ShadowSplit,
		U_ShadowTexOffset,
		U_MainCamViewMatrix,
		U_ShadowEdgeFactor,
		U_ShadowBiasMul,
		U_VS_ShadowSplit,
		U_RainVPMatrix,
		U_ShadowLightViewMatrixArray,
		U_ShadowLightWorldPosArray,
		U_ShadowLightDiffuseColorArray,
		U_ShadowLightDirectionOuterAngleArray,
		U_ShadowLightInnerAngleArray,
		U_ShadowLightMappingArray,

		U_ShadowLightDepthMulAdd,

		U_WeatherParam0,			// xyz:color w:factor
		U_WeatherParam1,			// x:roughness y:metal
		U_WeatherParam2,
		U_CoverColorParams,	        // xyz:range center w:range radius
		U_CoverColorRotation,
		U_VS_CloudCollision,
		U_PS_CloudCollision,

		U_VS_GravityDirection,
		U_PS_GravityDirection,

		U_VS_GravityRotationMatrix,
		U_PS_GravityRotationMatrix,

		U_ShaderDebugFlag,
		
		U_GlobalAgingColour,
		//	general register
		U_VSGeneralRegister0,
		U_VSGeneralRegister1,
		U_VSGeneralRegister2,
		U_VSGeneralRegister3,
		U_VSGeneralRegister4,
		U_VSGeneralRegister5,
		U_VSGeneralRegister6,
		U_VSGeneralRegister7,

		U_PSGeneralRegister0,
		U_PSGeneralRegister1,
		U_PSGeneralRegister2,
		U_PSGeneralRegister3,
		U_PSGeneralRegister4,
		U_PSGeneralRegister5,
		U_PSGeneralRegister6,
		U_PSGeneralRegister7,
		U_PSGeneralRegister8,
		U_PSGeneralRegister9,
		U_PSGeneralRegister10,
		U_PSGeneralRegister11,
		U_PSGeneralRegister12,
		U_PSGeneralRegister13,
		U_PSGeneralRegister14,
		U_PSGeneralRegister15,
		U_PSGeneralRegister16,
		U_PSGeneralRegister17,
		U_PSGeneralRegister18,
		U_PSGeneralRegister19,
		U_PSGeneralRegister20,
		U_PSGeneralRegister21,
		U_PSGeneralRegister22,
		U_PSGeneralRegister23,
		U_PSGeneralRegister24,
		U_PSGeneralRegister25,
		U_PSGeneralRegister26,
		U_PSGeneralRegister27,
		U_PSGeneralRegister28,
		U_PSGeneralRegister29,
		U_PSGeneralRegister30,
		U_PSGeneralRegister31,
		U_PSGeneralRegister32,

		UniformNameID_Count,


		//-----------------------------------------------------------------------------
		//	Custom
		U_VSCustom0,
		U_VSCustom1,
		U_VSCustom2,
		U_VSCustom3,
		U_VSCustom4,
		U_VSCustom5,
		U_VSCustom6,
		U_VSCustom7,
		U_VSCustom8,
		U_VSCustom9,
		U_VSCustom10,
		U_VSCustom11,
		U_VSCustom12,
		U_VSCustom13,
		U_VSCustom14,
		U_VSCustom15,

		U_PSCustom0,
		U_PSCustom1,
		U_PSCustom2,
		U_PSCustom3,
		U_PSCustom4,
		U_PSCustom5,
		U_PSCustom6,
		U_PSCustom7,
		U_PSCustom8,
		U_PSCustom9,
		U_PSCustom10,
		U_PSCustom11,
		U_PSCustom12,
		U_PSCustom13,
		U_PSCustom14,
		U_PSCustom15,

		U_CSCustom0,
		U_CSCustom1,
		U_CSCustom2,
		U_CSCustom3,
		U_CSCustom4,
		U_CSCustom5,
		U_CSCustom6,
		U_CSCustom7,
		U_CSCustom8,
		U_CSCustom9,
		U_CSCustom10,
		U_CSCustom11,
		U_CSCustom12,
		U_CSCustom13,
		U_CSCustom14,
		U_CSCustom15,

		//-----------------------------------------------------------------------------

		UniformNameID_Max
	};

	static_assert(UniformNameID_Max < 512, "UniformNameID_Max Error");

	//-----------------------------------------------------------------------------
	struct UniformNameIDUtil
	{
#define registerUniformNameID(key)  \
	m_UniformNameToIDMap.insert(std::make_pair(#key, key)); \
	m_UniformIDToName[key] = #key;

		typedef std::unordered_map<std::string, unsigned int>	UniformNameToIDMap;

		static void init()
		{
			registerUniformNameID(U_MainLightDirection);
			registerUniformNameID(U_MainLightColor);
			registerUniformNameID(U_VS_MainLightDirection);
			registerUniformNameID(U_VS_MainLightColor);
			registerUniformNameID(U_AuxiliaryLightDirection);
			registerUniformNameID(U_AuxiliaryLightColor);
			registerUniformNameID(U_SkyLightColor);
			registerUniformNameID(U_GroundLightColor);
			registerUniformNameID(U_VS_SkyLightColor);
			registerUniformNameID(U_VS_GroundLightColor);

			registerUniformNameID(U_PBR_MainLightColor);
			registerUniformNameID(U_PBR_SkyLightColor);
			registerUniformNameID(U_PBR_GroundLightColor);
			registerUniformNameID(U_PS_AmbientBlendThreshold);
			registerUniformNameID(U_VS_PBR_MainLightColor);
			registerUniformNameID(U_VS_PBR_SkyLightColor);
			registerUniformNameID(U_VS_PBR_GroundLightColor);
			registerUniformNameID(U_VS_AmbientBlendThreshold);

			registerUniformNameID(U_PBR_RMatrix);
			registerUniformNameID(U_PBR_GMatrix);
			registerUniformNameID(U_PBR_BMatrix);

			registerUniformNameID(U_PBR_Environment_Explosure);
			registerUniformNameID(U_GlobalEnvironmentCubeMipMapLevels);
			registerUniformNameID(U_RoleMainLightColor);
			registerUniformNameID(U_RoleSkyLightColor);
			registerUniformNameID(U_RoleGroundLightColor);

			registerUniformNameID(U_SunColor);
			registerUniformNameID(U_FogColorNear);
			registerUniformNameID(U_FogColorFar);
			registerUniformNameID(U_FogParam);
			registerUniformNameID(U_FogRampParam);

			registerUniformNameID(U_SphFogLowerLayerColor);
			registerUniformNameID(U_SphFogUpperLayerColor);
			registerUniformNameID(U_SphFogEarthCenter);
			registerUniformNameID(U_SphFogBlendParam);
			registerUniformNameID(U_SphFogParam0);
			registerUniformNameID(U_SphFogParam1);
			registerUniformNameID(U_SphFogSkyParam);

			registerUniformNameID(U_WaterShallowColor);
			registerUniformNameID(U_WaterDeepColor);
			registerUniformNameID(U_WaterFoamColor);
			registerUniformNameID(U_WaterFoamParams);
			registerUniformNameID(U_WaterReflectColor);
			registerUniformNameID(U_UnderWaterColor);

			registerUniformNameID(U_ShadowWeight);

			registerUniformNameID(U_CameraParam);
			registerUniformNameID(U_VS_ViewParam);
			registerUniformNameID(U_ViewParam);
			registerUniformNameID(U_VS_RenderWindowParam);
			registerUniformNameID(U_EnvironmentCubeMapMipLevels);

			registerUniformNameID(U_ReflectCubeMapMipLevels);

			registerUniformNameID(U_DepthRTParam);

			registerUniformNameID(U_ForwardSSREnable);

			registerUniformNameID(U_PS_ViewLightPercent);

			registerUniformNameID(U_ViewProjectMatrix);
			registerUniformNameID(U_InvViewProjectMatrix);
			registerUniformNameID(U_ViewMatrix);
			registerUniformNameID(U_ProjectMatrix);
			registerUniformNameID(U_InvProjectMatrix);
			registerUniformNameID(U_VS_InvProjectMatrix);
			registerUniformNameID(U_ZeroViewProjectMatrix);

			registerUniformNameID(U_VS_WorldViewMatrix);
			registerUniformNameID(U_VS_InvWorldViewMatrix);
			registerUniformNameID(U_VS_CameraPosition);
			registerUniformNameID(U_PS_CameraPosition);
			registerUniformNameID(U_CameraPositionLocalSpace);

			registerUniformNameID(U_ViewDirection);
			registerUniformNameID(U_PS_ViewDirection);
			registerUniformNameID(U_ViewRightVector);
			registerUniformNameID(U_ViewUpVector);

			registerUniformNameID(U_WorldViewProjectMatrix);
			registerUniformNameID(U_LastWorldViewProjectMatrix);
			registerUniformNameID(U_WorldMatrix);
			registerUniformNameID(U_InvWorldMatrix);
			registerUniformNameID(U_InvTransposeWorldMatrix);
			registerUniformNameID(U_PS_InvTransposeWorldMatrix);
			registerUniformNameID(U_WorldRotationMatrix);
			registerUniformNameID(U_WorldViewProjectNoRotationMatrix);
			registerUniformNameID(U_InvWorldViewProjectMatrix);
			registerUniformNameID(U_WorldPosition);
			registerUniformNameID(U_WorldScale);
			registerUniformNameID(U_WorldOrientation);
			registerUniformNameID(U_ObjectMirrored);
			
			registerUniformNameID(U_InstanceWorldMatrix);
			registerUniformNameID(U_InstanceLightWorldPosiArray);
			registerUniformNameID(U_InstanceLightDiffuseColorArray);

			registerUniformNameID(U_BoneMatrix);
			registerUniformNameID(U_LastBoneMatrix);
			registerUniformNameID(U_MaterialDiffuse);
			registerUniformNameID(U_MaterialSpecular);
			registerUniformNameID(U_MaterialFresnel);
			registerUniformNameID(U_MaterialEmission);
			registerUniformNameID(U_MaterialCustomParam);
			registerUniformNameID(U_MaterialCustomParam1);
			registerUniformNameID(U_GlobalBuildingColor);

			registerUniformNameID(U_LightPower);
			registerUniformNameID(U_VS_LightPower);

			registerUniformNameID(U_ClusterLightParam);
			registerUniformNameID(U_ClusterLightWorldPosArray);
			registerUniformNameID(U_ClusterLightDiffuseColorArray);
			registerUniformNameID(U_ClusterLightDirectionOuterAngleArray);
			registerUniformNameID(U_ClusterLightInnerAngleArray);

			registerUniformNameID(U_VS_Time);
			registerUniformNameID(U_PS_Time);
			registerUniformNameID(U_WindParams);
			registerUniformNameID(U_VS_WindParams);
			registerUniformNameID(U_PS_WindParams);
			registerUniformNameID(U_WindParamsObjSpace);
			registerUniformNameID(U_WindParamsGravitySpace);
			registerUniformNameID(U_AnimationParams);
			registerUniformNameID(U_GlobalAnimationParams);
			registerUniformNameID(U_VS_FPS);

			registerUniformNameID(U_LightVPMatrix0);
			registerUniformNameID(U_LightVPMatrix1);
			registerUniformNameID(U_LightVPMatrix2);
			registerUniformNameID(U_LightVPMatrix3);
			registerUniformNameID(U_LightVPMatrixView0);
			registerUniformNameID(U_LightVPMatrixView1);
			registerUniformNameID(U_LightVPMatrixView2);
			registerUniformNameID(U_LightVPMatrixView3);
			registerUniformNameID(U_WorldMatrixBaseView);
			registerUniformNameID(U_ShadowSplit);
			registerUniformNameID(U_ShadowTexOffset);
			registerUniformNameID(U_MainCamViewMatrix);
			registerUniformNameID(U_ShadowEdgeFactor);
			registerUniformNameID(U_ShadowBiasMul)
			registerUniformNameID(U_VS_ShadowSplit)
			registerUniformNameID(U_RainVPMatrix)

			registerUniformNameID(U_ShadowLightViewMatrixArray)
			registerUniformNameID(U_ShadowLightWorldPosArray)
			registerUniformNameID(U_ShadowLightDiffuseColorArray)
			registerUniformNameID(U_ShadowLightDirectionOuterAngleArray)
			registerUniformNameID(U_ShadowLightInnerAngleArray)
			registerUniformNameID(U_ShadowLightMappingArray)
			registerUniformNameID(U_ShadowLightDepthMulAdd)

			registerUniformNameID(U_WeatherParam0);
			registerUniformNameID(U_WeatherParam1);
			registerUniformNameID(U_WeatherParam2);
			registerUniformNameID(U_CoverColorParams);
			registerUniformNameID(U_CoverColorRotation);
			registerUniformNameID(U_GlobalAgingColour);

			registerUniformNameID(U_VS_CloudCollision);
			registerUniformNameID(U_PS_CloudCollision);
			
			registerUniformNameID(U_VS_GravityDirection);
			registerUniformNameID(U_PS_GravityDirection);

			registerUniformNameID(U_VS_GravityRotationMatrix);
			registerUniformNameID(U_PS_GravityRotationMatrix);

			registerUniformNameID(U_ShaderDebugFlag);

			registerUniformNameID(U_VSGeneralRegister0);
			registerUniformNameID(U_VSGeneralRegister1);
			registerUniformNameID(U_VSGeneralRegister2);
			registerUniformNameID(U_VSGeneralRegister3);
			registerUniformNameID(U_VSGeneralRegister4);
			registerUniformNameID(U_VSGeneralRegister5);
			registerUniformNameID(U_VSGeneralRegister6);
			registerUniformNameID(U_VSGeneralRegister7);

			registerUniformNameID(U_PSGeneralRegister0);
			registerUniformNameID(U_PSGeneralRegister1);
			registerUniformNameID(U_PSGeneralRegister2);
			registerUniformNameID(U_PSGeneralRegister3);
			registerUniformNameID(U_PSGeneralRegister4);
			registerUniformNameID(U_PSGeneralRegister5);
			registerUniformNameID(U_PSGeneralRegister6);
			registerUniformNameID(U_PSGeneralRegister7);
			registerUniformNameID(U_PSGeneralRegister8);
			registerUniformNameID(U_PSGeneralRegister9);
			registerUniformNameID(U_PSGeneralRegister10);
			registerUniformNameID(U_PSGeneralRegister11);
			registerUniformNameID(U_PSGeneralRegister12);
			registerUniformNameID(U_PSGeneralRegister13);
			registerUniformNameID(U_PSGeneralRegister14);
			registerUniformNameID(U_PSGeneralRegister15);
			registerUniformNameID(U_PSGeneralRegister16);
			registerUniformNameID(U_PSGeneralRegister17);
			registerUniformNameID(U_PSGeneralRegister18);
			registerUniformNameID(U_PSGeneralRegister19);
			registerUniformNameID(U_PSGeneralRegister20);
			registerUniformNameID(U_PSGeneralRegister21);
			registerUniformNameID(U_PSGeneralRegister22);
			registerUniformNameID(U_PSGeneralRegister23);
			registerUniformNameID(U_PSGeneralRegister24);
			registerUniformNameID(U_PSGeneralRegister25);
			registerUniformNameID(U_PSGeneralRegister26);
			registerUniformNameID(U_PSGeneralRegister27);
			registerUniformNameID(U_PSGeneralRegister28);
			registerUniformNameID(U_PSGeneralRegister29);
			registerUniformNameID(U_PSGeneralRegister30);
			registerUniformNameID(U_PSGeneralRegister31);
			registerUniformNameID(U_PSGeneralRegister32);

			registerUniformNameID(UniformNameID_Count);

			registerUniformNameID(U_VSCustom0);
			registerUniformNameID(U_VSCustom1);
			registerUniformNameID(U_VSCustom2);
			registerUniformNameID(U_VSCustom3);
			registerUniformNameID(U_VSCustom4);
			registerUniformNameID(U_VSCustom5);
			registerUniformNameID(U_VSCustom6);
			registerUniformNameID(U_VSCustom7);
			registerUniformNameID(U_VSCustom8);
			registerUniformNameID(U_VSCustom9);
			registerUniformNameID(U_VSCustom10);
			registerUniformNameID(U_VSCustom11);
			registerUniformNameID(U_VSCustom12);
			registerUniformNameID(U_VSCustom13);
			registerUniformNameID(U_VSCustom14);
			registerUniformNameID(U_VSCustom15);

			registerUniformNameID(U_PSCustom0);
			registerUniformNameID(U_PSCustom1);
			registerUniformNameID(U_PSCustom2);
			registerUniformNameID(U_PSCustom3);
			registerUniformNameID(U_PSCustom4);
			registerUniformNameID(U_PSCustom5);
			registerUniformNameID(U_PSCustom6);
			registerUniformNameID(U_PSCustom7);
			registerUniformNameID(U_PSCustom8);
			registerUniformNameID(U_PSCustom9);
			registerUniformNameID(U_PSCustom10);
			registerUniformNameID(U_PSCustom11);
			registerUniformNameID(U_PSCustom12);
			registerUniformNameID(U_PSCustom13);
			registerUniformNameID(U_PSCustom14);
			registerUniformNameID(U_PSCustom15);

			registerUniformNameID(U_CSCustom0);
			registerUniformNameID(U_CSCustom1);
			registerUniformNameID(U_CSCustom2);
			registerUniformNameID(U_CSCustom3);
			registerUniformNameID(U_CSCustom4);
			registerUniformNameID(U_CSCustom5);
			registerUniformNameID(U_CSCustom6);
			registerUniformNameID(U_CSCustom7);
			registerUniformNameID(U_CSCustom8);
			registerUniformNameID(U_CSCustom9);
			registerUniformNameID(U_CSCustom10);
			registerUniformNameID(U_CSCustom11);
			registerUniformNameID(U_CSCustom12);
			registerUniformNameID(U_CSCustom13);
			registerUniformNameID(U_CSCustom14);
			registerUniformNameID(U_CSCustom15);
		}

		static const unsigned int toValue(const std::string &name)
		{
			UniformNameToIDMap::const_iterator it = m_UniformNameToIDMap.find(name);
			if (it != m_UniformNameToIDMap.end())
				return it->second;

			return UniformNameID_Max;
		}

		static const std::string & toName(unsigned int id)
		{
			if (id < UniformNameID_Max)
				return m_UniformIDToName[id];

			return m_UniformIDToName[UniformNameID_Max];
		}

		static UniformNameToIDMap & getUniformNameToIDMap() { return m_UniformNameToIDMap; }
	private:
		static UniformNameToIDMap m_UniformNameToIDMap;
		static std::string m_UniformIDToName[UniformNameID_Max + 1];
	};
	//-----------------------------------------------------------------------------

	//=============================================================================
	enum ECHO_SAMPLER_NAME_ID
	{
		//通用的
		S_Diffuse = 0,
		S_Specular,// 1
		S_Bump, // 2
		S_Custom,// 3
		S_Custom2,// 4
		S_Custom3,// 5
		S_Custom4, // 6
		S_Custom5,// 7
		S_Custom6,// 8
		S_Custom7,// 9
		S_Custom8,// 10
		S_Custom9, // 11
		S_Custom10, // 12 
		S_Environment, // 13
		S_ShadowMap0, // 14
		S_ShadowMap1, // 15
		S_ShadowMap2,
		S_ShadowMap3,
		S_Dpsm0,
		S_Dpsm1,
		S_Dpsm2,
		S_Dpsm3,
		S_Dpsm4,
		S_Dpsm5,
		S_Dpsm6,
		S_Dpsm7,
		S_GBuffer0,
		S_GBuffer1,
		S_GBuffer2,
		S_DeferredSC,	// Deferred Scene Color
		S_Depth,
		S_DepthLoZ,
		S_Ripple,
#ifdef _WIN32
		S_RippleFlow,
		S_RippleDrops,
		S_RippleNoise,
#endif
		S_BackbufferTex,
		S_NoiseTex,
		S_Noise3DTex,
		S_Noise3DTex128,
		S_AgingTex,
		S_CausticTex,
		S_ClusterLightMaskTex, // 16
		S_RainEffectMap,

		S_ReflectCubeMap,		//材质里指定的反射贴图 模型水用
		S_EnvironmentCubeMap,	//全局环境反射
		S_GlobalEnvironmentCubeMap,
		S_PreComputeTransmittanceMap,
		SamplerNameID_Count,	//通用个数

		//扩展自定义用
		S_2DExt0,
		S_2DExt1,
		S_2DExt2,
		S_2DExt3,
		S_2DExt4,
		S_2DExt5,
		S_2DExt6,
		S_2DExt7,
		S_2DExt8,
		S_2DExt9,
		S_2DExt10,
		S_2DExt11,
		S_2DExt12,
		S_2DExt13,
		S_2DExt14,
		S_2DExt15,

		S_CubeMapExt0,
		S_CubeMapExt1,
		S_CubeMapExt2,
		S_CubeMapExt3,
		S_CubeMapExt4,
		S_CubeMapExt5,
		S_CubeMapExt6,
		S_CubeMapExt7,
		S_CubeMapExt8,
		S_CubeMapExt9,
		S_CubeMapExt10,
		S_CubeMapExt11,
		S_CubeMapExt12,
		S_CubeMapExt13,
		S_CubeMapExt14,
		S_CubeMapExt15,

		S_2DArrayExt0,
		S_2DArrayExt1,
		S_2DArrayExt2,
		S_2DArrayExt3,
		S_2DArrayExt4,
		S_2DArrayExt5,
		S_2DArrayExt6,
		S_2DArrayExt7,
		S_2DArrayExt8,
		S_2DArrayExt9,
		S_2DArrayExt10,
		S_2DArrayExt11,
		S_2DArrayExt12,
		S_2DArrayExt13,
		S_2DArrayExt14,
		S_2DArrayExt15,

		SamplerNameID_Max
	};
	constexpr unsigned int GpuCullSupportArrayTexture_SN = (1 << S_Diffuse) | (1 << S_Specular) | (1 << S_Bump);
	constexpr unsigned int SupportArrayTextureCount = 3;

	static_assert(SamplerNameID_Max < 128, "UniformNameID_Max Error");

	//-----------------------------------------------------------------------------
	struct SamplerNameIDUtil
	{
#define registerSamplerNameID(key) \
	m_SamplerNameToIDMap.insert(std::make_pair(#key, key)); \
	m_SamplerIDToName[key] = #key;

		typedef std::unordered_map<std::string, unsigned int>	SamplerNameToIDMap;

		static void init()
		{
			registerSamplerNameID(S_Diffuse);
			registerSamplerNameID(S_Specular);
			registerSamplerNameID(S_Bump);
			registerSamplerNameID(S_Custom);
			registerSamplerNameID(S_Custom2);
			registerSamplerNameID(S_Custom3);
			registerSamplerNameID(S_Custom4);
			registerSamplerNameID(S_Custom5);
			registerSamplerNameID(S_Custom6);
			registerSamplerNameID(S_Custom7);
			registerSamplerNameID(S_Custom8);
			registerSamplerNameID(S_Custom9);
			registerSamplerNameID(S_Custom10);
			registerSamplerNameID(S_Environment);
			registerSamplerNameID(S_ShadowMap0);
			registerSamplerNameID(S_ShadowMap1);
			registerSamplerNameID(S_ShadowMap2);
			registerSamplerNameID(S_ShadowMap3);
			registerSamplerNameID(S_Dpsm0)
			registerSamplerNameID(S_Dpsm1)
			registerSamplerNameID(S_Dpsm2)
			registerSamplerNameID(S_Dpsm3)
			registerSamplerNameID(S_Dpsm4)
			registerSamplerNameID(S_Dpsm5)
			registerSamplerNameID(S_Dpsm6)
			registerSamplerNameID(S_Dpsm7)
			registerSamplerNameID(S_GBuffer0)
			registerSamplerNameID(S_GBuffer1)
			registerSamplerNameID(S_GBuffer2)
			registerSamplerNameID(S_DeferredSC)
			registerSamplerNameID(S_Depth);
			registerSamplerNameID(S_DepthLoZ)
			registerSamplerNameID(S_Ripple);
#ifdef _WIN32
			registerSamplerNameID(S_RippleFlow);
			registerSamplerNameID(S_RippleDrops);
			registerSamplerNameID(S_RippleNoise);
#endif
			registerSamplerNameID(S_BackbufferTex);
			registerSamplerNameID(S_NoiseTex);
			registerSamplerNameID(S_Noise3DTex);
			registerSamplerNameID(S_Noise3DTex128);
			registerSamplerNameID(S_AgingTex);
			registerSamplerNameID(S_CausticTex);
			registerSamplerNameID(S_ClusterLightMaskTex);
			registerSamplerNameID(S_ReflectCubeMap);
			registerSamplerNameID(S_EnvironmentCubeMap);
			registerSamplerNameID(S_GlobalEnvironmentCubeMap);
			registerSamplerNameID(S_RainEffectMap);
			registerSamplerNameID(S_PreComputeTransmittanceMap);
			registerSamplerNameID(SamplerNameID_Count);

			registerSamplerNameID(S_2DExt0);
			registerSamplerNameID(S_2DExt1);
			registerSamplerNameID(S_2DExt2);
			registerSamplerNameID(S_2DExt3);
			registerSamplerNameID(S_2DExt4);
			registerSamplerNameID(S_2DExt5);
			registerSamplerNameID(S_2DExt6);
			registerSamplerNameID(S_2DExt7);
			registerSamplerNameID(S_2DExt8);
			registerSamplerNameID(S_2DExt9);
			registerSamplerNameID(S_2DExt10);
			registerSamplerNameID(S_2DExt11);
			registerSamplerNameID(S_2DExt12);
			registerSamplerNameID(S_2DExt13);
			registerSamplerNameID(S_2DExt14);
			registerSamplerNameID(S_2DExt15);

			registerSamplerNameID(S_CubeMapExt0);
			registerSamplerNameID(S_CubeMapExt1);
			registerSamplerNameID(S_CubeMapExt2);
			registerSamplerNameID(S_CubeMapExt3);
			registerSamplerNameID(S_CubeMapExt4);
			registerSamplerNameID(S_CubeMapExt5);
			registerSamplerNameID(S_CubeMapExt6);
			registerSamplerNameID(S_CubeMapExt7);
			registerSamplerNameID(S_CubeMapExt8);
			registerSamplerNameID(S_CubeMapExt9);
			registerSamplerNameID(S_CubeMapExt10);
			registerSamplerNameID(S_CubeMapExt11);
			registerSamplerNameID(S_CubeMapExt12);
			registerSamplerNameID(S_CubeMapExt13);
			registerSamplerNameID(S_CubeMapExt14);
			registerSamplerNameID(S_CubeMapExt15);

			registerSamplerNameID(S_2DArrayExt0);
			registerSamplerNameID(S_2DArrayExt1);
			registerSamplerNameID(S_2DArrayExt2);
			registerSamplerNameID(S_2DArrayExt3);
			registerSamplerNameID(S_2DArrayExt4);
			registerSamplerNameID(S_2DArrayExt5);
			registerSamplerNameID(S_2DArrayExt6);
			registerSamplerNameID(S_2DArrayExt7);
			registerSamplerNameID(S_2DArrayExt8);
			registerSamplerNameID(S_2DArrayExt9);
			registerSamplerNameID(S_2DArrayExt10);
			registerSamplerNameID(S_2DArrayExt11);
			registerSamplerNameID(S_2DArrayExt12);
			registerSamplerNameID(S_2DArrayExt13);
			registerSamplerNameID(S_2DArrayExt14);
			registerSamplerNameID(S_2DArrayExt15);
		}

		static const unsigned int toValue(const std::string & name)
		{
			SamplerNameToIDMap::const_iterator it = m_SamplerNameToIDMap.find(name);
			if (it != m_SamplerNameToIDMap.end())
				return it->second;

			return SamplerNameID_Max;
		}

		static const std::string & toName(unsigned int id)
		{
			if (id < SamplerNameID_Max)
				return m_SamplerIDToName[id];

			return m_SamplerIDToName[SamplerNameID_Max];
		}

		static SamplerNameToIDMap & getSamplerNameToIDMap() { return m_SamplerNameToIDMap; }
	private:
		static SamplerNameToIDMap m_SamplerNameToIDMap;
		static std::string m_SamplerIDToName[SamplerNameID_Max + 1];
	};
	//-----------------------------------------------------------------------------
	enum ECHO_BUFFER_NAME_ID
	{
		//-----------------------------------------------------------------------------
		//	Custom
		B_CSCustom0,
		B_CSCustom1,
		B_CSCustom2,
		B_CSCustom3,
		B_CSCustom4,
		B_CSCustom5,
		B_CSCustom6,
		B_CSCustom7,
		B_CSCustom8,
		B_CSCustom9,
		B_CSCustom10,
		B_CSCustom11,
		B_CSCustom12,
		B_CSCustom13,
		B_CSCustom14,
		B_CSCustom15,
		//-----------------------------------------------------------------------------

		BufferNameID_Max
	};

	static_assert(BufferNameID_Max < 64, "BufferNameID_Max Error");

	//-----------------------------------------------------------------------------
	struct BufferNameIDUtil
	{
#define registerBufferNameID(key)  \
	m_BufferNameToIDMap.insert(std::make_pair(#key, key)); \
	m_BufferIDToName[key] = #key;

		typedef std::unordered_map<std::string, unsigned int>	BufferNameToIDMap;

		static void init()
		{
			registerBufferNameID(B_CSCustom0);
			registerBufferNameID(B_CSCustom1);
			registerBufferNameID(B_CSCustom2);
			registerBufferNameID(B_CSCustom3);
			registerBufferNameID(B_CSCustom4);
			registerBufferNameID(B_CSCustom5);
			registerBufferNameID(B_CSCustom6);
			registerBufferNameID(B_CSCustom7);
			registerBufferNameID(B_CSCustom8);
			registerBufferNameID(B_CSCustom9);
			registerBufferNameID(B_CSCustom10);
			registerBufferNameID(B_CSCustom11);
			registerBufferNameID(B_CSCustom12);
			registerBufferNameID(B_CSCustom13);
			registerBufferNameID(B_CSCustom14);
			registerBufferNameID(B_CSCustom15);
		}

		static const unsigned int toValue(const std::string& name)
		{
			BufferNameToIDMap::const_iterator it = m_BufferNameToIDMap.find(name);
			if (it != m_BufferNameToIDMap.end())
				return it->second;

			return BufferNameID_Max;
		}

		static const std::string& toName(unsigned int id)
		{
			if (id < BufferNameID_Max)
				return m_BufferIDToName[id];

			return m_BufferIDToName[BufferNameID_Max];
		}

		static BufferNameToIDMap& getBufferNameToIDMap() { return m_BufferNameToIDMap; }
	private:
		static BufferNameToIDMap m_BufferNameToIDMap;
		static std::string m_BufferIDToName[BufferNameID_Max + 1];
	};
} // namespace Echo

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif
