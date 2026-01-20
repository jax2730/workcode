
#include <Common_Uniform_VS.inl>
#include <Common_Function.inl>

#ifdef USEINSTANCE
    #ifndef SKINNEDINSTANCE
      #undef HWSKINNED
	#endif
#endif

#ifdef USEINSTANCE
    #ifdef SKINNEDINSTANCE
 		uniform Texture2D S_2DExt0;
		SamplerState S_2DExt0SamplerState;
		float4 	U_VSCustom0[InstanceCount*UniformCount];
		uint4   U_VSCustom1[(InstanceCount + 3) >> 2]; // animationData [cpu data type is uint32 , need recalculate index ]
    #else
    	float4 	U_VSCustom0[InstanceCount * UniformCount];
	#endif
#else
	float4	U_WorldViewProjectMatrix[4];
	float4  U_WorldMatrix[3];
	float4  U_InvTransposeWorldMatrix[3];
	float4  U_ObjectMirrored;
#endif


#ifdef HWSKINNED
	#ifndef SKINNEDINSTANCE
		float4	U_BoneMatrix[3 * Bone_Matrix_Max_Cnt];
    #endif
#endif

#ifdef MOTIONBLUR
	float4	U_LastWorldViewProjectMatrix[4];
	#ifdef HWSKINNED	
		float4	U_LastBoneMatrix[3 * Bone_Matrix_Max_Cnt];
	#endif	
#endif

#ifdef SoftNoColor
	float4  U_InvWorldMatrix[3];
	float4  U_VSGeneralRegister1;	//偏移大小
	float4  U_WorldScale;
#endif
#ifdef SOFTBOARD
	float4  U_VSGeneralRegister1;	//偏移大小
	float4  U_WorldScale;
#endif


	float4 	U_VSGeneralRegister7; 	//x ---boundingbox min.y (current entity box) local box
									//y ---boundingbox max.y (current entity box)
									//z ---boundingbox min.y (parent entity box)
									//w ---boundingbox max.y (parent entity box)


#ifndef SHADOWPASS

	#ifdef VERTEX_EXCITATION
		float4 U_VSGeneralRegister0;	//顶点扰动
	#endif

#endif



struct VS_INPUT
{
	float4 i_Pos		: POSITION0;
#ifdef SHADOWPASS
	#ifdef ALPHA_TEST
		float4 i_UV		: TEXCOORD0;	
	#endif
#else
	float4 i_Normal		: NORMAL0;
	float4 i_UV			: TEXCOORD0;	
	float4 i_Tangent	: TANGENT0;
#endif

	#ifdef VERTEX_COLOR
		float4	i_Diffuse	: COLOR0;
	#endif

#ifdef HWSKINNED
	float4	blend	: BLENDWEIGHT0;
	uint4	index	: BLENDINDICES0;
#endif
};

struct VS_OUTPUT
{
	float4	o_PosInClip			: SV_POSITION;
#ifdef SHADOWPASS
	#ifdef ALPHA_TEST
		float4 o_UV		: TEXCOORD0;	
	#endif
#else
	float4 	o_UV				: TEXCOORD0;
	float4	o_Fog				: TEXCOORD1;
	float4	o_WNormal			: TEXCOORD2;
	float4	o_WPos				: TEXCOORD3;

	float4	o_Tangent			: TEXCOORD4;
	float4	o_Binormal			: TEXCOORD5;

	//float4	o_EyeDir			: TEXCOORD6;
	#ifdef VERTEX_COLOR
		float4 o_Diffuse		: TEXCOORD7;
	#endif
	#ifdef USESHADOWMAP
		float   o_ViewSpaceZ		: TEXCOORD8;
		float4  O_LightSpacePos[SHADOWSPLITE] : TEXCOORD9; 
	#endif
	#ifdef SOFTBOARD
		float4 o_ClipPos    : TEXCOORD6;
	#endif
	#ifdef MOTIONBLUR
		float4 o_VelUV		: TEXCOORD15;		
	#endif
#endif
#ifdef PARABOLOID_MAPPING
	float	o_ClipDepth			: TEXCOORD12;
#endif
	float4	o_PosInModel		: TEXCOORD13;

