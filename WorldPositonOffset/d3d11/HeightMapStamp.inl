#ifdef ENABLE_STAMP_TERRAIN

#define instances U_CSCustom15
#define Epsilon 1e-6f
struct StampPair
{
    float TransitionScale;
    float Value;
};

struct StampTerrainInstance
{
// Base_BEGIN
    // float4
    float width;
    float height;
    float verticalScale;
    float transition;
    // float4
    float3 axis;
    float angle;
    // float4 
    int behavior; // 
    float verticalTrans;
// Base_END
    int TextureID;
    int padding;
    // float4x4
    float4x4 transform;
};
Texture2DArray S_2DArrayExt10;
SamplerState S_2DArrayExt10SamplerState;
Texture2DArray S_2DArrayExt11;
SamplerState S_2DArrayExt11SamplerState;
Texture2DArray S_2DArrayExt12;
SamplerState S_2DArrayExt12SamplerState;
Texture2DArray S_2DArrayExt13;
SamplerState S_2DArrayExt13SamplerState;
Texture2DArray S_2DArrayExt14;
SamplerState S_2DArrayExt14SamplerState;

#define SAMPLE_BLOCK(SAMPLER)                                       \
{                                                                   \
    SAMPLER.GetDimensions(width, height, elements);                 \
    float2 uv0 = uv * float2(width, height);                        \
    uv0 -= 0.5f;                                                    \
    float2 texLoc = floor(uv0);                                     \
    delta = uv0 - floor(uv0);                                       \
    v4.w = SAMPLER.Load(toCoord(texLoc, width, height, index));     \
    texLoc.x += 1.0f;                                               \
    v4.z = SAMPLER.Load(toCoord(texLoc, width, height, index));     \
    texLoc.y += 1.0f;                                               \
    v4.y = SAMPLER.Load(toCoord(texLoc, width, height, index));     \
    texLoc.x -= 1.0f;                                               \
    v4.x = SAMPLER.Load(toCoord(texLoc, width, height, index));     \
    break;                                                          \
}


StampTerrainInstance instances[STAMP_TERRAIN_INSTANCES_MAX];
float sigmoid(float value, float center, float width)
{
    return 1.0f / (1.0f + (float)exp(-(value - center) / width));
}
float sigmoidMin(float a, float b, float tolerance)
{
    return a+(b-a)*(1.0f/(1-exp2((b-a)/tolerance)));
}
float sigmoidMax(float a, float b, float tolerance)
{
    return b+(a-b)*(1.0f/(1-exp2((b-a)/tolerance)));
}
int4 toCoord(float2 uv, int width, int height, int index)
{
    int2 originalUV = int2(uv);
    int2 clampUV = clamp(originalUV, 0, int2(width-1, height-1));
    int layer = int(uint(instances[index].TextureID) & 0x0000ffff);
    int4 coord = int4(clampUV.xy,layer, 0);
    return coord;
}
StampPair Evaluate(float3 unitSphere, int index)
{
    StampPair Pair;
    Pair.Value = Pair.TransitionScale = 2.0f;
    float4 transformed = mul(float4(unitSphere, 1.0f), instances[index].transform);
    float3 transformedVector = transformed.xyz; // Impl->transform * unitSphere;
    // transformedVector = normalize(transformedVector);

    //if(transformedVector.y > Epsilon)
    //{
        // float2 uv = float2(transformedVector.x / transformedVector.y, transformedVector.z / transformedVector.y);
        float2 uv = transformedVector.xz;
        // bool ok = uv.x >= -instances[index].width / 2
	    // && uv.x <= +instances[index].width / 2
	    // && uv.y >= -instances[index].height / 2
	    // && uv.y <= +instances[index].height / 2;
        bool ok = transformedVector.y > Epsilon && uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f;
        if(ok)
        {
            // uv += float2(instances[index].width / 2, instances[index].height / 2);
            // uv.x /= instances[index].width, uv.y /= instances[index].height;

            float horizon = (uv.x - (1.0f - instances[index].transition)) / instances[index].transition;
            horizon = max(horizon, (instances[index].transition - uv.x) / instances[index].transition);
            horizon = clamp(horizon, 0.0f, 1.0f);
            float vertical = (uv.y - (1.0f - instances[index].transition)) / instances[index].transition;
            vertical = max(vertical, (instances[index].transition - uv.y) / instances[index].transition);
            vertical = clamp(vertical, 0.0f, 1.0f);
            if (horizon >= Epsilon && vertical >= Epsilon)
            {
            	Pair.TransitionScale = sqrt(horizon * horizon + vertical * vertical);
            }
            else if (horizon < Epsilon)
            {
            	Pair.TransitionScale = vertical;
            }
            else
            {
            	Pair.TransitionScale = horizon;
            }
            float width, height, elements;
            float4 v4;
            float2 delta;
            int arrayID = int(uint(instances[index].TextureID) >> 16);
            switch(arrayID)
            {
                case 0:
                    SAMPLE_BLOCK(S_2DArrayExt10)
                case 1:
                    SAMPLE_BLOCK(S_2DArrayExt11)
                #if 0
                case 2:
                    SAMPLE_BLOCK(S_2DArrayExt12)
                case 3:
                    SAMPLE_BLOCK(S_2DArrayExt13)
                case 4:
                    SAMPLE_BLOCK(S_2DArrayExt14)
                #endif
                default:
                break;
            }
            
            Pair.Value = delta.y * ((1.0f-delta.x)*v4.x+delta.x*v4.y)+(1.0f-delta.y)*((1.0f-delta.x)*v4.w+delta.x*v4.z);
            
            Pair.Value *= instances[index].verticalScale;
            Pair.Value += instances[index].verticalTrans;
            
        }
    //}
    return Pair;
}
float Melt(float originalValue, StampPair additionalValue, int index)
{
    float Value = originalValue;
    if(additionalValue.TransitionScale>=1.0f)
    {
        return Value;
    }

    // if(instances[index].behavior==0)
    // {
    //     Value = sigmoidMin(originalValue, additionalValue.Value, instances[index].tolerance);
    // }
    // else if(instances[index].behavior==1)
    // {
    //     Value = sigmoidMax(originalValue, additionalValue.Value, instances[index].tolerance);
    // }
    // else if(instances[index].behavior==2)
    // {
        Value = additionalValue.Value;
    // }

    if(additionalValue.TransitionScale<=1e-6f)
    {
        return Value;
    }
    float lambda = sigmoid(additionalValue.TransitionScale, 0.5f, 0.07f);
    return lambda * originalValue + (1 - lambda) * Value;
}


float EvaluateVal(float originalValue, float3 unitSphere)
{
    uint numStructs = instances[0].padding;
    float val = originalValue;
    for(int i=0;i<numStructs;++i)
    {
        val = Melt(val, Evaluate(unitSphere, i), i);
    }
    return val;
}
#endif