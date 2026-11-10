// ray sphere intersection test
// return float2 t
// t.x for near intersection param
// t.y for far intersection param
float2 RayIntersectSphere(float3 rayOrigin, float3 rayDir, float4 sphere)
{
    float3 temp = rayOrigin - sphere.xyz;
    float b = dot(rayDir, temp);
    float c = dot(temp, temp) - sphere.w * sphere.w;

    float disc = b * b - c;

    // both t.x and t.y are negative means no intersection
    if(disc < 0.0) return float2(-1.f, -1.f);
    
    float disc_sqrt = sqrt(disc);
	
    float t0 = -b - disc_sqrt;
    float t1 = -b + disc_sqrt;
    
    return float2(t0, t1);
}

float2 RayIntersectSphere1(float3 rayOrigin, float3 rayDir, float4 sphere)
{
    float3 temp = rayOrigin - sphere.xyz;
    float b = dot(rayDir, temp);
    float c = dot(temp, temp) - sphere.w * sphere.w;

    float disc = b * b - c;

    // both t.x and t.y are negative means no intersection
    if(disc < 0.0) return float2(1e38f, 1e38f);

    float disc_sqrt = sqrt(disc);

    float t0 = -b - disc_sqrt;
    float t1 = -b + disc_sqrt;

    return float2(t0, t1);
}

bool RayHitSphere(float2 t)
{
    return t.x > 0.f || t.y > 0.f;
}

bool RayHitSphere1(float2 t)
{
    return t.y > t.x && t.y > 0.f;
}

bool CameraOutsideSphere(float2 t)
{
    // both t.x and t.y < 0.f for ray misses sphere
    // or both t.x and t.y > 0.f for ray hits sphere
    return t.x * t.y > 0.f; 
}

bool CameraInsideSphere(float2 t)
{
    return t.x < 0.f && t.y > 0.f;
}

bool PointInsideAtmosphere(in float3 p, float4 sphere)
{
    float distance = length(p - sphere.xyz);
    return distance < sphere.w;
}

bool AtmosphereGetBlocked(in float tObject, in float tAtmosphereClose)
{
    return tObject < tAtmosphereClose;
}

//rayDir must be normalized vector
float DistanceRayToPoint(in float3 rayOrigin, in float3 rayDir, in float3 p)
{
    float3 originToP = p - rayOrigin;
    float VDotCosTheta = abs(dot(originToP, rayDir));  
    return sqrt(dot(originToP, originToP) - VDotCosTheta * VDotCosTheta);
}