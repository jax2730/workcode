#ifndef SHADOWSPLITE
#define SHADOWSPLITE 1
#endif

void ProjectToLightSpace(out float4 lightSpacePos, float4 lightMatrix[4], float3 pos)
{
    float4 worldPosH = float4(pos, 1.0);
    lightSpacePos.x = dot(lightMatrix[0], worldPosH);
    lightSpacePos.y = dot(lightMatrix[1], worldPosH);
    lightSpacePos.z = dot(lightMatrix[2], worldPosH);
    lightSpacePos.w = 1.0;
    lightSpacePos.xy = lightSpacePos.xy * float2(0.5, -0.5) + 0.5;
}

void ProjectToLightSpaceInternal(out float4 lightSpacePos[SHADOWSPLITE], float3 position, float3 normal, float viewZ
#if SHADOWSPLITE > 0
	, float4 lightMatrix0[4]
#endif
#if SHADOWSPLITE > 1
    , float4 lightMatrix1[4]
#endif
#if SHADOWSPLITE > 2
    , float4 lightMatrix2[4]
#endif
#if SHADOWSPLITE > 3
	, float4 lightMatrix3[4]
#endif
)
{
    float nDotL = clamp(dot(normal, -U_MainLightDirection.xyz), 0.05, 1.0);
    float4 normalBias = sqrt(1.0 - nDotL * nDotL) * U_ShadowOffsetScale;

#if SHADOWSPLITE > 0
    ProjectToLightSpace(lightSpacePos[0], lightMatrix0, position + normal * normalBias[0]);
#endif

#if SHADOWSPLITE > 1
    ProjectToLightSpace(lightSpacePos[1], lightMatrix1, position + normal * normalBias[1]);
#endif

#if SHADOWSPLITE > 2
    ProjectToLightSpace(lightSpacePos[2], lightMatrix2, position + normal * normalBias[2]);
#endif

#if SHADOWSPLITE > 3
    ProjectToLightSpace(lightSpacePos[3], lightMatrix3, position + normal * normalBias[3]);
#endif
}

void ProjectToLightSpaceInternal(out float4 lightSpacePos[SHADOWSPLITE], float3 position
#if SHADOWSPLITE > 0
	, float4 lightMatrix0[4]
#endif
#if SHADOWSPLITE > 1
    , float4 lightMatrix1[4]
#endif
#if SHADOWSPLITE > 2
    , float4 lightMatrix2[4]
#endif
#if SHADOWSPLITE > 3
	, float4 lightMatrix3[4]
#endif
	)
{
#if SHADOWSPLITE > 0
    ProjectToLightSpace(lightSpacePos[0], lightMatrix0, position);
#endif
#if SHADOWSPLITE > 1
    ProjectToLightSpace(lightSpacePos[1], lightMatrix1, position);
#endif
#if SHADOWSPLITE > 2
    ProjectToLightSpace(lightSpacePos[2], lightMatrix2, position);
#endif
#if SHADOWSPLITE > 3
    ProjectToLightSpace(lightSpacePos[3], lightMatrix3, position);
#endif
}

void ProjectToLightSpace(out float4 lightSpacePos[SHADOWSPLITE], float3 worldPos, float3 normal, float viewZ)
{
	ProjectToLightSpaceInternal(lightSpacePos, worldPos, normal, viewZ
#if SHADOWSPLITE > 0
	, U_LightVPMatrix0
#endif
#if SHADOWSPLITE > 1
    , U_LightVPMatrix1
#endif
#if SHADOWSPLITE > 2
    , U_LightVPMatrix2
#endif
#if SHADOWSPLITE > 3
	, U_LightVPMatrix3
#endif
	);
}

void ProjectToLightSpace(out float4 lightSpacePos[SHADOWSPLITE], float3 worldPos)
{
	ProjectToLightSpaceInternal(lightSpacePos, worldPos
#if SHADOWSPLITE > 0
	, U_LightVPMatrix0
#endif
#if SHADOWSPLITE > 1
    , U_LightVPMatrix1
#endif
#if SHADOWSPLITE > 2
    , U_LightVPMatrix2
#endif
#if SHADOWSPLITE > 3
	, U_LightVPMatrix3
#endif
	);
}

void ProjectToLightSpaceCameraBase(out float4 lightSpacePos[SHADOWSPLITE], float3 worldPosCamBase, float3 normal, float viewZ)
{
	ProjectToLightSpaceInternal(lightSpacePos, worldPosCamBase, normal, viewZ
#if SHADOWSPLITE > 0
	, U_LightVPMatrixView0
#endif
#if SHADOWSPLITE > 1
    , U_LightVPMatrixView1
#endif
#if SHADOWSPLITE > 2
    , U_LightVPMatrixView2
#endif
#if SHADOWSPLITE > 3
	, U_LightVPMatrixView3
#endif
	);
}

void ProjectToLightSpaceCameraBase(out float4 lightSpacePos[SHADOWSPLITE], float3 worldPosCamBase)
{
	ProjectToLightSpaceInternal(lightSpacePos, worldPosCamBase
#if SHADOWSPLITE > 0
	, U_LightVPMatrixView0
#endif
#if SHADOWSPLITE > 1
    , U_LightVPMatrixView1
#endif
#if SHADOWSPLITE > 2
    , U_LightVPMatrixView2
#endif
#if SHADOWSPLITE > 3
	, U_LightVPMatrixView3
#endif
	);
}
