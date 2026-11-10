// Light_Function_PS.inl


//#define CLUSTER_DEBUG
#define ALL_LIGHT_CALCULATE_ENABLE 1
#define FRAGMENT_TO_LIGHT_DISTANCE_TEST 1
#define CORE_LIGHT_CALCULATE_ENABLE 1

#define CLUSTER_LIGHT_MISS_DEBUG 0
#define CLUSTER_LIGHT_HIT_DEBUG 0
#define CLUSTER_LIGHT_COVER_DEBUG 0

#define PI 3.14159265358979f

#if CLUSTER_LIGHT_MISS_DEBUG
#define CLUSTER_LIGHT_MISS_DEBUG_COLOR float3(0.8125, 0, 0)
#else
#define CLUSTER_LIGHT_MISS_DEBUG_COLOR float3(0, 0, 0)
#endif

#if CLUSTER_LIGHT_HIT_DEBUG
#define CLUSTER_LIGHT_HIT_DEBUG_COLOR float3(0, 0.8125, 0)
#else
#define CLUSTER_LIGHT_HIT_DEBUG_COLOR float3(0, 0, 0)
#endif

#if CLUSTER_LIGHT_COVER_DEBUG
#define CLUSTER_LIGHT_COVER_DEBUG_COLOR float3(0, 0, 0.852125)
#else
#define CLUSTER_LIGHT_COVER_DEBUG_COLOR float3(0, 0, 0)
#endif


float KajiyaKaySpec(float3 t, half3 h, float specPower)
{
	float tdh = dot(t, h);
	float dirAtten = smoothstep(-1.0, 0.0, tdh);
	
	float aa = max(1.0h - tdh * tdh, 0.0001);
    return dirAtten * pow(aa, specPower * 0.5);
}

float saturate(float value)
{
    return clamp(value, 0.0, 1.0);
}

float GetLuminance(float3 color)
{
    const float3 lumWeight = float3(0.2125f, 0.7154f, 0.0721f);
    return dot(color, lumWeight);
}

// IMPORTANT: Physical base render (PBR) light calculate.
float3
calculateLightPower(float3 N, float3 V, float3 L, float3 lightPower, float3 albedo, float roughness, float metallic)
{
	float3 H    = normalize(L+V);
	float NdotH = max(dot(N, H), 0.0);
	float LdotH = max(dot(L,H),0.0);
	float NdotL = max(dot(N, L), 0.0);
    
    float a2    = roughness*roughness;
    float a4    = a2*a2;
        
    float NdotH2 = NdotH*NdotH;
    float LdotH2 = LdotH*LdotH;
    	  
    float denom = 4.0*3.1415926*pow(NdotH2*(a4-1.0)+1.0,2.0)*(LdotH2*(1.0-a2)+a2);
    denom = max(denom, 0.0001);//note by hejiwei: clamp的数值与安卓平台对齐
    float brdf  = a4/denom;
    	
    float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, albedo, metallic);
	
    float3 kd    = 1.0 - min(F0/(LdotH + 0.00001), 1.0);
    kd = kd * (1.0- float(metallic));
    	
    return (albedo*kd / 3.141592654 + brdf * F0) * NdotL * lightPower;	
}

float3
calculateLightPowerWithAmbientBias(float3 N, float3 V, float3 L, float3 lightPower, float3 albedo, float roughness, float metallic, float ambientBias)
{
    
	float3 H    = normalize(L+V);
	float NdotH = max(dot(N, H), 0.0);
	float LdotH = max(dot(L,H),0.0);
	float NdotL = max(dot(N, L), 0.0);
	NdotL = clamp(NdotL, ambientBias, float(1.0));
	
    float a2    = roughness*roughness;
	float a4    = a2*a2;
    
	float NdotH2 = NdotH*NdotH;
	float LdotH2 = LdotH*LdotH;
	  
    float denom = 4.0*3.1415926*pow(NdotH2*(a4-1.0)+1.0,2.0)*(LdotH2*(1.0-a2)+a2);
	denom = max(denom, 0.0001);//note by hejiwei: clamp的数值与安卓平台对齐
    float brdf  = a4/denom;
    float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, albedo, metallic);
	
    float3 kd    = 1.0 - min(F0/(LdotH + 0.00001), 1.0);
	kd = kd * (1.0-metallic);
	
	return (albedo*kd / 3.141592654 + brdf * F0) * NdotL * lightPower;	
}

