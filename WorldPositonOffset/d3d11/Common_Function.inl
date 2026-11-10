
float2 fogExp3 (in float3 cameraToWorldPos,in float2 heightScale,in float2 volFogHeightDensity,in float4 fogRamp)
{
	float2 fogInt = float2(1.0f,1.0f);
	static const float c_slopeThreshold = 0.01f;
	float2 top = heightScale * cameraToWorldPos.y;
	if( abs( top.x ) > c_slopeThreshold )
	{
		fogInt.x *= (exp(top.x) - 1.0f) / top.x;
	}
	
	if( abs( top.y ) > c_slopeThreshold )
	{
		fogInt.y *= (exp(top.y) - 1.0f) / top.y;
	}

	float l = length( cameraToWorldPos );
	float2 len = float2(l,l);
	float2 u = len * volFogHeightDensity;
	fogInt *= u;

	float2 fFactor = saturate( exp2( -fogInt ) );

	float2 vRampDen = saturate( len * fogRamp.xy - fogRamp.zw);
	vRampDen = vRampDen * (2.f - vRampDen);
	fFactor = vRampDen - fFactor * vRampDen; 

	return fFactor;
}

float4 ComputeFogColor(in float3 cameraToWorldPos,in float2 heightScale,in float2 volFogHeightDensity,
	in float4 fogRamp,in float4 vFogColorNear, in float4 vFogColorFar)
{
	float2 fFactor = fogExp3(cameraToWorldPos, heightScale, volFogHeightDensity, fogRamp);
	float2 fFactorDiff = 1.0f - fFactor;
	float4 fogColor = fFactor.x * fFactorDiff.y * vFogColorNear + fFactor.y * vFogColorFar;
	fogColor.a = fFactorDiff.x * fFactorDiff.y;
	return fogColor;
}

float4 texSample2D(Texture2D tex,float2 uv,SamplerState samplerState )
{
   return  tex.Sample(samplerState, uv);
}

float4x4 MakeRotation(in float4 ori)
{
	float fTx  = ori.x + ori.x;
	float fTy  = ori.y + ori.y;
	float fTz  = ori.z + ori.z;
	float fTwx = fTx * ori.w;
	float fTwy = fTy * ori.w;
	float fTwz = fTz * ori.w;
	float fTxx = fTx * ori.x;
	float fTxy = fTy * ori.x;
	float fTxz = fTz * ori.x;
	float fTyy = fTy * ori.y;
	float fTyz = fTz * ori.y;
	float fTzz = fTz * ori.z;

	// rotation
	float4x4 m = {
			1.0f - (fTyy + fTzz), fTxy - fTwz,          fTxz + fTwy,          0.0f,
			fTxy + fTwz,          1.0f - (fTxx + fTzz), fTyz - fTwx,          0.0f,
			fTxz - fTwy,          fTyz + fTwx,          1.0f - (fTxx + fTyy), 0.0f,
			0.0f,                 0.0f,                 0.0f,                 1.0f
			};
	return m;
}

float4x4 MakeScale(in float fScale)
{
	float4x4 m = {
		fScale,	   0.f,	   0.f, 0.f,
		   0.f, fScale,    0.f, 0.f,
		   0.f,    0.f, fScale, 0.f,
		   0.f,    0.f,    0.f, 1.f
	};
	return m;
}

float4x4 MakeScale(in float3 vScale)
{
	float4x4 m = {
		vScale.x, 0.f,	    0.f,      0.f,
		     0.f, vScale.y, 0.f,      0.f,
		     0.f, 0.f,      vScale.z, 0.f,
		     0.f, 0.f,      0.f,      1.f
	};
	return m;

}

float4x4 MakeTranslate(in float3 vPos)
{
	float4x4 m = {
		1.f, 0.f, 0.f, vPos.x,
		0.f, 1.f, 0.f, vPos.y,
		0.f, 0.f, 1.f, vPos.z,
		0.f, 0.f, 0.f, 1.f,
	};
	return m;
}

