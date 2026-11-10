
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
	float4	U_WorldMatrix[3];
	float4	U_InvTransposeWorldMatrix[3];
	float4	U_WindParamsObjSpace;
	float4  U_ObjectMirrored;
#endif

float4 	U_VSGeneralRegister7; 	//x ---boundingbox min.y (current entity box) local box
								//y ---boundingbox max.y (current entity box)
								//z ---boundingbox min.y (parent entity box)
								//w ---boundingbox max.y (parent entity box)

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

float4	U_VSGeneralRegister0;	// x: sway start height
float4	U_VSGeneralRegister1;	// x: sway amplitude, y: sway frequency

#ifdef SoftNoColor
	float4  U_InvWorldMatrix[3];
	float4  U_WorldScale;
#endif
#ifdef SOFTBOARD
	float4  U_WorldScale;
#endif

float4 TriangleWave(float4 x)
{
	return abs(frac(x) * 2.0 - 1.0);
}

float4 SmoothCurve(float4 x)
{
	return x * x * (3.0 - 2.0 * x);
}

float4 SmoothTriangleWave(float4 x)
{
	return SmoothCurve(TriangleWave(x));
}

void _TrunkBending(inout float3 vPos, in float fSwayStartHeight,
		in float fTime, in float fPhase, in float4 vWindParams,
		in float fAmp, in float fFreq)
{
	float fBendScale = max(vPos.y - fSwayStartHeight, 0.f);
	fBendScale = fBendScale * fBendScale * 0.001;
	float fLength = length(vPos);
	float3 vNewPos = vPos;
	float vWave = SmoothTriangleWave(float4(fFreq * (fTime + fPhase), 0.0, 0.0, 0.0)).x;
	vWave = vWave * 2.0 - 1.0;
	vNewPos.xz += vWave * vWindParams.xz * vWindParams.w * fBendScale * fAmp;
	vPos = normalize(vNewPos) * fLength;
}

struct VS_INPUT
{
	float4 i_Pos			: POSITION0;
#ifdef SHADOWPASS
	#ifdef ALPHA_TEST
		float4 i_UV			: TEXCOORD0;	
	#endif
#else
	float4 i_Normal			: NORMAL0;
	float4 i_UV				: TEXCOORD0;	
	float4 i_Tangent		: TANGENT0;
	#ifdef VERTEX_COLOR
		float4	i_Diffuse	: COLOR0;
	#endif
#endif

#ifdef HWSKINNED
	float4	i_BlendWeight	: BLENDWEIGHT0;
	uint4	i_BlendIndices	: BLENDINDICES0;
#endif
};

struct VS_OUTPUT
{
	float4	o_PosInClip		: SV_POSITION;
#ifdef SHADOWPASS
	#ifdef ALPHA_TEST
		float4 o_UV			: TEXCOORD0;	
	#endif
#else
	float4 	o_UV			: TEXCOORD0;	// w: ViewSpaceZ if USESHADOWMAP
	float4	o_Fog			: TEXCOORD1;
	float4	o_Normal		: TEXCOORD2;
	float4	o_WorldPos		: TEXCOORD3;
	
	float4	o_Tangent		: TEXCOORD4;
	float4	o_Binormal		: TEXCOORD5;

	#ifdef USESHADOWMAP
		float4  O_LightSpacePos[SHADOWSPLITE] : TEXCOORD6; 
	#endif
	
	#ifdef VERTEX_COLOR
		float4 o_Diffuse	: COLOR0;
	#endif
	#ifdef SOFTBOARD
		float4 o_ClipPos    : TEXCOORD8;
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
	float4 vObjPos = float4(IN.i_Pos.xyz, 1.0f);
	vsOut.o_PosInModel = vObjPos;
	#ifdef HWSKINNED
		float4	Weight		= IN.i_BlendWeight;
		uint4  Index		= IN.i_BlendIndices;

		float4 BoneMatC0 = float4(0.0, 0.0, 0.0, 0.0);
		float4 BoneMatC1 = float4(0.0, 0.0, 0.0, 0.0);
		float4 BoneMatC2 = float4(0.0, 0.0, 0.0, 0.0);

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
		#endif
		vObjPos = float4(dot(BoneMatC0, vObjPos), dot(BoneMatC1, vObjPos), dot(BoneMatC2, vObjPos), 1.f);
	#endif

	float3x4 matWorld = (float3x4)0;
	float4x4 matInvTransposeWorld = (float4x4)0;
	float4x4 matWVP = (float4x4)0;
	float4 vWindParamsObjSpace = float4(1.f, 0.f, 0.f, 1.f);
	float fHandedness = 1.f;

#ifdef USEINSTANCE
	int idxInst = i_InstanceID * UniformCount;

