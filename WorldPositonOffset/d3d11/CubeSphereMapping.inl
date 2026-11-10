//#ifdef PROCEDURAL_SPHERE
static const float3 faceForward[6] =
{
    float3(1.0, 0.0, 0.0),
    float3(-1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, -1.0, 0.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 0.0, -1.0),
};

static const float3 faceRight[6] =
{
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, 1.0),
    float3(1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(-1.0, 0.0, 0.0),
};

static const float3 faceUp[6] =
{
    float3(0.0, 1.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 1.0, 0.0),
};

float3 GetCubeVertex(uint face, float2 xy)
{
    return faceForward[face] + xy.x * faceRight[face] + xy.y * faceUp[face];
}

void MapToCube(float3 ov, out uint face, out float2 xy)
{
    float3 aov = abs(ov);

	float t;
    if (aov.x >= aov.y && aov.x >= aov.z)
    {
        face = ov.x > 0.0 ? 0 : 1;
		t = 1.0 / aov.x;
    }
    else if (aov.y >= aov.z && aov.y >= aov.x)
    {
        face = ov.y > 0.0 ? 2 : 3;
		t = 1.0 / aov.y;
    }
    else
    {
        face = ov.z > 0.0 ? 4 : 5;
		t = 1.0 / aov.z;
    }

    float3 p = ov * t;
    xy = float2(dot(p, faceRight[face]), dot(p, faceUp[face]));
}
//#endif

#ifdef PROCEDURAL_TORUS
float3 MapToTorus(float r, float toroidal, float poloidal)
{
	float cp, sp, ct, st;
	sincos(poloidal, sp, cp);
	sincos(toroidal, st, ct);
	float x = (1.0f + r * cp) * ct;
	float y = r * sp;
	float z = -(1.0f + r * cp) * st;
	return float3(x, y, z);
}

float2 MapToPlane(float3 pos, float r)
{
	float x = atan2(pos.z, pos.x);
	float y = asin(pos.y / r);
	return float2(x, y) / (2.0 * 3.1415926535897932384626433832795);
}
#endif