float D_GGX(float NoH, float a) {
        float a2 = a * a;
        float f = (NoH * a2 - NoH) * NoH + 1.0;
        return a2 / (3.1415926 * f * f);
}

float3 F_Schlick(float u, float3 f0) {
        return f0 + (float3(1.0, 1.0, 1.0) - f0) * pow(1.0 - u, 5.0);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a) {
        float a2 = a * a;
        float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
        float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
        return 0.5f / max(GGXV + GGXL, 1e-5f);
}

float4
calculateLightPowerWithSpecularPower(float3 N, float3 V, float3 L, float3 lightPower, float3 albedo, float roughness, float metallic, float specularPower)
{
    float3 H    = normalize(L+V);
	float NdotH = max(dot(N, H), 0.0);
	NdotH = abs(dot(N, H)) + 1e-5;
	float LdotH = clamp(dot(L,H),0.0, 1.0);
	float NdotL = clamp(dot(N, L), 0.0, 1.0);
	roughness = max(roughness, 0.0001);
    float a2    = roughness*roughness;
	float a4    = a2*a2;
    
	float NdotH2 = NdotH*NdotH;
	float LdotH2 = LdotH*LdotH;
	  
    float denom = 4.0*3.1415926*pow(NdotH2*(a4-1.0)+1.0,2.0)*(LdotH2*(1.0-a2)+a2);
	denom = max(denom, 0.0001);//note by hejiwei: clamp的数值与安卓平台对齐
    float brdf  = a4/denom;
	
    float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, albedo, metallic);

    float3 kd    = 1.0 - min(F0/(LdotH + 0.00001), 1.0);
	kd = kd * (1.0-metallic);
	
	float4 result;
	result.xyz	=  (kd * albedo / 3.141592654 + brdf * F0) * NdotL * lightPower;
    float D = D_GGX(NdotH, a2);
    float3  F = F_Schlick(LdotH, F0);
    float VV = V_SmithGGXCorrelated(NdotH, NdotL, a2);
    float3 Fr =  D * F * VV;
    result.xyz = Fr * NdotL;
	result.w = brdf * F0.r * NdotL * specularPower;
	return result;		
}


#ifdef CLUSTER_POINT_LIGHT

// NOTE(yanghang):Global cluster mask texture. one texel for each pixel of one region.
Texture2D<uint> S_ClusterLightMaskTex;

// IMPORTANT: Point light func.
float3
calculateClusterPBRPointLight(float2 fragScreenPos,
                              float3 fragWorldPos,
                              float3 N,
                              float3 V,
                              float3 albedo,
                              float roughness,
                              float metallic)
{
    float3 color = float3(0,0,0);
    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * ClusterBlockSizeX_INV);
    block_location.y = int(fragScreenPos.y * ClusterBlockSizeY_INV);
    block_location.z = 0;
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);
    
    #ifdef CLUSTER_DEBUG
    int light_miss_count = 0;
    int light_hit_count = 0;
    #endif

    //cluster_mask = 0xFFFF0000U;
    int validLightBitCount = countbits(cluster_mask);
    
    #if ALL_LIGHT_CALCULATE_ENABLE
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;
            
        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
        
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;

        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
           deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;
            #endif
        #endif
            
            #if CORE_LIGHT_CALCULATE_ENABLE
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate((frag_light_square_d - square_inner_radius) * invDeltaSquareOuterInnerRadius);

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
            
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;


            color += calculateLightPower(N, V,
                                         pointLightDir_h,
                                         pointLightColor,
                                         albedo,
                                         roughness,
                                         metallic);
            
            #else
            color += U_ClusterLightDiffuseColorArray[i].rgb;
            #endif
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;
            #endif            
        }
        #endif
        
        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
        
    }// end for
    #endif
    
    #ifdef CLUSTER_DEBUG
    color += CLUSTER_LIGHT_MISS_DEBUG_COLOR * float(light_miss_count);
    color += CLUSTER_LIGHT_HIT_DEBUG_COLOR * float(light_hit_count);
    color += CLUSTER_LIGHT_COVER_DEBUG_COLOR * float(validLightBitCount);
    #endif

    return color;
}