	float4  o_BoundBoxRange		: TEXCOORD14;

};
#ifdef EDITOR_MODE
	#ifdef HierarchicalLOD
		uniform float4 U_VSGeneralRegister6;

	#endif
#endif

#ifdef SKINNEDINSTANCE
	float3x4 loadBoneMatrix (uint animationData , uint index)
	{
		float3x4 rvt ;

		rvt[0] = S_2DExt0.Load(uint3(index*3 + 0, animationData, 0));
		rvt[1] = S_2DExt0.Load(uint3(index*3 + 1, animationData, 0));
		rvt[2] = S_2DExt0.Load(uint3(index*3 + 2, animationData, 0));
		return rvt ;
	}
#endif

VS_OUTPUT main(VS_INPUT IN, uint i_InstanceID : SV_InstanceID)
{
	VS_OUTPUT vsOut = (VS_OUTPUT)0;
	float4  pos =  float4(IN.i_Pos.xyz, 1.0f);
	vsOut.o_PosInModel = pos;
	float4 WorldMatrix[3];
	float4 InvTransposeWorldMatrix[3];
	float fHandedness = 1.f;

	#ifdef HWSKINNED
		float4	Weight		= IN.blend;
		uint4  Index		= IN.index;

		float4 BoneMatC0	= float4(0.0,0.0,0.0,0.0);
		float4 BoneMatC1	= float4(0.0,0.0,0.0,0.0);
		float4 BoneMatC2	= float4(0.0,0.0,0.0,0.0);

    	#ifdef SKINNEDINSTANCE
		   	// recalculate index
		   	uint 	animationData = U_VSCustom1[i_InstanceID >> 2][i_InstanceID & 3];
			for(int i=0;i<4;++i)
			{
		     	float3x4 res=loadBoneMatrix(animationData,Index[i]);
		     	BoneMatC0 += Weight[i] * res[0];
			 	BoneMatC1 += Weight[i] * res[1];
			 	BoneMatC2 += Weight[i] * res[2];
			}
	    #else
			for(int i=0;i<4;++i)
			{
				int idx = Index[i];
				BoneMatC0	+=	U_BoneMatrix[idx*3+0]*Weight[i];
				BoneMatC1	+=	U_BoneMatrix[idx*3+1]*Weight[i];
				BoneMatC2	+=	U_BoneMatrix[idx*3+2]*Weight[i];
			}
		#endif // ifdef SKINNEDINSTANCE
	#endif

#ifdef USEINSTANCE
	#ifdef  HWSKINNED
		pos = float4(dot(BoneMatC0,pos),dot(BoneMatC1,pos),dot(BoneMatC2,pos),1.0f);		
	#endif
	#ifndef SHADOWPASS
		float4  normal =  float4(IN.i_Normal.xyz, 0.0f);
		#ifdef  HWSKINNED
			normal = float4(dot(BoneMatC0,normal),dot(BoneMatC1,normal),dot(BoneMatC2,normal),0.0f);
		#endif
		#ifdef VERTEX_EXCITATION
			pos.xyz += normal.xyz * U_VSGeneralRegister0.x * sin(IN.i_UV.x * U_VSGeneralRegister0.y * U_VS_Time.x + IN.i_UV.y * U_VSGeneralRegister0.z * U_VS_Time.x);
		#endif
	#endif
	
	int idxInst = i_InstanceID * UniformCount;
	#ifdef HWSKINNED
		WorldMatrix[0] = U_VSCustom0[idxInst + 0];		// now is WroldViewMatrix
		WorldMatrix[1] = U_VSCustom0[idxInst + 1];
		WorldMatrix[2] = U_VSCustom0[idxInst + 2];
		InvTransposeWorldMatrix[0] = U_VSCustom0[idxInst + 3];
		InvTransposeWorldMatrix[1] = U_VSCustom0[idxInst + 4];
		InvTransposeWorldMatrix[2] = U_VSCustom0[idxInst + 5];
		
		// translate to camera space
		float4 vObjPosInCam ;
		vObjPosInCam.xyz  = mul((float3x4)WorldMatrix, pos);
		vObjPosInCam.w    = 1.f;
		vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);

		WorldMatrix[0][3] += U_VS_CameraPosition[0];
		WorldMatrix[1][3] += U_VS_CameraPosition[1];
		WorldMatrix[2][3] += U_VS_CameraPosition[2]; 	// now is WroldMatrix

		float4 Wpos;
		Wpos.xyz = mul((float3x4)WorldMatrix, pos);
		Wpos.w = 1.0;
	#else
		WorldMatrix[0] = U_VSCustom0[idxInst + 0];
		WorldMatrix[1] = U_VSCustom0[idxInst + 1];
		WorldMatrix[2] = U_VSCustom0[idxInst + 2];
		InvTransposeWorldMatrix[0] = U_VSCustom0[idxInst + 3];
		InvTransposeWorldMatrix[1] = U_VSCustom0[idxInst + 4];
		InvTransposeWorldMatrix[2] = U_VSCustom0[idxInst + 5];
		
		float4 Wpos;
		Wpos.xyz = mul((float3x4)WorldMatrix, pos);
		Wpos.w = 1.0;

		// translate to camera space
		float4 vObjPosInCam = Wpos - U_VS_CameraPosition;
		vObjPosInCam.w = 1.f;
		vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);
	#endif
	
	fHandedness = determinant(float3x3(WorldMatrix[0].xyz, WorldMatrix[1].xyz, WorldMatrix[2].xyz));
	fHandedness = fHandedness >= 0.f ? 1.f : -1.f;
