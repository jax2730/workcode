#include <Common_Uniform_VS.inl>
#include <Common_Function.inl> 

float4	U_WorldViewProjectMatrix[4];

float4	U_VSGeneralRegister0;	// x: sway frequency, y: sway amplitude

float4	U_VSGeneralRegister1;	// xyz:  Box half size
float4	U_VSGeneralRegister2;	// x: Penetrate Beg, y: Penetrate End
float4	U_VSGeneralRegister3;	// xyz:  Box center

float4	U_VSGeneralRegister4;	// x: fade start, y: 1 / fadedist, z: fade end	// near camera

uniform float4 U_VSCustom0; 	// sphWorldPos;
uniform float4 U_VSCustom1; 	// RoleCamSpaPos;	
uniform float4 U_VSCustom2; 	// xy: VegBound MinY/MaxY, zw: scale base/scale range

uniform float4 U_VSCustom3; 	// vegSpaToCam;

uniform float4 U_VSCustom5; 	// uvPercent;	

// Get a quaternion inverse
float4 quaternion_inverse(in float4 quat)
{
	float fNorm = quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w;
	float fInvNorm = 1.0f/fNorm;
	return float4( -quat.xyz * fInvNorm, quat.w * fInvNorm);
}

// Rotate a vector with quaternion
float3 quaternion_mul(in float4 quat, in float3 v)
{
	float3 uv = cross(quat.xyz, v);
	float3 uuv = cross(quat.xyz, uv);
	uv *= 2.f * quat.w;
	uuv *= 2.f;
	return v + uv + uuv;
}

// generate a triangle wave from a linear wave, range: 0~1
float TriangleWave(float fValue)
{
	return abs(frac(fValue) * 2.0 - 1.0);
}

float4 getQuatFormAngleAxis(float angle, float3 axis)
{
	float4 retQuat = float4(0.f, 0.f, 0.f, 0.f);
	float halfAngle = angle * 0.5f;
	float fSin = sin(halfAngle);

	retQuat.w = cos(halfAngle);
	retQuat.x = fSin*axis.x;
	retQuat.y = fSin*axis.y;
	retQuat.z = fSin*axis.z;
	return retQuat;
}

float2 getUVIndexPercent(float2 uv, float r, float4 percent)
{
	r -= percent.x;
	if (r < 0.f) return uv;

	r -= percent.y;
	if (r < 0.f) return float2(uv.x + 0.5f, uv.y);
	
	r -= percent.z;
	if (r < 0.f) return float2(uv.x, uv.y + 0.5f);
	
	r -= percent.w;
	if (r < 0.f) return float2(uv.x + 0.5f, uv.y + 0.5f);

	return uv;
}

struct VS_INPUT
{
	float3 i_MeshPos : POSITION0;
	float3 i_MeshNor : NORMAL0;
	float3 i_MeshTan : TANGENT0;
	float2 i_MeshTex : TEXCOORD0;
	float4 i_MeshCol : COLOR0;

	float4 i_VegPos  : POSITION1;		//	xyz: ins world pos, w: scale
	float4 i_VegNor	 : NORMAL1;			//	rotate quaternion 
};

struct VS_OUTPUT
{
	float4	o_PosInClip	: SV_POSITION;
	float4	o_UV		: TEXCOORD0;		//z for camera spec pos;
	float4	o_Fog		: TEXCOORD1;
	float4  o_WPos		: TEXCOORD2;		//	xyz: world pos
	float3	o_WNormal	: TEXCOORD3;

#ifdef BUMP_MAP
	float3	o_Tangent	: TEXCOORD4;
	float3	o_Binormal	: TEXCOORD5;
#endif

	float4  o_Diffuse		: COLOR0;		//	xyz: mesh color, w: height factor
	float4 	o_FadeFac		: COLOR1;		//	x: common, y: fade out, z: fade in, w: penetrate value

#ifdef USESHADOWMAP
	float4  O_LightSpacePos[SHADOWSPLITE] : TEXCOORD6;
#endif
};

VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT vsOut = (VS_OUTPUT)0;

	//	Get vegetation base data	
	//	----------------------------------------------------------
	float3 RolePushPos = U_VSCustom1.xyz;										//	role cam position
	float3 vVegRoot =  IN.i_VegPos.xyz;											//	veg world root position 

	float3 vMeshPos = IN.i_MeshPos;												//	veg mesh space position
	float3 vMeshCenP= U_VSGeneralRegister3.xyz;									//	veg mesh center position in veg space		
	float3 vMeshNor = IN.i_MeshNor;												//	veg mesh space normal
	float4 vMeshQua = IN.i_VegNor * 2.f - 1.f;									//	veg rotate quaternion
	float  vMeshHeightFactor = (IN.i_MeshPos.y -  U_VSCustom2.x) / (U_VSCustom2.y - U_VSCustom2.x);	
	//	----------------------------------------------------------
	vsOut.o_Diffuse.xyz = IN.i_MeshCol.xyz;
	vsOut.o_Diffuse.w = vMeshHeightFactor;

	//	Veg mesh random rotate
	float4 rotateQuat = getQuatFormAngleAxis(IN.i_VegPos.w * 3.1415926, float3(0.f, 1.f, 0.f));
	vMeshPos = quaternion_mul(rotateQuat, vMeshPos);
	vMeshNor  = quaternion_mul(rotateQuat, vMeshNor);
	vMeshCenP = quaternion_mul(rotateQuat, vMeshCenP);


	//	Add veg wind animation
	//	----------------------------------------------------------
