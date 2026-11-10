cbuffer UniformBlock_Common_PS : register( b6 ) 
{
    float4 	U_MainLightDirection;
	float4	U_MainLightColor; //linear
	float4 	U_AuxiliaryLightDirection;
	float4	U_AuxiliaryLightColor; //linear
	float4	U_SkyLightColor; //linear
	float4	U_GroundLightColor; //linear
	float4	U_PBR_MainLightColor; //linear
	float4	U_PBR_SkyLightColor; //linear
	float4	U_PBR_GroundLightColor; //linear
	float4	U_RoleMainLightColor; //linear
	float4	U_RoleSkyLightColor; //linear
	float4	U_RoleGroundLightColor; //linear
	float4	U_SunColor; //linear
	float4	U_WaterShallowColor; //gamma
	float4	U_WaterDeepColor; //gamma
	float4	U_WaterFoamColor; //gamma
	float4  U_WaterFoamParams; 
	float4	U_WaterReflectColor; //gamma
	float4	U_UnderWaterColor; //gamma
	float4	U_ViewParam;
	float4	U_EnvironmentCubeMapMipLevels;
	float4	U_DepthRTParam;
	float4	U_InvProjectMatrix[4];
	float4	U_PS_CameraPosition;
	float4	U_PS_ViewDirection;
	float4	U_PS_Time;
	float4	U_PS_WindParams;
	float4	U_WeatherParam0; //linear
	float4	U_WeatherParam1;
	float4	U_WeatherParam2;
	float4  U_CoverColorParams;
	float4  U_CoverColorRotation;
	float4	U_GlobalAgingColour; //gamma
	float4  U_ShadowSplit; //xyz for distance of each split, w for shadowfadeout distance;
	float4  U_ShadowTexOffset;//xyz for texture sample offset, w for shadow strenth
	float4  U_ShadowEdgeFactor;	//x:shadow edge
	float4	U_ShadowOffsetScale;
	int4    U_ShadowUsePlaneDepthBias;
	float4  U_CascadeShadowSettings[2];
	float4 	U_LightVPMatrix0[4];
	float4 	U_LightVPMatrix1[4];
	float4 	U_LightVPMatrix2[4];	
	float4 	U_LightVPMatrix3[4];
	float4 	U_LightVPMatrixView0[4];  
	float4 	U_LightVPMatrixView1[4];
	float4 	U_LightVPMatrixView2[4];
	float4 	U_LightVPMatrixView3[4];
	float4  U_PS_CloudCollision;
	float4	U_PS_GravityDirection;
	float4	U_PS_GravityRotationMatrix[4];
	float4  U_PS_AmbientBlendThreshold; // .y->pbr_light .z->role_light
	float4  U_ForwardSSREnable;
	float4	U_PS_ViewLightPercent;
	
	float4x4 U_ShadowLightViewMatrixArray[MAX_POINT_LIGHT_PROBE_COUNT];
	// px py pz innerRad2
	float4 U_ShadowLightWorldPosArray[MAX_POINT_LIGHT_PROBE_COUNT];
	// dx dy dz outerRad2
	float4 U_ShadowLightDiffuseColorArray[MAX_POINT_LIGHT_PROBE_COUNT]; //linear
	// cr cg cb cosOuterAng
	float4 U_ShadowLightDirectionOuterAngleArray[MAX_POINT_LIGHT_PROBE_COUNT];
	// cosInnerAng invDeltaCosAngle invDeltaSqrRadius 0.0
	float4 U_ShadowLightInnerAngleArray[MAX_POINT_LIGHT_PROBE_COUNT];
	// n	f	f/(f-n)		-nf/(f-n)
	float4 U_ShadowLightMappingArray[MAX_POINT_LIGHT_PROBE_COUNT];

    // NOTE:(yanghang): Cluster block x|y size. point light count in calculate.
    float4 U_ClusterLightParam;
    // NOTE:(yanghang): This macro define by outside.  
    // #define MAX_CLUSTER_POINT_LIGHT_COUNT 8|16|32
    // IMPORTANT: U_ClusterLightWorldPosArray[i].w = inner_radius * inner_radius;
    // U_ClusterLightDiffuseColorArray[i].w = outer_radius * outer_radius;
    float4 U_ClusterLightWorldPosArray[MAX_CLUSTER_POINT_LIGHT_COUNT];
    float4 U_ClusterLightDiffuseColorArray[MAX_CLUSTER_POINT_LIGHT_COUNT]; //linear
    //NOTE: x,y,z for direction, w for cos(outer angle). 
    //spoint light has direction and outer angel range.
    // point light use (0,0,1)direction and PI outer angle.
    float4 U_ClusterLightDirectionOuterAngleArray[MAX_CLUSTER_POINT_LIGHT_COUNT];
    //NOTE: x for inner angle. y for inverse delta cos of outer inner angle.
    // z for inverse delta square outer inner radius.
    float4 U_ClusterLightInnerAngleArray[MAX_CLUSTER_POINT_LIGHT_COUNT];
};

// NOTE:(yanghang):Optimize! Use inverse value to remove divide OP.
#define ClusterBlockSizeX_INV U_ClusterLightParam.x
#define ClusterBlockSizeY_INV U_ClusterLightParam.y