float4x4 MakeTransform(in float3 vPos, in float fScale, in float4 ori)
{
	float fTx  = ori.x + ori.x;
	float fTy  = ori.y + ori.y;
	float fTz  = ori.z + ori.z;
	float fTwx = fTx * ori.w;
	float fTwy = fTy * ori.w;
	float fTwz = fTz * ori.w;
	float fTxx = fTx * ori.x;
	float fTxy = fTy * ori.x;
	float fTxz = fTz * ori.x;
	float fTyy = fTy * ori.y;
	float fTyz = fTz * ori.y;
	float fTzz = fTz * ori.z;

	// rotation
	float4x4 m = {
			1.0f - (fTyy + fTzz), fTxy - fTwz,          fTxz + fTwy,          0.0f,
			fTxy + fTwz,          1.0f - (fTxx + fTzz), fTyz - fTwx,          0.0f,
			fTxz - fTwy,          fTyz + fTwx,          1.0f - (fTxx + fTyy), 0.0f,
			0.0f,                 0.0f,                 0.0f,                 1.0f
			};
	// uniform scaling
	m *= fScale;
	// translate
	m[0][3] = vPos.x;
	m[1][3] = vPos.y;
	m[2][3] = vPos.z;
	m[3][3] = 1.0f;

	return m;
}

float4x4 MakeTransform(in float3 vPos, in float3 vScale, in float4 ori)
{
	float fTx  = ori.x + ori.x;
	float fTy  = ori.y + ori.y;
	float fTz  = ori.z + ori.z;
	float fTwx = fTx * ori.w;
	float fTwy = fTy * ori.w;
	float fTwz = fTz * ori.w;
	float fTxx = fTx * ori.x;
	float fTxy = fTy * ori.x;
	float fTxz = fTz * ori.x;
	float fTyy = fTy * ori.y;
	float fTyz = fTz * ori.y;
	float fTzz = fTz * ori.z;

	// rotation
	float4x4 m = {
			1.0f - (fTyy + fTzz), fTxy - fTwz,          fTxz + fTwy,          0.0f,
			fTxy + fTwz,          1.0f - (fTxx + fTzz), fTyz - fTwx,          0.0f,
			fTxz - fTwy,          fTyz + fTwx,          1.0f - (fTxx + fTyy), 0.0f,
			0.0f,                 0.0f,                 0.0f,                 1.0f
			};
	// scaling
	m = mul(m, MakeScale(vScale));
	// translate
	m[0][3] = vPos.x;
	m[1][3] = vPos.y;
	m[2][3] = vPos.z;
	m[3][3] = 1.0f;

	return m;
}

float4x4 MakeInverseTransform(in float3 vPos, in float3 vScale, in float4 ori)
{
	// inverse
	vPos = -vPos;
	vScale = 1.f / vScale;
	ori.xyz = -ori.xyz;
	float4x4 m = MakeRotation(ori);
	float3 vInvTranslate = mul(m, float4(vPos, 0.f)).xyz;
	vInvTranslate *= vScale;
	m[0][0] *= vScale[0];	m[0][1] *= vScale[0];	m[0][2] *= vScale[0];	m[0][3] = vInvTranslate[0];
	m[1][0] *= vScale[1];	m[1][1] *= vScale[1];	m[1][2] *= vScale[1];	m[1][3] = vInvTranslate[1];
	m[2][0] *= vScale[2];	m[2][1] *= vScale[2];	m[2][2] *= vScale[2];	m[2][3] = vInvTranslate[2];
	m[3][0] = 0.f;			m[3][1] = 0.f;			m[3][2] = 0.f;			m[3][3] = 1.f;
	return m;
}

// Assuming LH-posV.z has been reversed.
void ParaboloidMapping(inout float4 posV, float xyMul, float depthMul, float depthAdd)
{
	float dist = length(posV.xyz);
	posV.xyz /= dist;
	// reflect to d0 = (0, 0, 1)
	// half vector h = p' + d0
	// p'x = hx / hz
	// p'y = hy / hz
	posV.xy /= posV.z + 1.0;
	posV.xy *= xyMul;
	//posV.z = (dist - nearDist) / (farDist - nearDist);
	posV.z = depthMul * dist + depthAdd;
	posV.w = 1.0;
}

float ClampToNearPlane(float z)
{
#ifdef REVERSE_DEPTH
	return min(1.0, z);
#else
	return max(0.0, z);
#endif
}