#ifdef WIND_ANIMATION
	float4 vAnimParams = U_VSGeneralRegister0 * U_GlobalAnimationParams.yxwz;
	float2 vWaveBase = vAnimParams.x * U_VS_Time.x;
	vWaveBase += vVegRoot.xz * float2(0.1f, 0.2f);				//	each veg have diff wave form root pos
	float3 vWave = vWaveBase.xyy * float3(1.125f, 0.975f, 0.397f);
	vWave = sin(vWave);

	// swaying
	float swayFactor = vMeshHeightFactor;
	float fSway = vAnimParams.y * swayFactor * swayFactor;

	// Get scene wind dir
	float4 vQuaInv = quaternion_inverse(vMeshQua);
	float3 windMeshSpaceDir = quaternion_mul(vQuaInv, U_WindParams.xyz);

	float3 vOff = float3(0.f, 0.f, 0.f);
	vOff.xz += windMeshSpaceDir.xz * vWave.x * fSway;										
	vOff.xz += (float2(-windMeshSpaceDir.z, windMeshSpaceDir.x)) * vWave.y * vWave.z * fSway;	// perpendicular wind direction

	// shaking
	float fShakeFreq = vAnimParams.z * U_VS_Time.x;
	fShakeFreq += dot(vMeshPos, 2);	// phase
	float fShakeAmp = vAnimParams.w * swayFactor * swayFactor;
	vOff += normalize(vMeshNor + windMeshSpaceDir.xyz) * TriangleWave(fShakeFreq) * fShakeAmp;

	// push by role
	float3 vRoleToVeg = vVegRoot +  U_VSCustom3.xyz - RolePushPos.xyz;
	float fRoleToVegLen = length(vRoleToVeg);

	float3 RoleToVegMeshSpaceDir = quaternion_mul(vQuaInv, normalize(vRoleToVeg));	
	RoleToVegMeshSpaceDir = normalize(RoleToVegMeshSpaceDir);
	float2 vegPushDir = RoleToVegMeshSpaceDir.xz;		// push direction

	float fPushLen = clamp(U_VSGeneralRegister4.w - fRoleToVegLen, 0.0f, U_VSGeneralRegister4.w);
	float fLenFactor = 1.0f - clamp(fPushLen / U_VSGeneralRegister4.w, 0.0f, 1.0f);
	fLenFactor = min(fLenFactor * 5.0f, 1.0f);
	vOff.xz += vegPushDir * vMeshPos.y * fPushLen * fLenFactor;

	float fLen = length(vMeshPos);
	vMeshPos += vOff;
	vMeshPos = normalize(vMeshPos) * fLen;