	float3 vPosition =    U_VSCustom0[idxInst + 0];
	float3 vScale =       U_VSCustom0[idxInst + 1];
	float4 vOrientation = U_VSCustom0[idxInst + 2];

	#ifdef SKINNEDINSTANCE
		float3 vMVPosition = vPosition;
		vPosition.x += U_VS_CameraPosition.x;
		vPosition.y += U_VS_CameraPosition.y;
		vPosition.z += U_VS_CameraPosition.z;

		float4x4 _matWorld = MakeTransform( vMVPosition.xyz, vScale.xyz, vOrientation);

		matWorld[0] = _matWorld[0];
		matWorld[1] = _matWorld[1];
		matWorld[2] = _matWorld[2];
		// translate to world space
		matWorld[0][3] += U_VS_CameraPosition.x;
		matWorld[1][3] += U_VS_CameraPosition.y;
		matWorld[2][3] += U_VS_CameraPosition.z;
	#else
		// float3 vMVPosition = vPosition;
		// vMVPosition.x -= U_VS_CameraPosition.x;
		// vMVPosition.y -= U_VS_CameraPosition.y;
		// vMVPosition.z -= U_VS_CameraPosition.z;
		float4x4 _matWorld = MakeTransform( vPosition.xyz, vScale.xyz, vOrientation);
		matWorld[0] = _matWorld[0];
		matWorld[1] = _matWorld[1];
		matWorld[2] = _matWorld[2];
		// translate to camera space
		_matWorld[0][3] -= U_VS_CameraPosition.x;
		_matWorld[1][3] -= U_VS_CameraPosition.y;
		_matWorld[2][3] -= U_VS_CameraPosition.z;
	#endif

	matInvTransposeWorld = MakeInverseTransform(vPosition.xyz, vScale.xyz, vOrientation);
	matInvTransposeWorld = transpose(matInvTransposeWorld);
	matWVP = mul((float4x4)U_ZeroViewProjectMatrix, _matWorld);

	// transform wind parameters from world space to object space
	float4x4 matRotation = MakeRotation(vOrientation);
	vWindParamsObjSpace = mul(transpose(matRotation), U_WindParams);

	fHandedness = determinant(float3x3(matWorld[0].xyz, matWorld[1].xyz, matWorld[2].xyz));
	fHandedness = fHandedness >= 0.f ? 1.f : -1.f;
#else

	matWorld[0] = U_WorldMatrix[0];
	matWorld[1] = U_WorldMatrix[1];
	matWorld[2] = U_WorldMatrix[2];

	matInvTransposeWorld[0] = U_InvTransposeWorldMatrix[0];
	matInvTransposeWorld[1] = U_InvTransposeWorldMatrix[1];
	matInvTransposeWorld[2] = U_InvTransposeWorldMatrix[2];

	matWVP[0] = U_WorldViewProjectMatrix[0];
	matWVP[1] = U_WorldViewProjectMatrix[1];
	matWVP[2] = U_WorldViewProjectMatrix[2];
	matWVP[3] = U_WorldViewProjectMatrix[3];

	vWindParamsObjSpace = U_WindParamsObjSpace;

	fHandedness = U_ObjectMirrored.w;
#endif

	float fBendPhase = matWorld[0][3] + matWorld[1][3] + matWorld[2][3];
	_TrunkBending(vObjPos.xyz, U_VSGeneralRegister0.x, U_VS_Time.x, fBendPhase,
			vWindParamsObjSpace, U_VSGeneralRegister1.x, U_VSGeneralRegister1.y);

	vsOut.o_PosInClip = mul(matWVP, vObjPos);

#ifdef PARABOLOID_MAPPING
	vsOut.o_ClipDepth = vsOut.o_PosInClip.z;
	ParaboloidMapping(vsOut.o_PosInClip, U_ShadowLightDepthMulAdd.x, U_ShadowLightDepthMulAdd.z, U_ShadowLightDepthMulAdd.w);
#endif

#ifdef SHADOWPASS
	#ifdef ALPHA_TEST
		vsOut.o_UV.xy = IN.i_UV.xy;
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
				int idx = IN.i_BlendIndices[i];
				lastBoneMatC0	+=	U_LastBoneMatrix[idx*3+0]*IN.i_BlendWeight[i];	
				lastBoneMatC1	+=	U_LastBoneMatrix[idx*3+1]*IN.i_BlendWeight[i];	
				lastBoneMatC2	+=	U_LastBoneMatrix[idx*3+2]*IN.i_BlendWeight[i];	
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
	vsOut.o_WorldPos.xyz = mul(matWorld, vObjPos);
	vsOut.o_WorldPos.w = 1.0;
	//vsOut.o_UV = IN.i_UV;
	#ifdef SOFTBOARD
		vsOut.o_ClipPos = vsOut.o_PosInClip;
		vsOut.o_UV.z = U_WorldScale.x;
	#endif
	float4 normal =  float4(IN.i_Normal.xyz, 0.0f);
	float4 tangent =  float4(IN.i_Tangent.xyz, 0.0f);
		