float3
calculateClusterPBRPointLightWithAmbientBias(float2 fragScreenPos,
                                             float3 fragWorldPos,
                                             float3 N,
                                             float3 V,
                                             float3 albedo,
                                             float roughness,
                                             float metallic,
                                             float ambienBias)
{
    float3 color = float3(0,0,0);
    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * float(ClusterBlockSizeX_INV));
    block_location.y = int(fragScreenPos.y * float(ClusterBlockSizeY_INV));
    block_location.z = 0;
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);
            
    #ifdef CLUSTER_DEBUG
    int light_miss_count = 0;
    int light_hit_count = 0;    
    #endif

    int validLightBitCount = countbits(cluster_mask);
    
    #if ALL_LIGHT_CALCULATE_ENABLE    
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;
            
        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
                
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);
        
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
           deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;
            #endif
        #endif

            #if CORE_LIGHT_CALCULATE_ENABLE            
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate((float(frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius)));

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
                    
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;
        
            color += calculateLightPowerWithAmbientBias(N, V,
                                                        pointLightDir_h,
                                                        pointLightColor,
                                                        albedo,
                                                        roughness,
                                                        metallic,
                                                        ambienBias);
            #else
            color.r += 0.00005 * float(i);
            #endif            
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;
            #endif            
        }
        #endif

        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
    }// end for
    #endif
    
    #ifdef CLUSTER_DEBUG
    color += CLUSTER_LIGHT_MISS_DEBUG_COLOR * float(light_miss_count);
    color += CLUSTER_LIGHT_HIT_DEBUG_COLOR * float(light_hit_count);
    color += CLUSTER_LIGHT_COVER_DEBUG_COLOR * float(validLightBitCount);
    #endif

    return color;
}

struct PointLightAndSpecularOutput
{
    float3 color;
    float3 specular;
};

PointLightAndSpecularOutput
calculateClusterPointLightAndSpecular(float2 fragScreenPos,
                                      float3 fragWorldPos,
                                      float3 normal,
                                      float glossiness,
                                      float3 viewDir)
{
    PointLightAndSpecularOutput result;
    result.color = float3(0,0,0);
    result.specular = float3(0,0,0);

    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * (ClusterBlockSizeX_INV));
    block_location.y = int(fragScreenPos.y * (ClusterBlockSizeY_INV));
    block_location.z = 0;
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);
        
    #ifdef CLUSTER_DEBUG
    int light_miss_count = 0;
    int light_hit_count = 0;
    #endif

    int validLightBitCount = countbits(cluster_mask);

    #if ALL_LIGHT_CALCULATE_ENABLE
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;
        
        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
                
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;
        
        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);
        
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
           deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;     
            #endif
        #endif

            #if CORE_LIGHT_CALCULATE_ENABLE            
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate(((float(frag_light_square_d) - square_inner_radius) / (square_outer_radius - square_inner_radius)));

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
                    
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;
            float3 pointLightDir_h = normalize(float3(pointLightDir));

            float pfNDotL = saturate(dot(normal, pointLightDir_h));
            result.color += pfNDotL * pointLightColor;
                    
            float pfNdotH = saturate(dot(normalize(viewDir + pointLightDir_h), normal));
            result.specular += pow(pfNdotH, glossiness) * pointLightColor;
            #else
            result.color += U_ClusterLightDiffuseColorArray[i].rgb;
            #endif

        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;     
            #endif
        }
        #endif
        
        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
    }// end for
    #endif
    
    #ifdef CLUSTER_DEBUG
    result.color += CLUSTER_LIGHT_MISS_DEBUG_COLOR * float(light_miss_count);
    result.color += CLUSTER_LIGHT_HIT_DEBUG_COLOR * float(light_hit_count);
    result.color += CLUSTER_LIGHT_COVER_DEBUG_COLOR * float(validLightBitCount);
    #endif
    
    return (result);
}