#endif
	//	----------------------------------------------------------

	//	Vegetation fade
	//	----------------------------------------------------------
	float3 vCamToVeg = vVegRoot +  U_VSCustom3.xyz;
	float fLenCV = length(vCamToVeg);	

	//	limit show range
	vMeshPos *= fLenCV > U_VSGeneralRegister4.z ? 0.f : 1.f;

	float fFadeFar = (fLenCV - U_VSGeneralRegister4.x) * U_VSGeneralRegister4.y;
	fFadeFar = clamp(fFadeFar, 0.f, 1.f);
	vsOut.o_FadeFac = float4(1.f, fFadeFar, 1.f, 1.f);
	//	----------------------------------------------------------

	//	Mesh scale
	float realScale = U_VSCustom2.z + U_VSCustom2.w * IN.i_VegPos.w;
	vMeshPos *= realScale;
	vMeshCenP *= realScale;

	// rotate to mesh normalize
	vMeshPos = quaternion_mul(vMeshQua, vMeshPos);
	vMeshNor  = quaternion_mul(vMeshQua, vMeshNor);
	vMeshCenP = quaternion_mul(vMeshQua, vMeshCenP);
	vsOut.o_WNormal = vMeshNor;

	float3 vMeshWorldPos = vMeshPos + vVegRoot + U_VSCustom0.xyz;
	vsOut.o_WPos.xyz = vMeshWorldPos;
	vsOut.o_WPos.w = 1.f;
	vsOut.o_PosInClip = mul((float4x4)U_WorldViewProjectMatrix, float4(vMeshPos + (vVegRoot + U_VSCustom3.xyz), 1.f));

	vsOut.o_UV.xy = getUVIndexPercent(IN.i_MeshTex.xy, IN.i_VegPos.w / 2.f + 0.5f, U_VSCustom5);

	//	Penetrate about
	//	----------------------------------------------------------
	float vegRealHalfSize = length(U_VSGeneralRegister1.xyz) * realScale;
	float3 camSpaceCenP = vVegRoot + vMeshCenP + U_VSCustom3.xyz;
	float camDisToCenP = length(camSpaceCenP);
	float disProportion = camDisToCenP / vegRealHalfSize;
	vsOut.o_FadeFac.w  = saturate((U_VSGeneralRegister2.y - disProportion) / (U_VSGeneralRegister2.y - U_VSGeneralRegister2.x));

	//	Lighting about
	//	----------------------------------------------------------

	#ifdef BUMP_MAP
		float3 vMeshTangent = IN.i_MeshTan.xyz;
		vMeshTangent  = quaternion_mul(rotateQuat, vMeshTangent);
		vMeshTangent = quaternion_mul(vMeshQua, vMeshTangent);
		float3 vMeshBinormal = cross(vMeshTangent, vMeshNor);

		vsOut.o_Tangent = vMeshTangent;
		vsOut.o_Binormal = vMeshBinormal;
	#endif
	//	----------------------------------------------------------

	//	Fog 
	//	----------------------------------------------------------
	float4x4 gravityRotatePosMatrix;	
	gravityRotatePosMatrix[0] = U_VS_GravityRotationMatrix[0];
	gravityRotatePosMatrix[1] = U_VS_GravityRotationMatrix[1];
	gravityRotatePosMatrix[2] = U_VS_GravityRotationMatrix[2];
	gravityRotatePosMatrix[3] = U_VS_GravityRotationMatrix[3];

	float3 vCamToWorld = -(vMeshPos + (vVegRoot + U_VSCustom3.xyz));
	vCamToWorld = mul(gravityRotatePosMatrix,float4(vCamToWorld.xyz,0.0f)).xyz;
	vsOut.o_Fog = ComputeFogColor(vCamToWorld, U_FogParam.xy, U_FogParam.zw, U_FogRampParam, U_FogColorNear, U_FogColorFar);
	//	----------------------------------------------------------

	//	Shadow map  
	//	----------------------------------------------------------
	#ifdef USESHADOWMAP
		#if SHADOWSPLITE > 0
			vsOut.O_LightSpacePos[0].x = dot(U_LightVPMatrix0[0], vsOut.o_WPos);
			vsOut.O_LightSpacePos[0].y = dot(U_LightVPMatrix0[1], vsOut.o_WPos);
			vsOut.O_LightSpacePos[0].z = dot(U_LightVPMatrix0[2], vsOut.o_WPos);
			vsOut.O_LightSpacePos[0].w = dot(U_LightVPMatrix0[3], vsOut.o_WPos);
		#endif

		#if SHADOWSPLITE > 1
			vsOut.O_LightSpacePos[1].x = dot(U_LightVPMatrix1[0], vsOut.o_WPos);
			vsOut.O_LightSpacePos[1].y = dot(U_LightVPMatrix1[1], vsOut.o_WPos);
			vsOut.O_LightSpacePos[1].z = dot(U_LightVPMatrix1[2], vsOut.o_WPos);
			vsOut.O_LightSpacePos[1].w = dot(U_LightVPMatrix1[3], vsOut.o_WPos);
		#endif

		#if SHADOWSPLITE > 2
			vsOut.O_LightSpacePos[2].x = dot(U_LightVPMatrix2[0], vsOut.o_WPos);
			vsOut.O_LightSpacePos[2].y = dot(U_LightVPMatrix2[1], vsOut.o_WPos);
			vsOut.O_LightSpacePos[2].z = dot(U_LightVPMatrix2[2], vsOut.o_WPos);
			vsOut.O_LightSpacePos[2].w = dot(U_LightVPMatrix2[3], vsOut.o_WPos);
		#endif

		#if SHADOWSPLITE > 3
			vsOut.O_LightSpacePos[3].x = dot(U_LightVPMatrix3[0], vsOut.o_WPos);
			vsOut.O_LightSpacePos[3].y = dot(U_LightVPMatrix3[1], vsOut.o_WPos);
			vsOut.O_LightSpacePos[3].z = dot(U_LightVPMatrix3[2], vsOut.o_WPos);
			vsOut.O_LightSpacePos[3].w = dot(U_LightVPMatrix3[3], vsOut.o_WPos);
		#endif

		vsOut.o_UV.z = -dot(U_MainCamViewMatrix, vsOut.o_WPos);
	#endif

	return vsOut;
}