	#ifdef HWSKINNED
		normal = float4(dot(BoneMatC0, normal), dot(BoneMatC1, normal), dot(BoneMatC2, normal), 0.0f);
		tangent = float4(dot(BoneMatC0, tangent), dot(BoneMatC1, tangent), dot(BoneMatC2, tangent), 0.0f);
	#endif

	vsOut.o_Normal.xyz = mul(matInvTransposeWorld, normal);
	vsOut.o_Tangent.xyz = mul(matWorld, tangent);
	
	vsOut.o_Binormal.xyz = cross(vsOut.o_Tangent.xyz, vsOut.o_Normal.xyz) * IN.i_Tangent.w;
	vsOut.o_Binormal.xyz *= fHandedness;
	
	vsOut.o_UV.xy = IN.i_UV.xy;
	
	float3 camToWorld =  U_VS_CameraPosition.xyz - vsOut.o_WorldPos.xyz;
	#ifdef SoftNoColor
		float3 camtoposi = normalize(vsOut.o_WorldPos.xyz - U_VS_CameraPosition.xyz);
		float3 viewDir = normalize(U_ViewDirection.xyz);
		float temp = dot(camtoposi, viewDir);
		temp = max(temp, 0.000001);
		vsOut.o_WorldPos.xyz += camtoposi * U_WorldScale.x/ temp;
		
		float4 tempLocalPosi = vsOut.o_WorldPos;
		tempLocalPosi.x = dot(U_InvWorldMatrix[0], vsOut.o_WorldPos);
		tempLocalPosi.y = dot(U_InvWorldMatrix[1], vsOut.o_WorldPos);
		tempLocalPosi.z = dot(U_InvWorldMatrix[2], vsOut.o_WorldPos);
		
		vsOut.o_PosInClip.x = dot(U_WorldViewProjectMatrix[0], tempLocalPosi);
		vsOut.o_PosInClip.y = dot(U_WorldViewProjectMatrix[1], tempLocalPosi);
		vsOut.o_PosInClip.z = dot(U_WorldViewProjectMatrix[2], tempLocalPosi);
		vsOut.o_PosInClip.w = dot(U_WorldViewProjectMatrix[3], tempLocalPosi);
	#endif
	
	#ifdef VERTEX_COLOR
		vsOut.o_Diffuse = IN.i_Diffuse;
	#endif

	float4x4 gravityRotatePosMatrix;	
	gravityRotatePosMatrix[0] = U_VS_GravityRotationMatrix[0];
	gravityRotatePosMatrix[1] = U_VS_GravityRotationMatrix[1];
	gravityRotatePosMatrix[2] = U_VS_GravityRotationMatrix[2];
	gravityRotatePosMatrix[3] = U_VS_GravityRotationMatrix[3];
	camToWorld = mul(gravityRotatePosMatrix,float4(camToWorld.xyz,0.0f)).xyz;
	
	vsOut.o_Fog = ComputeFogColor(camToWorld, U_FogParam.xy, U_FogParam.zw, U_FogRampParam, U_FogColorNear, U_FogColorFar);

	#ifdef USESHADOWMAP
		float viewZ = -dot(U_MainCamViewMatrix, float4(vsOut.o_WorldPos.xyz, 1.0f));
	
	    float4 wpos = vsOut.o_WorldPos;
		float3 wNormal = normalize(vsOut.o_Normal.xyz);
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
		vsOut.o_UV.w = viewZ;
	#endif
	#ifdef EDITOR_MODE
		#ifdef HierarchicalLOD
			vsOut.o_PosInClip = float4(2.0f * IN.i_UV.x - 1.0f, 1.0f - 2.0f * IN.i_UV.y, 1.0f, 1.0f);
		#endif
	#endif
#endif
#ifndef SHADOWPASS
	vsOut.o_BoundBoxRange.x = U_VSGeneralRegister7.x;//dot(matWorld[1], float4(0.0f, U_VSGeneralRegister7.x, 0.0f, 1.0f));
	vsOut.o_BoundBoxRange.y = U_VSGeneralRegister7.y;//dot(matWorld[1], float4(0.0f, U_VSGeneralRegister7.y, 0.0f, 1.0f));
	vsOut.o_BoundBoxRange.z = U_VSGeneralRegister7.z;
	vsOut.o_BoundBoxRange.w = U_VSGeneralRegister7.w;
#endif
	return vsOut;
}

