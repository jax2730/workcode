Texture2D<float> S_DepthLoZ;

// Transforms origin to uv space
// Mat must be able to transform origin from its current space into clip space.
float3 ProjectPosition(float3 origin, float4 mat[4])
{
	float4 projected;
	projected.x = dot(float4(origin, 1), mat[0]);
	projected.y = dot(float4(origin, 1), mat[1]);
	projected.z = dot(float4(origin, 1), mat[2]);
	projected.w = dot(float4(origin, 1), mat[3]);
	projected.xyz /= projected.w;
	projected.xy = 0.5 * projected.xy + 0.5;
	projected.y = (1 - projected.y);
	return projected.xyz;
}

// Origin and direction must be in the same space and mat must be able to transform from that space into clip space.
float3 ProjectDirection(float3 origin, float3 direction, float3 screen_space_origin, float4 mat[4])
{
	float3 offsetted = ProjectPosition(origin + direction, mat);
	return offsetted - screen_space_origin;
}

// Mat must be able to transform origin from texture space to a linear space.
float3 InvProjectPosition(float3 coord, float4 mat[4])
{
	coord.y = (1 - coord.y);
	coord.xy = 2 * coord.xy - 1;
	float4 projected;
	projected.x = dot(float4(coord, 1), mat[0]);
	projected.y = dot(float4(coord, 1), mat[1]);
	projected.z = dot(float4(coord, 1), mat[2]);
	projected.w = dot(float4(coord, 1), mat[3]);
	projected.xyz /= projected.w;
	return projected.xyz;
}

float FFX_SSSR_LoadDepth(int2 pixel_coordinate, int mip) {
    return S_DepthLoZ.Load(int3(pixel_coordinate, mip));
}

float3 FFX_SSSR_ScreenSpaceToViewSpace(float3 screen_space_position) {
	return InvProjectPosition(screen_space_position, U_InvProjectMatrix);
}

float3 ScreenSpaceToWorldSpace(float3 screen_space_position)
{
	return InvProjectPosition(screen_space_position, U_InvViewProjectMatrix);
}

float3 MinDiff(const float3 p, const float3 pr, const float3 pl)
{
    float3 v1 = pr - p;
    float3 v2 = p - pl;
    return dot(v1, v1) < dot(v2, v2) ? v1 : v2;
}

float3 FFX_SSSR_LoadWorldSpaceNormal(int2 screen_space_position)
{
	int2 tc = screen_space_position;
	float3 p = ScreenSpaceToWorldSpace(float3((tc + 0.5) * U_DepthRTParam.zw, FFX_SSSR_LoadDepth(tc, 0)));
	
    int2 tcr = screen_space_position + int2(1, 0);
    float3 pr = ScreenSpaceToWorldSpace(float3((tcr + 0.5) * U_DepthRTParam.zw, FFX_SSSR_LoadDepth(tcr, 0)));

    int2 tcl = screen_space_position + int2(-1, 0);
    float3 pl = ScreenSpaceToWorldSpace(float3((tcl + 0.5) * U_DepthRTParam.zw, FFX_SSSR_LoadDepth(tcl, 0)));

    int2 tct = screen_space_position + int2(0, -1);
    float3 pt = ScreenSpaceToWorldSpace(float3((tct + 0.5) * U_DepthRTParam.zw, FFX_SSSR_LoadDepth(tct, 0)));

    int2 tcb = screen_space_position + int2(0, 1);
    float3 pb = ScreenSpaceToWorldSpace(float3((tcb + 0.5) * U_DepthRTParam.zw, FFX_SSSR_LoadDepth(tcb, 0)));
    return normalize(cross(MinDiff(p, pr, pl), MinDiff(p, pt, pb)));
}

float3 FFX_SSSR_HierarchicalRaymarch(
	float3 origin, 
	float3 direction, 
	bool is_mirror, 
	float2 screen_size, 
	int most_detailed_mip,
	int least_detailed_mip,
	uint min_traversal_occupancy, 
	uint max_traversal_intersections, 
	out bool valid_hit);

float2 FFX_SSSR_GetMipResolution(float2 screen_dimensions, int mip_level);

float FFX_SSSR_ValidateHit(
	float3 hit, 
	float2 uv, 
	float3 world_space_ray_direction, 
	float2 screen_size, 
	float depth_buffer_thickness,
	out bool back_face);

float4 CastScreenSpaceRay(
	uint2 screen_size,
	float2 uv,
	float3 world_space_normal,
	float z)
{
	bool is_mirror = true;
	int most_detailed_mip = 0;
	int least_detailed_mip = 6;
	float2 mip_resolution = FFX_SSSR_GetMipResolution(screen_size, most_detailed_mip);
	
	float3 screen_uv_space_ray_origin = float3(uv, z);
	float3 view_space_ray = FFX_SSSR_ScreenSpaceToViewSpace(screen_uv_space_ray_origin);
	float3 view_space_ray_direction = normalize(view_space_ray);
	
	float3 view_space_surface_normal = mul(float4(world_space_normal, 0), U_ViewMatrix).xyz;
	
	float3 view_space_reflected_direction = reflect(view_space_ray_direction, view_space_surface_normal);
	float3 screen_space_ray_direction = ProjectDirection(view_space_ray, view_space_reflected_direction, 
		screen_uv_space_ray_origin, U_ProjectMatrix);
	
	//====SSR====
	bool valid_hit = false;
	const int max_steps = 128;
	float3 hit = FFX_SSSR_HierarchicalRaymarch(screen_uv_space_ray_origin, screen_space_ray_direction, is_mirror, screen_size,
		most_detailed_mip, least_detailed_mip, 0, max_steps, valid_hit);
	
	float3 world_space_origin = ScreenSpaceToWorldSpace(screen_uv_space_ray_origin);
	float3 world_space_hit = ScreenSpaceToWorldSpace(hit);
	float3 world_space_ray = world_space_hit - world_space_origin.xyz;
	
	float depthBufferThickness = max(0.5 * abs(FFX_SSSR_ScreenSpaceToViewSpace(hit).z - view_space_ray.z), 0.1);
	bool back_face;
	float confidence = valid_hit ? FFX_SSSR_ValidateHit(hit, uv, world_space_ray, screen_size, depthBufferThickness,
		back_face) : 0;
	//float world_ray_length = max(0, length(world_space_ray));
	
	float3 reflection_radiance = 0;
	if (confidence > 0 && !back_face)
	{
       // Found an intersection with the depth buffer -> We can lookup the color from lit scene.
		reflection_radiance = S_BackbufferTex.SampleLevel(S_BackbufferTexSamplerState, hit.xy, 0).xyz;
		reflection_radiance.xyz = pow(reflection_radiance.xyz, 2.2);
	}
	return float4(reflection_radiance, confidence);
}