PointLightAndSpecularOutput
calculateClusterPointLightAndSpecularByHair(float2 fragScreenPos,
                                      float3 fragWorldPos,
                                      float3 normal,
                                      float3 tangent,
                                      float3 tangentSec,
                                      half3 specColor,
                                      half3 specColorSec,
                                      float glossiness,
                                      float3 viewDir)
{
    PointLightAndSpecularOutput result;
    result.color = float3(0,0,0);
    result.specular = float3(0,0,0);

    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * (ClusterBlockSizeX_INV));
    block_location.y = int(fragScreenPos.y * (ClusterBlockSizeY_INV));
    block_location.z = 0;
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);
        
    #ifdef CLUSTER_DEBUG
    int light_miss_count = 0;
    int light_hit_count = 0;
    #endif

    int validLightBitCount = countbits(cluster_mask);

    #if ALL_LIGHT_CALCULATE_ENABLE
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;
        
        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
                
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;
        
        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);
        
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
           deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;     
            #endif
        #endif

            #if CORE_LIGHT_CALCULATE_ENABLE            
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate(((float(frag_light_square_d) - square_inner_radius) / (square_outer_radius - square_inner_radius)));

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
                    
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;
            float3 pointLightDir_h = normalize(float3(pointLightDir));
            float NdotL = max(dot(normal, pointLightDir_h), 1e-6);
            float NdotV = dot(normalize(normal), normalize(viewDir));
            pointLightColor *= NdotL;

            // Hair Specular Lighting
            float3 halfVec = normalize(pointLightDir_h + viewDir);   
            float hairSpec = KajiyaKaySpec(tangent, halfVec, glossiness);
            hairSpec += KajiyaKaySpec(tangentSec, halfVec, glossiness);
            
            result.specular += hairSpec * pointLightColor * NdotV * NdotL;
            #else
            result.color += U_ClusterLightDiffuseColorArray[i].rgb;
            #endif

        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;     
            #endif
        }
        #endif
        
        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
    }// end for
    #endif
    
    #ifdef CLUSTER_DEBUG
    result.color += CLUSTER_LIGHT_MISS_DEBUG_COLOR * float(light_miss_count);
    result.color += CLUSTER_LIGHT_HIT_DEBUG_COLOR * float(light_hit_count);
    result.color += CLUSTER_LIGHT_COVER_DEBUG_COLOR * float(validLightBitCount);
    #endif
    
    return (result);
}

struct PointLightAndSpecularFactorOutput
{
    float3 color;
    float specularFactor;
};

PointLightAndSpecularFactorOutput
calculateClusterPointLightAndSpecularFator(float2 fragScreenPos,
                                           float3 fragWorldPos,
                                           float3 normal,
                                           float glossiness,
                                           float3 viewDir)
{
    PointLightAndSpecularFactorOutput result;
    result.color = float3(0,0,0);
    result.specularFactor = 0.0;

    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * float(ClusterBlockSizeX_INV));
    block_location.y = int(fragScreenPos.y * float(ClusterBlockSizeY_INV));
    block_location.z = 0;
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);
        
    #ifdef CLUSTER_DEBUG
    int light_miss_count = 0;
    int light_hit_count = 0;
    #endif

    int validLightBitCount = countbits(cluster_mask);
    
    #if ALL_LIGHT_CALCULATE_ENABLE    
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;
        
        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
                
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;

        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);

        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
           deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;
            #endif
        #endif

            #if CORE_LIGHT_CALCULATE_ENABLE            
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate((frag_light_square_d - square_inner_radius) * invDeltaSquareOuterInnerRadius);

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
                    
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;

            float pfNDotL = saturate(dot(normal, pointLightDir_h));
            result.color += pfNDotL * pointLightColor;
                    
            float pfNdotH = saturate(dot(normalize(viewDir + pointLightDir_h), normal));
            result.specularFactor += pow(pfNdotH, glossiness);
            #else
            result.color.r += 0.00005 * float(i);
            #endif            
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;
            #endif
        }
        #endif

        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
    }// end for
    #endif
    
    #ifdef CLUSTER_DEBUG
    result.color += CLUSTER_LIGHT_MISS_DEBUG_COLOR * float(light_miss_count);
    result.color += CLUSTER_LIGHT_HIT_DEBUG_COLOR * float(light_hit_count);
    result.color += CLUSTER_LIGHT_COVER_DEBUG_COLOR * float(validLightBitCount);
    #endif
    return (result);
}