#else
	#ifdef  HWSKINNED
		pos = float4(dot(BoneMatC0,pos),dot(BoneMatC1,pos),dot(BoneMatC2,pos),1.0f);		
	#endif
	#ifndef SHADOWPASS
		float4  normal =  float4(IN.i_Normal.xyz, 0.0f);
		#ifdef  HWSKINNED
			normal = float4(dot(BoneMatC0,normal),dot(BoneMatC1,normal),dot(BoneMatC2,normal),0.0f);
		#endif
		#ifdef VERTEX_EXCITATION
			pos.xyz += normal.xyz * U_VSGeneralRegister0.x * sin(IN.i_UV.x * U_VSGeneralRegister0.y * U_VS_Time.x + IN.i_UV.y * U_VSGeneralRegister0.z * U_VS_Time.x);
		#endif
	#endif
	
	vsOut.o_PosInClip.x = dot(U_WorldViewProjectMatrix[0], pos);
	vsOut.o_PosInClip.y = dot(U_WorldViewProjectMatrix[1], pos);
	vsOut.o_PosInClip.z = dot(U_WorldViewProjectMatrix[2], pos);
	vsOut.o_PosInClip.w = dot(U_WorldViewProjectMatrix[3], pos);	
#endif

#ifdef PARABOLOID_MAPPING
	vsOut.o_ClipDepth = vsOut.o_PosInClip.z;
	ParaboloidMapping(vsOut.o_PosInClip, U_ShadowLightDepthMulAdd.x, U_ShadowLightDepthMulAdd.z, U_ShadowLightDepthMulAdd.w);
#endif

#ifdef SHADOWPASS
	#ifdef ALPHA_TEST
		vsOut.o_UV = IN.i_UV;
	#endif
	
	#ifndef PARABOLOID_MAPPING
		//depthclamp
		vsOut.o_PosInClip.z = ClampToNearPlane(vsOut.o_PosInClip.z);
	#endif
	
