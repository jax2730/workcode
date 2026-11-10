cbuffer UniformBlock_Common_VS : register( b6 )
{
    float4	U_FogColorNear;
	float4	U_FogColorFar;
	float4	U_FogParam;
	float4  U_FogRampParam;
	float4	U_ViewProjectMatrix[4];
	float4	U_ViewMatrix[4];
	float4	U_ProjectMatrix[4];
	float4	U_ZeroViewProjectMatrix[4];
	float4	U_VS_CameraPosition;
	float4	U_CameraParam;
	float4	U_VS_ViewParam;
	float4	U_VS_RenderWindowParam;
	float4	U_ViewDirection;
	float4	U_ViewRightVector;
	float4	U_ViewUpVector;
	float4 	U_MainCamViewMatrix;
	float4	U_VS_MainLightDirection;
	float4	U_VS_MainLightColor;
	float4	U_VS_SkyLightColor;
	float4	U_VS_GroundLightColor;
	float4	U_VS_PBR_MainLightColor;
	float4	U_VS_PBR_SkyLightColor;
	float4	U_VS_PBR_GroundLightColor;
	float4	U_VS_Time;
	float4	U_WindParams;
	float4	U_VS_WindParams;
	float4	U_WindParamsGravitySpace;
	float4	U_GlobalAnimationParams;
	float4  U_VS_CloudCollision;
	float4	U_ShadowLightDepthMulAdd;
	float4	U_VS_GravityDirection;
	float4	U_VS_GravityRotationMatrix[4];
	float4  U_VS_AmbientBlendThreshold; // .y->pbr_light .z->role_light
	float4  U_VS_FPS;
	float4  U_RainVPMatrix[4];
};
#define  Bone_Matrix_Max_Cnt 72

static const float4 ShadowNormalOffset = float4(8, 6, 4, 4) * 1.4;