float3
calculateClusterPointLightForVolumetricClound(float2 fragScreenPos,
                                              float3 fragWorldPos)
{
    float3 color = float3(0,0,0);

    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * float(ClusterBlockSizeX_INV));
    block_location.y = int(fragScreenPos.y * float(ClusterBlockSizeY_INV));
    block_location.z = 0;
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);

    #ifdef CLUSTER_DEBUG
    int light_miss_count = 0;
    int light_hit_count = 0;
    #endif

    int validLightBitCount = countbits(cluster_mask);
    
    #if ALL_LIGHT_CALCULATE_ENABLE
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;

        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
                
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;

        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);
        
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
           deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;
            #endif
        #endif

            #if CORE_LIGHT_CALCULATE_ENABLE            
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate((frag_light_square_d - square_inner_radius) * invDeltaSquareOuterInnerRadius);

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
                    
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;

            color += pointLightColor;
            #else
            color.r += 0.00005 * float(i);
            #endif
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;
            #endif
        }
        #endif

        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
    }// end for
    #endif

    #ifdef CLUSTER_DEBUG
    color += CLUSTER_LIGHT_MISS_DEBUG_COLOR * float(light_miss_count);
    color += CLUSTER_LIGHT_HIT_DEBUG_COLOR * float(light_hit_count);
    color += CLUSTER_LIGHT_COVER_DEBUG_COLOR * float(validLightBitCount);
    #endif
    
    return (color);
}

float3
calculateClusterPointLight(float2 fragScreenPos,
                           float3 fragWorldPos,
                           float3 normal)
{
    float3 color = float3(0,0,0);

    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * float(ClusterBlockSizeX_INV));
    block_location.y = int(fragScreenPos.y * float(ClusterBlockSizeY_INV));
    block_location.z = 0;
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);

    #ifdef CLUSTER_DEBUG
    int light_miss_count = 0;
    int light_hit_count = 0;
    #endif

    int validLightBitCount = countbits(cluster_mask);
    
    #if ALL_LIGHT_CALCULATE_ENABLE
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;

        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
                
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;

        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);

        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
           deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;
            #endif
        #endif

            #if CORE_LIGHT_CALCULATE_ENABLE            
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate((frag_light_square_d - square_inner_radius) * invDeltaSquareOuterInnerRadius);

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
                    
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;

            float pfNDotL = saturate(dot(normal, pointLightDir_h));
            color += pfNDotL * pointLightColor;
            #else
            color.r += 0.00005 * float(i);
            #endif
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;
            #endif
        }
        #endif

        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
    }// end for
    #endif

    #ifdef CLUSTER_DEBUG
    color += CLUSTER_LIGHT_MISS_DEBUG_COLOR * float(light_miss_count);
    color += CLUSTER_LIGHT_HIT_DEBUG_COLOR * float(light_hit_count);
    color += CLUSTER_LIGHT_COVER_DEBUG_COLOR * float(validLightBitCount);
    #endif
    
    return (color);
}

struct LightAndShadowOffsetOutput
{
    float3 color;
    float shadowOffset;
};