#else
	#ifdef MOTIONBLUR
		float4  lastPos =  float4(IN.i_Pos.xyz, 1.0f);
		#ifdef HWSKINNED

			float4 lastBoneMatC0	= float4(0.0,0.0,0.0,0.0);		
			float4 lastBoneMatC1	= float4(0.0,0.0,0.0,0.0);
			float4 lastBoneMatC2	= float4(0.0,0.0,0.0,0.0);

			for(int i=0;i<4;++i)
			{
				int idx = Index[i];
				lastBoneMatC0	+=	U_LastBoneMatrix[idx*3+0]*Weight[i];	
				lastBoneMatC1	+=	U_LastBoneMatrix[idx*3+1]*Weight[i];	
				lastBoneMatC2	+=	U_LastBoneMatrix[idx*3+2]*Weight[i];	
			}
			lastPos = float4(dot(lastBoneMatC0,lastPos),dot(lastBoneMatC1,lastPos),dot(lastBoneMatC2,lastPos),1.0f);	
		#endif
	
		float4 lastPosInClip;
		lastPosInClip.x = dot(U_LastWorldViewProjectMatrix[0], lastPos);
		lastPosInClip.y = dot(U_LastWorldViewProjectMatrix[1], lastPos);
		lastPosInClip.z = dot(U_LastWorldViewProjectMatrix[2], lastPos);
		lastPosInClip.w = dot(U_LastWorldViewProjectMatrix[3], lastPos);
		
		float2 vel = vsOut.o_PosInClip.xy/vsOut.o_PosInClip.w-lastPosInClip.xy/lastPosInClip.w;
		vel *=float2(0.5f,-0.5f);
		vel = vel* U_VS_FPS.x*U_VS_FPS.y;
		vel =vel/ U_VS_ViewParam.zw * 0.5;
		vsOut.o_VelUV.xy = vel;
	#endif
	#ifdef SOFTBOARD
		vsOut.o_ClipPos = vsOut.o_PosInClip;
		vsOut.o_UV.z = U_VSGeneralRegister1.x * U_WorldScale.x;
	#endif	
	#ifdef USEINSTANCE
		vsOut.o_WPos = Wpos;
	#else
		WorldMatrix[0] = U_WorldMatrix[0];
		WorldMatrix[1] = U_WorldMatrix[1];
		WorldMatrix[2] = U_WorldMatrix[2];

		InvTransposeWorldMatrix[0] = U_InvTransposeWorldMatrix[0];
		InvTransposeWorldMatrix[1] = U_InvTransposeWorldMatrix[1];
		InvTransposeWorldMatrix[2] = U_InvTransposeWorldMatrix[2];

		vsOut.o_WPos.xyz = mul((float3x4)WorldMatrix, pos);
		vsOut.o_WPos.w = 1.0;

		fHandedness = U_ObjectMirrored.w;
	#endif
	float4	vCamPos				=	U_VS_CameraPosition;
	#ifdef SoftNoColor
		float3 camtoposi = normalize(vsOut.o_WPos.xyz - vCamPos.xyz);
		float3 viewDir = normalize(U_ViewDirection.xyz);
		float temp = dot(camtoposi, viewDir);
		temp = max(temp, 0.000001);
		vsOut.o_WPos.xyz += camtoposi * U_VSGeneralRegister1.x * U_WorldScale.x/ temp;
		
		float4 tempLocalPosi = vsOut.o_WPos;
		tempLocalPosi.x = dot(U_InvWorldMatrix[0], vsOut.o_WPos);
		tempLocalPosi.y = dot(U_InvWorldMatrix[1], vsOut.o_WPos);
		tempLocalPosi.z = dot(U_InvWorldMatrix[2], vsOut.o_WPos);
		
		vsOut.o_PosInClip.x = dot(U_WorldViewProjectMatrix[0], tempLocalPosi);
		vsOut.o_PosInClip.y = dot(U_WorldViewProjectMatrix[1], tempLocalPosi);
		vsOut.o_PosInClip.z = dot(U_WorldViewProjectMatrix[2], tempLocalPosi);
		vsOut.o_PosInClip.w = dot(U_WorldViewProjectMatrix[3], tempLocalPosi);
	#endif
	
	float4  tangent =  float4(IN.i_Tangent.xyz, 0.0f);
	
	#ifdef  HWSKINNED
		//normal = float4(dot(BoneMatC0,normal),dot(BoneMatC1,normal),dot(BoneMatC2,normal),0.0f);	
		tangent = float4(dot(BoneMatC0,tangent),dot(BoneMatC1,tangent),dot(BoneMatC2,tangent),0.0f);		
	#endif
	
	vsOut.o_WNormal.xyz = mul((float3x4)InvTransposeWorldMatrix, float4(normal.xyz, 0.0f));
	vsOut.o_Tangent.xyz = mul((float3x4)WorldMatrix, float4(tangent.xyz, 0.0f));
	
	vsOut.o_Binormal.xyz = cross(vsOut.o_Tangent.xyz, vsOut.o_WNormal.xyz) * IN.i_Tangent.w;
	vsOut.o_Binormal.xyz *= fHandedness;
	
	
	vsOut.o_UV.xy = IN.i_UV.xy;
		
	
	float3 camToWorld =  vCamPos.xyz - vsOut.o_WPos.xyz;
	
	//vsOut.o_EyeDir.xyz = camToWorld;

	float4x4 gravityRotatePosMatrix;	
	gravityRotatePosMatrix[0] = U_VS_GravityRotationMatrix[0];
	gravityRotatePosMatrix[1] = U_VS_GravityRotationMatrix[1];
	gravityRotatePosMatrix[2] = U_VS_GravityRotationMatrix[2];
	gravityRotatePosMatrix[3] = U_VS_GravityRotationMatrix[3];
	camToWorld = mul(gravityRotatePosMatrix,float4(camToWorld.xyz,0.0f)).xyz;
		
	vsOut.o_Fog = ComputeFogColor(camToWorld, U_FogParam.xy, U_FogParam.zw, U_FogRampParam, U_FogColorNear, U_FogColorFar);
	
	#ifdef USESHADOWMAP
		float viewZ = -dot(U_MainCamViewMatrix, float4(vsOut.o_WPos.xyz, 1.0f));
	
	    float4 wpos = vsOut.o_WPos;
		float3 wNormal = normalize(vsOut.o_WNormal.xyz);
		float3 wLight = -U_VS_MainLightDirection.xyz;

		float shadowCos = dot(wNormal, wLight);
		float shadowSine = sqrt(1-shadowCos*shadowCos);
		
		float4 normalBias = ShadowNormalOffset * shadowSine * U_ShadowBiasMul;
		float4 wposOffset = wpos;
		const float BlendThreshold = 1;
	
		#if SHADOWSPLITE > 0
		{
		#if SHADOWSPLITE > 1
			float lerpAmt = smoothstep(1.0 - BlendThreshold, 1.0, viewZ / U_VS_ShadowSplit[0].x);
			wposOffset.xyz  = wpos.xyz + wNormal * lerp(normalBias[0], normalBias[1], lerpAmt);
		#else
			wposOffset.xyz  = wpos.xyz + wNormal * normalBias[0];
		#endif
		}
		vsOut.O_LightSpacePos[0].x = dot(U_LightVPMatrix0[0], wposOffset);
		vsOut.O_LightSpacePos[0].y = dot(U_LightVPMatrix0[1], wposOffset);
		vsOut.O_LightSpacePos[0].z = dot(U_LightVPMatrix0[2], wposOffset);
		vsOut.O_LightSpacePos[0].w = dot(U_LightVPMatrix0[3], wposOffset);
		#endif
		#if SHADOWSPLITE > 1	
		{
		#if SHADOWSPLITE > 2
			float lerpAmt = smoothstep(1.0 - BlendThreshold, 1.0, (viewZ - U_VS_ShadowSplit[0].x) / (U_VS_ShadowSplit[0].y - U_VS_ShadowSplit[0].x));
			wposOffset.xyz  = wpos.xyz + wNormal * lerp(normalBias[1], normalBias[2], lerpAmt);
		#else
			wposOffset.xyz  = wpos.xyz + wNormal * normalBias[1];
		#endif
		}
		vsOut.O_LightSpacePos[1].x = dot(U_LightVPMatrix1[0], wposOffset);
		vsOut.O_LightSpacePos[1].y = dot(U_LightVPMatrix1[1], wposOffset);
		vsOut.O_LightSpacePos[1].z = dot(U_LightVPMatrix1[2], wposOffset);
		vsOut.O_LightSpacePos[1].w = dot(U_LightVPMatrix1[3], wposOffset);
		#endif
		#if SHADOWSPLITE > 2	
	   	{
		#if SHADOWSPLITE > 3
			float lerpAmt = smoothstep(1.0 - BlendThreshold, 1.0, (viewZ - U_VS_ShadowSplit[0].y) / (U_VS_ShadowSplit[0].z - U_VS_ShadowSplit[0].y));
			wposOffset.xyz  = wpos.xyz + wNormal * lerp(normalBias[2], normalBias[3], lerpAmt);
		#else
			wposOffset.xyz  = wpos.xyz + wNormal * normalBias[2];
		#endif
		}
		vsOut.O_LightSpacePos[2].x = dot(U_LightVPMatrix2[0], wposOffset);
		vsOut.O_LightSpacePos[2].y = dot(U_LightVPMatrix2[1], wposOffset);
		vsOut.O_LightSpacePos[2].z = dot(U_LightVPMatrix2[2], wposOffset);
		vsOut.O_LightSpacePos[2].w = dot(U_LightVPMatrix2[3], wposOffset);
		#endif	
		#if SHADOWSPLITE > 3	
		wposOffset.xyz = wpos.xyz + wNormal * normalBias[3];
		vsOut.O_LightSpacePos[3].x = dot(U_LightVPMatrix3[0], wposOffset);
		vsOut.O_LightSpacePos[3].y = dot(U_LightVPMatrix3[1], wposOffset);
		vsOut.O_LightSpacePos[3].z = dot(U_LightVPMatrix3[2], wposOffset);
		vsOut.O_LightSpacePos[3].w = dot(U_LightVPMatrix3[3], wposOffset);
		#endif			
		vsOut.o_ViewSpaceZ = viewZ;
	#endif
	
	#ifdef VERTEX_COLOR
		vsOut.o_Diffuse = IN.i_Diffuse;
	#endif
	#ifdef EDITOR_MODE
		#ifdef HierarchicalLOD
			vsOut.o_PosInClip = float4(2.0f * ((IN.i_UV.x - U_VSGeneralRegister6.x)* U_VSGeneralRegister6.z) - 1.0f, 1.0f - 2.0f * ((IN.i_UV.y - U_VSGeneralRegister6.y) * U_VSGeneralRegister6.w), 1.0f, 1.0f);
		#endif
	#endif
#endif

#ifndef SHADOWPASS
	vsOut.o_BoundBoxRange.x = U_VSGeneralRegister7.x;//dot(WorldMatrix[1], float4(0.0f, U_VSGeneralRegister7.x, 0.0f, 1.0f));
	vsOut.o_BoundBoxRange.y = U_VSGeneralRegister7.y;//dot(WorldMatrix[1], float4(0.0f, U_VSGeneralRegister7.y, 0.0f, 1.0f));
	vsOut.o_BoundBoxRange.z = U_VSGeneralRegister7.z;
	vsOut.o_BoundBoxRange.w = U_VSGeneralRegister7.w;
#endif

	return vsOut;
}