LightAndShadowOffsetOutput
calculateClusterPBRPointLightAndShadowOffset(float2 fragScreenPos,
                                             float3 fragWorldPos,
                                             float3 N,
                                             float3 V,
                                             float3 albedo,
                                             float roughness,
                                             float metallic)
{
    LightAndShadowOffsetOutput result;
    result.color = float3(0,0,0);
    result.shadowOffset = 0.0;
        
    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * float(ClusterBlockSizeX_INV));
    block_location.y = int(fragScreenPos.y * float(ClusterBlockSizeY_INV));
    block_location.z = 0;
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);

    #ifdef CLUSTER_DEBUG
    int light_miss_count = 0;
    int light_hit_count = 0;
    #endif

    int validLightBitCount = countbits(cluster_mask);
    
    #if ALL_LIGHT_CALCULATE_ENABLE    
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;

        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
                
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;

        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);
        
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
           deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;
            #endif
        #endif

            #if CORE_LIGHT_CALCULATE_ENABLE            
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate((frag_light_square_d - square_inner_radius) * invDeltaSquareOuterInnerRadius);

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
                    
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;

            result.color += calculateLightPower(N, V, pointLightDir_h, pointLightColor, albedo, roughness, metallic);
            result.shadowOffset += GetLuminance(pointLightColor);
            #else
            result.color.r += 0.00005 * float(i);
            #endif
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;
            #endif
        }
        #endif
                
        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
    }// end for
    #endif
    
    #ifdef CLUSTER_DEBUG
    result.color += CLUSTER_LIGHT_MISS_DEBUG_COLOR * float(light_miss_count);
    result.color += CLUSTER_LIGHT_HIT_DEBUG_COLOR * float(light_hit_count);
    result.color += CLUSTER_LIGHT_COVER_DEBUG_COLOR * float(validLightBitCount);
    #endif
    return (result);
}


// For Glass material
struct PBRLightWithSpecularOutput
{
    float3 color;
    float specular;
};

PBRLightWithSpecularOutput
calculateClusterPBRPointLightAndSpecular(float2 fragScreenPos,
                                         float3 fragWorldPos,
                                         float3 N,
                                         float3 V,
                                         float3 albedo,
                                         float roughness,
                                         float metallic)
{
    PBRLightWithSpecularOutput result;
    result.color = float3(0,0,0);
    result.specular = 0.0;
    
    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * float(ClusterBlockSizeX_INV));
    block_location.y = int(fragScreenPos.y * float(ClusterBlockSizeY_INV));
    block_location.z = 0;
    
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);
    
    #ifdef CLUSTER_DEBUG
    int light_miss_count = 0; 
    int light_hit_count = 0;
    #endif

    int validLightBitCount = countbits(cluster_mask);
    
    #if ALL_LIGHT_CALCULATE_ENABLE    
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;
        
        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
                
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;

        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
           deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;
            #endif            
        #endif

            #if CORE_LIGHT_CALCULATE_ENABLE            
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate((frag_light_square_d - square_inner_radius) * invDeltaSquareOuterInnerRadius);

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
                    
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;
            float3 pointLightDir_h = normalize(float3(pointLightDir));

            float4 lightPower = calculateLightPowerWithSpecularPower(N, V, pointLightDir_h, pointLightColor, albedo, roughness, metallic, lightattenua);
            result.color += lightPower.rgb;
            result.specular += lightPower.w;  
            #else
            result.color.r += 0.00005 * float(i);
            #endif            
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;
            #endif            
        }        
        #endif

        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
    }// end for
    #endif
    
    #ifdef CLUSTER_DEBUG
    result.color += CLUSTER_LIGHT_MISS_DEBUG_COLOR * float(light_miss_count);
    result.color += CLUSTER_LIGHT_HIT_DEBUG_COLOR * float(light_hit_count);
    result.color += CLUSTER_LIGHT_COVER_DEBUG_COLOR * float(validLightBitCount);
    #endif

    return (result);
}


float3 calculateClusterPointLightForLightMap(float2 fragScreenPos, float3 fragWorldPos, half3x3 tbn, float4 lightMap1, float4 lightMap2, float pointLightFactor)
{
    float3 outColor = float3(0.0f, 0.0f, 0.0f);

    // NOTE(yanghang): Check fragment in which block region.
    int3 block_location;
    block_location.x = int(fragScreenPos.x * float(ClusterBlockSizeX_INV));
    block_location.y = int(fragScreenPos.y * float(ClusterBlockSizeY_INV));
    block_location.z = 0;
    uint cluster_mask = S_ClusterLightMaskTex.Load(block_location);

    #ifdef CLUSTER_DEBUG
        int light_miss_count = 0;
        int light_hit_count = 0;
    #endif

    int validLightBitCount = countbits(cluster_mask);
    
    #if ALL_LIGHT_CALCULATE_ENABLE
    int i = 0;
    int zeroBitCount;
    // NOTE(yanghang): It's may be 8,16,32
    for(int scan_index = 0;
        scan_index < validLightBitCount;
        scan_index++)
    {
        zeroBitCount = firstbitlow(cluster_mask);
        i += zeroBitCount;

        float3 pointLightDir = U_ClusterLightWorldPosArray[i].xyz - fragWorldPos;
        float frag_light_square_d =
            pointLightDir.x * pointLightDir.x +
            pointLightDir.y * pointLightDir.y +
            pointLightDir.z * pointLightDir.z;
                
        // NOTE:(yanghang): Optimize.
        float square_outer_radius = U_ClusterLightDiffuseColorArray[i].w;

        //NOTE: cos(0 ~ PI) from [-1,1]
        float3 spotLightDir = U_ClusterLightDirectionOuterAngleArray[i].xyz;
        float cosOuterSpotLightAngle = U_ClusterLightDirectionOuterAngleArray[i].w;

        float3 pointLightDir_h = normalize(float3(pointLightDir));
        float deltaAngleCos = dot(spotLightDir, -pointLightDir_h);

        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        if(frag_light_square_d < square_outer_radius &&
        deltaAngleCos > cosOuterSpotLightAngle)
        {
            #ifdef CLUSTER_DEBUG
            light_hit_count++;
            #endif
        #endif

            #if CORE_LIGHT_CALCULATE_ENABLE            
            float square_inner_radius = U_ClusterLightWorldPosArray[i].w;
            //NOTE(yanghang): Optimize, float fAtten = 1.0 - clamp((frag_light_square_d - square_inner_radius) / (square_outer_radius - square_inner_radius), 0.0, 1.0);
            float invDeltaSquareOuterInnerRadius = U_ClusterLightInnerAngleArray[i].z;
            float fAtten = 1.0 - saturate((frag_light_square_d - square_inner_radius) * invDeltaSquareOuterInnerRadius);

            float cosInnerSpotLightAngle = U_ClusterLightInnerAngleArray[i].x;
            float invDeltaCosInnerOuterAngle = U_ClusterLightInnerAngleArray[i].y;
            fAtten *= 1.0 - saturate((deltaAngleCos - cosInnerSpotLightAngle) * invDeltaCosInnerOuterAngle);
            
            float lightattenua = fAtten * fAtten;
                    
            float3 pointLightColor = float3(U_ClusterLightDiffuseColorArray[i].xyz) * lightattenua;

            float3 pointLightDir_Local = normalize(mul(tbn, -pointLightDir_h));

            //判断并取Left/Right方向灯光
            float hMap = (pointLightDir_Local.x > 0.0f) ? (lightMap2.x) : (lightMap1.x);
            //判断并取Top/Bottom方向灯光
            float vMap = (pointLightDir_Local.y > 0.0f) ? (lightMap2.y) : (lightMap1.y);
            //判断并取Front/Back方向灯光
            float dMap = (pointLightDir_Local.z > 0.0f) ? (lightMap1.z) : (lightMap2.z);

            float  weight = hMap * abs(pointLightDir_Local.x) + vMap * abs(pointLightDir_Local.y) + dMap * abs(pointLightDir_Local.z);
            weight = weight * pointLightFactor;
            float3  resultLightColor = pointLightColor.rgb * weight;
            outColor += resultLightColor;
            #else
                outColor += 0.00005 * float(i);
            #endif
            
        #if FRAGMENT_TO_LIGHT_DISTANCE_TEST
        }
        else
        {
            #ifdef CLUSTER_DEBUG
            light_miss_count++;
            #endif
        }
        #endif

        cluster_mask >>= (zeroBitCount + 1);
        i += 1;
    }// end for
    #endif
    return outColor;
}

#endif
