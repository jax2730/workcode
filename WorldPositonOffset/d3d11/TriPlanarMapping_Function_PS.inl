// note by hejiwei: 地形采样贴图用到的一些函数，不一定是三平面投影相关的

// helper function
//-------------------------------------------------------------------------------------------------------
    float3 UnpackNormal(in float3 sampledNormal, float strength){
        float3 n = 2.f * sampledNormal - 1.f;
        return normalize(float3(n.x * strength, n.y * strength, 1.0f));
    }
    
    float3 UnpackNormal(in float2 sampledNormal_xy, float strength)
    {
        float3 n = 2.f * float3(sampledNormal_xy, 0.5f) - 1.f;
        return normalize(float3(n.x * strength, n.y * strength, 1.0f));
    }
    
	// only remap rgb channel 
    float4 sRGBToLinear(in float4 color)
    {
       color.xyz = pow(color.xyz, 2.2);
       return color;
    }
     
    float3 ComputeTriplanarSampleWeight(in float3 normal, in float delta, in float m)
    {
        float3 weight = abs(normal.xyz);
        weight = pow(max(weight - delta, 1e-6f), m);
        float totalWeight = dot(weight, 1.f);
        return weight / totalWeight;//renormalize weights
    }

    struct MaterialSample{
        uint matIndex[2];
        float customUVTilling[2];
        float customNormalStrength[2];
        float3 customColor[2];
        float weight;
        #ifndef KARST_TERRAIN
            float4 customEmission[2];
            float enableTextureEmissive[2];
            float customMetallic[2];
        #endif
    };

    // select two materials from four adjacent mat id pixel
    // params:
    // samplePoint      : continuous coordinate of sample point
    // aroundCoord      : discret coordinate of pixels around sample point
    // aroundID         : material id values of pixels around sample point
    // uvTilling        : 16 uv tilling params stored in 4 float4
    // normalStrength   : 16 normal strength params stored in 4 float4
#ifdef KARST_TERRAIN
    MaterialSample SelectMaterial(
        in float2 samplePoint, 
        in int4 aroundCoord[4], 
        in uint aroundID[4], 
        in float4 uvTilling[8], 
        in float4 normalStrength[8],
        in float4 materialColor[32])
#else
    MaterialSample SelectMaterial(
        in float2 samplePoint, 
        in int3 aroundCoord[4], 
        in uint aroundID[4], 
        in float4 uvTilling[4], 
        in float4 normalStrength[4],
        in float4 materialColor[16],
        in float4 emission[16],
        in float4 textureEmissive[8])
#endif
    {
        //init
        MaterialSample materialSample;
        materialSample.matIndex[0] = 0;
        materialSample.matIndex[1] = 0;
        materialSample.weight = 1.f;
        
        int2 currentIDMapCoord = int2(floor(samplePoint)); // get discret coordinate of sample point
        uint currentID = 0;

        // loop over four pixels, find the pixel that current sample hits
        [unroll]
        for(int i = 0; i < 4; i++){
            if(aroundCoord[i].x == currentIDMapCoord.x &&
                aroundCoord[i].y == currentIDMapCoord.y)
                currentID = aroundID[i];
        }

        float nearestDistance = 10000.0f;
        float2 nearestCoordVector; // vector pointing from the sample point to the center of nearest pixel
        float2 nearestCoord; 

        uint nearestID = currentID;
        // find a pixel that has a different matID with currentID and has a minimum distance
        [unroll]
        for(int j = 0; j < 4; j++)
        {
            if(aroundID[j] != currentID)
            {
                float2 coordVector = samplePoint - (float2(aroundCoord[j].xy) + 0.5f);
                float coordDis = (coordVector.x * coordVector.x + coordVector.y * coordVector.y);
                if(coordDis < nearestDistance)
                {
                    nearestDistance = coordDis;
                    nearestCoordVector = coordVector;
                    nearestCoord = float2(aroundCoord[j].xy);

                    nearestID = aroundID[j];
                }
            }
        }

        materialSample.matIndex[0] = currentID;
        materialSample.matIndex[1] = nearestID;

        float2 totalCoordVector;
        if(currentID != nearestID)
        {
            totalCoordVector = (nearestCoord) - floor(samplePoint);

            // 最近的两个不同的材质位于对角线上
            if(abs(totalCoordVector.x) == abs(totalCoordVector.y))
            {
                float2 absNearestCoordVector = abs(nearestCoordVector);

                // 取长边混合
                if(absNearestCoordVector.x <= absNearestCoordVector.y)
                    totalCoordVector.x = 0.f;
                else    
                    totalCoordVector.y = 0.f;
            }
            materialSample.weight = saturate(dot((-totalCoordVector), nearestCoordVector));
        }
        // compute custom params
        #ifdef KARST_TERRAIN
            materialSample.customUVTilling[0] = 
                uvTilling[materialSample.matIndex[0] / 8u][materialSample.matIndex[0] % 4u];
            materialSample.customUVTilling[1] = 
                uvTilling[materialSample.matIndex[1] / 8u][materialSample.matIndex[1] % 4u];
    
            materialSample.customNormalStrength[0] =
                normalStrength[materialSample.matIndex[0] / 8u][materialSample.matIndex[0] % 4u];
            materialSample.customNormalStrength[1] = 
                normalStrength[materialSample.matIndex[1] / 8u][materialSample.matIndex[1] % 4u];
        #else
            materialSample.customUVTilling[0] = 
                uvTilling[materialSample.matIndex[0] / 4u][materialSample.matIndex[0] % 4u];
            materialSample.customUVTilling[1] = 
                uvTilling[materialSample.matIndex[1] / 4u][materialSample.matIndex[1] % 4u];

            materialSample.customNormalStrength[0] =
                normalStrength[materialSample.matIndex[0] / 4u][materialSample.matIndex[0] % 4u];
            materialSample.customNormalStrength[1] = 
                normalStrength[materialSample.matIndex[1] / 4u][materialSample.matIndex[1] % 4u];

            materialSample.enableTextureEmissive[0] = 
                textureEmissive[materialSample.matIndex[0] / 4u][materialSample.matIndex[0] % 4u];
            materialSample.enableTextureEmissive[1] =
                textureEmissive[materialSample.matIndex[1] / 4u][materialSample.matIndex[1] % 4u];

            materialSample.customMetallic[0] = 
                textureEmissive[materialSample.matIndex[0] / 4u + 4][materialSample.matIndex[0] % 4u];
            materialSample.customMetallic[1] =
                textureEmissive[materialSample.matIndex[1] / 4u + 4][materialSample.matIndex[1] % 4u];

            materialSample.customEmission[0] = emission[materialSample.matIndex[0]];
            materialSample.customEmission[1] = emission[materialSample.matIndex[1]];
        #endif
        materialSample.customColor[0] = materialColor[materialSample.matIndex[0]].xyz;
        materialSample.customColor[1] = materialColor[materialSample.matIndex[1]].xyz;

        return materialSample;
    }

    struct BilinearSample{
        // value of projectionIndex could be 0, 1, 2 since uvs are calculated via triplanar projection
        // 0,1,2 reffers to projection along x, y, z axis of our world coordinate system
        int projectionIndex[2];          

        // value of tangentSpaceIndex could be 0, 1 since the projections along z and x axises 
        // generate the same tangent space 
        // 0, 1 reffers to projection along z\x axis or projection along y axis
        int tangentSpaceIndex[2];

        float weight; // renormalized weight of uvIndex[0]
    };

    // choose two projections of greater weight
    // then return them with weight decending order
    BilinearSample BilinearProjection(in float3 weight)
    {
        BilinearSample bilinearSample;

        //init
        bilinearSample.projectionIndex[0] = 0;
        bilinearSample.projectionIndex[1] = 0;
        bilinearSample.tangentSpaceIndex[0] = 0;
        bilinearSample.tangentSpaceIndex[1] = 0;
        bilinearSample.weight = 1.0f;

        // x is the miniest
        [branch]
        if(weight.x <= weight.y && weight.x <= weight.z)
        {
            [branch]
            if(weight.y > weight.z)
            {
                bilinearSample.projectionIndex[0] = 1; // prj y
                bilinearSample.projectionIndex[1] = 2; // prj z
                bilinearSample.tangentSpaceIndex[0] = 1;
                bilinearSample.tangentSpaceIndex[1] = 0;
            }
            else
            {
                bilinearSample.projectionIndex[0] = 2;  // prj z
                bilinearSample.projectionIndex[1] = 1;  // prj y
                bilinearSample.tangentSpaceIndex[0] = 0;
                bilinearSample.tangentSpaceIndex[1] = 1;
            } 
        }
        else
        {
            // y is the miniest 
            [branch]
            if(weight.y <= weight.x && weight.y <= weight.z)
            {
                [branch]
                if(weight.x > weight.z)
                {
                    bilinearSample.projectionIndex[0] = 0; // prj x
                    bilinearSample.projectionIndex[1] = 2; // prj z
                    bilinearSample.tangentSpaceIndex[0] = 0;
                    bilinearSample.tangentSpaceIndex[1] = 0;
                }
                else
                {
                    bilinearSample.projectionIndex[0] = 2;  // prj z
                    bilinearSample.projectionIndex[1] = 0;  // prj x
                    bilinearSample.tangentSpaceIndex[0] = 0;
                    bilinearSample.tangentSpaceIndex[1] = 0;
                }
            }

            // z is the miniest
            else{
                if(weight.x > weight.y)
                {
                    bilinearSample.projectionIndex[0] = 0;  // prj x
                    bilinearSample.projectionIndex[1] = 1;  // prj y
                    bilinearSample.tangentSpaceIndex[0] = 0;
                    bilinearSample.tangentSpaceIndex[1] = 1;
                }
                else
                {
                    bilinearSample.projectionIndex[0] = 1;  // prj y
                    bilinearSample.projectionIndex[1] = 0;  // prj x
                    bilinearSample.tangentSpaceIndex[0] = 1;
                    bilinearSample.tangentSpaceIndex[1] = 0;
                }
            }
        }

        //renormalize weight
        bilinearSample.weight = weight[bilinearSample.projectionIndex[0]] / 
            (weight[bilinearSample.projectionIndex[0]] + weight[bilinearSample.projectionIndex[1]] + 1e-5f);

        return bilinearSample;
    }
//-------------------------------------------------------------------------------------------------------

// test functions
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------

// hash & noise related function
//-------------------------------------------------------------------------------------------------------
    float hash(in float2 uv)
    {
        return frac(
            1e4f * sin(17.f * uv.x + 0.1f * uv.y) * (0.1f + abs(sin(13.f * uv.y)))
            );
    }   

    float hash3D(in float3 uv)
    {
        return hash(float2(hash(uv.xy), uv.z ));
    }

    static const float4x4 mIndexNoise =
    {
        0.7215686, 0.1686275, 0.5411765, 0.0,
        0.8156863, 0.282353, 0.454902, 0.1215686,
        0.2196079, 0.2745098, 0.764706, 0.6431373,
        0.8196079, 0.6431373, 0.098039, 0.1098039,
    };

    float indexNoise(float2 seed)
    {
        seed = abs(seed);
        int indexCount = 4;
        float uvstep = 1 / indexCount; // uvstep = 1/indexCount
        seed *= float2(indexCount, indexCount);
        float2 f = frac(seed);
        int ix = floor(seed.x % indexCount);
        int iy = floor(seed.y % indexCount);

        int ixof = step(indexCount, ix + 1) ? 0 : ix + 1; //repeat address
        int iyof = step(indexCount, iy + 1) ? 0 : iy + 1;

        float noise = lerp(lerp(mIndexNoise[ix][iy], mIndexNoise[ixof][iy], f.x), 
                            lerp(mIndexNoise[ix][iyof], mIndexNoise[ixof][iyof], f.x), f.y);

        return noise; 
    }

    //value noise with cubic lerp
    float valueNoise(float2 uv)
    {
        uv *= 0.8f; // could be replaced by any number. we are using 0.8 only because it gets good result while testing.

        float2 i = floor(uv);

        float2 f = frac(uv);

        // float2 u = f * f * f * (10.0 + f * (f * 6.0 - 15.0));
        float2 u = f;
        return lerp(lerp(hash(i + float2(0,0)),
                   hash(i + float2(1,0)),u.x),
                lerp(hash(i + float2(0,1)),
                   hash(i + float2(1,1)),u.x),u.y);
    }

//-------------------------------------------------------------------------------------------------------

// only sample one direction of triplanar mapping
//-------------------------------------------------------------------------------------------------------

    //lazy triplanar sampling: only sample the plane of highest weight
    float2 lazySampleUV(out float2 duvdx, out float2 duvdy, 
        in float2 UVXZ, in float2 UVXY, in float2 UVYZ, in float3 weight)
    {   
        // float lodXZ = computeTextureLevel(UVXZ, textureSize);
        // float lodXY = computeTextureLevel(UVXY, textureSize);
        // float lodYZ = computeTextureLevel(UVYZ, textureSize);
        float2 ddxXZ = ddx(UVXZ);
        float2 ddxXY = ddx(UVXY);
        float2 ddxYZ = ddx(UVYZ);

        float2 ddyXZ = ddy(UVXZ);
        float2 ddyXY = ddy(UVXY);
        float2 ddyYZ = ddy(UVYZ);

        float offset = 1.f / 512.f;
        duvdx = weight.x + offset > weight.y ? 
            weight.x + offset > weight.z ? ddxXZ : ddxYZ : 
            weight.y + offset > weight.z ? ddxXY : ddxYZ;
        duvdy = weight.x + offset > weight.y ?
            weight.x + offset > weight.z ? ddyXZ : ddyYZ :
            weight.y + offset > weight.z ? ddyXY : ddyYZ;
        return weight.x + offset > weight.y ?
            weight.x + offset > weight.z ? UVXZ : UVYZ : 
            weight.y + offset > weight.z ? UVXY : UVYZ;
    }

    float3 lazySampleTangent(in float3 tangent1, in float3 tangent2, in float3 weight)
    {
        return weight.z > weight.y && weight.z > weight.x ? tangent2 : tangent1;
    }

    float3 lazySampleBiTangent(in float3 biTangent1, in float3 biTangent2, in float3 weight)
    {
        return weight.z > weight.y && weight.z > weight.x ? biTangent2 : biTangent1;
    }

    // //hashed sampling: using a random value to sample one direction of mapping
    // float3 hashedSampleUV(in float hashValue, in float2 UVXZ, in float2 UVXY, in float2 UVYZ, in float3 weight, float2 textureSize)
    // {
    //     float lodXZ = floor(computeTextureLevel(UVXZ, textureSize));
    //     float lodXY = floor(computeTextureLevel(UVXY, textureSize));
    //     float lodYZ = floor(computeTextureLevel(UVYZ, textureSize));

    //     return (weight.x > hashValue) ?
    //         float3(UVXZ, lodXZ) : 
    //         ((weight.x + weight.y > hashValue) ? float3(UVXY,lodXY) :
    //             float3(UVYZ, lodYZ));
    // }
//-------------------------------------------------------------------------------------------------------

// texture repetition
//-------------------------------------------------------------------------------------------------------
    #define frequency 0.3
    #define IndexRange 12.0
    // static const float4x4 mIndexNoise =
    // {
    //     0.7215686, 0.1686275, 0.5411765, 0.0,
    //     0.8156863, 0.282353, 0.454902, 0.1215686,
    //     0.2196079, 0.2745098, 0.764706, 0.6431373,
    //     0.8196079, 0.6431373, 0.098039, 0.1098039,
    // };

    // float IndexNoise(float2 uv)
    // {
    //     uv = abs(uv);
    //     int indexCount = 4;
    //     float uvstep = 1 / indexCount;
    //     uv *= float2(indexCount, indexCount);
    //     float2 f = frac(uv);
    //     int ix = floor(uv.x % indexCount);
    //     int iy = floor(uv.y % indexCount);

    //     int ixof = step(indexCount, ix + 1) ? 0 : ix + 1;
    //     int iyof = step(indexCount, iy + 1) ? 0 : iy + 1;

    //     float noise = lerp(
    //                     lerp(mIndexNoise[ix][iy], mIndexNoise[ixof][iy], f.x), 
    //                     lerp(mIndexNoise[ix][iyof], mIndexNoise[ixof][iyof], f.x),
    //                     f.y);
    //     return noise;
    // }
    
    float sum(float3 v){return v.x + v.y + v.z;}

    float2 ComputeUVNoTile(float2 uv)
    {
        // float noise = IndexNoise(uv.xy * frequency);
        float noise = valueNoise(uv.xy);

        float l = noise * IndexRange;
        float f = frac(l);

        float ia = floor(l);
        float ib = ia + 1.0;

        float2 offa = sin(float2(3.0, 7.0) * ia);
        float2 offb = sin(float2(3.0, 7.0) * ib);

        float2 uva = frac(offa + uv.xy);
        float2 uvb = frac(offb + uv.xy);

        float2 duv = uva - uvb;

        return lerp(uva, uvb, smoothstep(0.2, 0.8, f - 0.1 * (duv.x + duv.y)));
    }

    //UV.z 为纹理数组的索引
    float4 SampleNoTile(in Texture2DArray tex, in SamplerState texSamplerState, in float3 UV, float noise)
    {
        //float noise = IndexNoise(UV.xy * frequency);//0.3 is a empirial number
        // float noise = valueNoise(UV.xy);

        float2 dx = ddx(UV.xy);
        float2 dy = ddy(UV.xy);

        float l = noise * IndexRange;
        float f = frac(l);

        float ia = floor(l);
        float ib = ia + 1.0;

        float2 offa = sin(float2(3.0, 7.0) * ia);
        float2 offb = sin(float2(3.0, 7.0) * ib);

        float4 texa = tex.SampleGrad(texSamplerState, float3(UV.xy + offa, UV.z), dx, dy);
        float4 texb = tex.SampleGrad(texSamplerState, float3(UV.xy + offb, UV.z), dx, dy);
    
        float4 result = lerp(texa, texb, smoothstep(0.2, 0.8, f - 0.1 * sum((texa - texb).xyz)));
        return result;
    }

    float4 SampleNoTile(in Texture2DArray tex, in SamplerState texSamplerState, 
        in float3 UV, float2 dx, float2 dy, float noise)
    {
        // float noise = IndexNoise(UV.xy * frequency);//0.3 is a empirial number
        // float noise = valueNoise(UV.xy);

        float l = noise * IndexRange;
        float f = frac(l);

        float ia = floor(l);
        float ib = ia + 1.0;

        float2 offa = sin(float2(3.0, 7.0) * ia);
        float2 offb = sin(float2(3.0, 7.0) * ib);

        float4 texa = tex.SampleGrad(texSamplerState, float3(UV.xy + offa, UV.z), dx, dy);
        float4 texb = tex.SampleGrad(texSamplerState, float3(UV.xy + offb, UV.z), dx, dy);
    
        float4 result = lerp(texa, texb, smoothstep(0.2, 0.8, f - 0.1 * sum((texa - texb).xyz)));
        return result;
    }

    float4 SampleAlbedoNoTile(in Texture2DArray tex, in SamplerState texSamplerState, 
        in float3 UV, float noise)
    {
        // float noise = IndexNoise(UV.xy * frequency);//0.3 is a empirial number
        //float noise = valueNoise(UV.xy);

        float2 dx = ddx(UV.xy);
        float2 dy = ddy(UV.xy);

        float l = noise * IndexRange;
        float f = frac(l);

        float ia = floor(l);
        float ib = ia + 1.0;

        float2 offa = sin(float2(3.0, 7.0) * ia);
        float2 offb = sin(float2(3.0, 7.0) * ib);

        float4 texa = sRGBToLinear(tex.SampleGrad(texSamplerState, float3(UV.xy + offa, UV.z), dx, dy));
        float4 texb = sRGBToLinear(tex.SampleGrad(texSamplerState, float3(UV.xy + offb, UV.z), dx, dy));
    
        float4 result = lerp(texa, texb, smoothstep(0.2, 0.8, f - 0.1 * sum((texa - texb).xyz)));
        return result;
    }

    struct TextureSample
    {
        float4 map0Sample; // xyz: baseColor  w: ao
        float4 map1Sample; // xy : normal    zw: metallic\roughness
    };

    TextureSample TextureBombingCheap( 
        in Texture2DArray map0, 
        in SamplerState sampler0,
        in Texture2DArray map1,
        in SamplerState sampler1,
        in Texture2D noisetex,
        in SamplerState sampler2,
        in float3 UV)
    {
        TextureSample result;
        float CellScale = 0.1f;
        float4 noisevalue = noisetex.Sample(sampler2, UV.xy * CellScale);
        float4 map0_a = sRGBToLinear(map0.Sample(sampler0, UV));
        float4 map0_b = sRGBToLinear(map0.Sample(sampler0, float3(UV.xy + noisevalue.xy, UV.z)));

        float4 map1_a = map1.Sample(sampler1, UV);
        float4 map1_b = map1.Sample(sampler1, float3(UV.xy + noisevalue.xy, UV.z));

        result.map0Sample = lerp(map0_a, map0_b, noisevalue.w);
        result.map1Sample = lerp(map1_a, map1_b, noisevalue.w); 
        return result;
    }

    TextureSample SampleNoTile(
        in Texture2DArray map0, 
        in SamplerState sampler0,
        in Texture2DArray map1,
        in SamplerState sampler1,
        in float3 UV)
    {
        TextureSample result;

        float noise = indexNoise(abs(UV.xy) * frequency);
#ifdef KARST_TERRAIN
        float2 ddx_tc = ddx(UV.xy);
        float2 ddy_tc = ddy(UV.xy);

        float len_x = length(ddx_tc);
        float len_y = length(ddy_tc);

        float N = len_x > len_y ? len_x / len_y : len_y / len_x;
        N = min(8.0, N);
        float2 dx = (len_x > len_y) ? ddx_tc / N : ddx_tc;
        float2 dy = (len_y > len_x) ? ddy_tc / N : ddy_tc;
#else
        float2 dx = ddx(UV.xy);
        float2 dy = ddy(UV.xy);
#endif
        float l = noise * IndexRange;
        float f = frac(l);

        float ia = floor(l);
        float ib = ia + 1.0;

        float2 offa = sin(float2(3.0, 7.0) * ia);// hash function
        float2 offb = sin(float2(3.0, 7.0) * ib);// hash function

        float4 map0_a = sRGBToLinear(map0.SampleGrad(sampler0, float3(UV.xy + offa, UV.z), dx, dy));
        float4 map0_b = sRGBToLinear(map0.SampleGrad(sampler0, float3(UV.xy + offb, UV.z), dx, dy));

        float4 map1_a = map1.SampleGrad(sampler1, float3(UV.xy + offa, UV.z), dx, dy);
        float4 map1_b = map1.SampleGrad(sampler1, float3(UV.xy + offb, UV.z), dx, dy);

        result.map0Sample = lerp(map0_a, map0_b, smoothstep(0.2, 0.8, f - 0.1 * sum((map0_a - map0_b).xyz)));
        result.map1Sample = lerp(map1_a, map1_b, smoothstep(0.2, 0.8, f - 0.1 * sum((map1_a - map1_b).xyz))); 
        return result;
    }
    
    TextureSample DistantTile(
			in Texture2DArray map0, 
			in SamplerState sampler0,
			in Texture2DArray map1,
			in SamplerState sampler1,
			in float3 UV,
			in float2 dx,
			in float2 dy,
            in float2 nearUVTiling,
            in float2 farUVTiling,
            in float blendFactor,
            in float2 n)
		{
			TextureSample result;
            
	#ifdef KARST_TERRAIN
			float2 ddx_tc = dx;
			float2 ddy_tc = dy;

			float len_x = length(ddx_tc);
			float len_y = length(ddy_tc);

			float N = len_x > len_y ? len_x / len_y : len_y / len_x;
			N = min(8.0, N);
			dx = (len_x > len_y) ? ddx_tc / N : ddx_tc;
			dy = (len_y > len_x) ? ddy_tc / N : ddy_tc;
	#else

	#endif

			float2 uvNear = UV.xy * nearUVTiling;
            float2 uvFar = UV.xy * farUVTiling;
            float2 neardx = dx * nearUVTiling;
            float2 neardy = dy * nearUVTiling;
            float2 fardx = dx * farUVTiling;
            float2 fardy = dy * farUVTiling;
            float4 map0_a = map0.SampleGrad(sampler0, float3(uvNear, UV.z + n.x), neardx, neardy);
            float4 map0_b = map0.SampleGrad(sampler0, float3(uvFar, UV.z+ n.y), fardx, fardy);
            float4 map1_a = map1.SampleGrad(sampler1, float3(uvNear, UV.z + n.x), neardx, neardy);
            float4 map1_b = map1.SampleGrad(sampler1, float3(uvFar, UV.z+ n.y), fardx, fardy);

            result.map0Sample = lerp(map0_a, map0_b,blendFactor);
            result.map1Sample = lerp(map1_a, map1_b,blendFactor);
			return result;
		}

    TextureSample SampleNoTile(
			in Texture2DArray map0, 
			in SamplerState sampler0,
			in Texture2DArray map1,
			in SamplerState sampler1,
			in float3 UV,
			in float2 dx,
			in float2 dy,
            in int tile_switch) // tile_switch 用于控制是否开启纹理重复
		{
			TextureSample result;

			float noise = indexNoise(abs(UV.xy) * frequency) * float(tile_switch);
	#ifdef KARST_TERRAIN
			float2 ddx_tc = dx;
			float2 ddy_tc = dy;

			float len_x = length(ddx_tc);
			float len_y = length(ddy_tc);

			float N = len_x > len_y ? len_x / len_y : len_y / len_x;
			N = min(8.0, N);
			dx = (len_x > len_y) ? ddx_tc / N : ddx_tc;
			dy = (len_y > len_x) ? ddy_tc / N : ddy_tc;
	#else

	#endif
			float l = noise * IndexRange;
			float f = frac(l);

			float ia = floor(l);
			float ib = ia + 1.0;

			float2 offa = sin(float2(3.0, 7.0) * ia);// hash function
			float2 offb = sin(float2(3.0, 7.0) * ib);// hash function

			float4 map0_a = sRGBToLinear(map0.SampleGrad(sampler0, float3(UV.xy + offa, UV.z), dx, dy));
			float4 map0_b = sRGBToLinear(map0.SampleGrad(sampler0, float3(UV.xy + offb, UV.z), dx, dy));

			float4 map1_a = map1.SampleGrad(sampler1, float3(UV.xy + offa, UV.z), dx, dy);
			float4 map1_b = map1.SampleGrad(sampler1, float3(UV.xy + offb, UV.z), dx, dy);

			result.map0Sample = lerp(map0_a, map0_b, smoothstep(0.2, 0.8, f - 0.1 * sum((map0_a - map0_b).xyz)));
			result.map1Sample = lerp(map1_a, map1_b, smoothstep(0.2, 0.8, f - 0.1 * sum((map1_a - map1_b).xyz))); 
			return result;
		}
    // float4 SampleAlbedoNoTile(in Texture2DArray tex, in SamplerState texSamplerState,
    //  in float3 UV, float2 dx, float2 dy, in float noise)
    // {
    //     //float noise = IndexNoise(UV.xy * frequency);//0.3 is a empirial number
    //     // float noise = valueNoise(UV.xy);

    //     float l = noise * IndexRange;
    //     float f = frac(l);

    //     float ia = floor(l);
    //     float ib = ia + 1.0;

    //     float2 offa = sin(float2(3.0, 7.0) * ia);
    //     float2 offb = sin(float2(3.0, 7.0) * ib);

    //     float4 texa = sRGBToLinear(tex.SampleGrad(texSamplerState, float3(UV.xy + offa, UV.z), dx, dy));
    //     float4 texb = sRGBToLinear(tex.SampleGrad(texSamplerState, float3(UV.xy + offb, UV.z), dx, dy));
    
    //     float4 result = lerp(texa, texb, smoothstep(0.2, 0.8, f - 0.1 * sum((texa - texb).xyz)));
    //     return result;
    // }
//-------------------------------------------------------------------------------------------------------


// sample nomral
//-------------------------------------------------------------------------------------------------------

	// 4 layers, bilinear interpolation
    float3 SampleNormalMap(
        in Texture2DArray tex, 
        in SamplerState texSampler, 
        in float2 uv, 
        in int4 arrayIndex, 
        in float2 weights,              //该权重为双线性插值 u、v 方向的权重
        in float normalStrength
        )
    {
    float3 cTex1 = tex.Sample(texSampler, float3(uv, arrayIndex.x)).xyz;
    float3 cTex2 = tex.Sample(texSampler, float3(uv, arrayIndex.y)).xyz;
    float3 cTex3 = tex.Sample(texSampler, float3(uv, arrayIndex.z)).xyz;
    float3 cTex4 = tex.Sample(texSampler, float3(uv, arrayIndex.w)).xyz;
    return UnpackNormal(
            lerp(
                lerp(cTex4, cTex3, weights.x), 
                lerp(cTex1, cTex2, weights.x),
                weights.y
            ), 
            normalStrength);
    }

	// 4 layers, bilinear interpolation
    float3 SampleNormalMap(
        in Texture2DArray tex, 
        in SamplerState texSampler, 
        in float2 uv, 
        in int4 arrayIndex, 
        in float2 weights,              //该权重为双线性插值 u、v 方向的权重
        in float2 duvdx,
        in float2 duvdy,
        in float normalStrength)
    {
    float3 cTex1 = tex.SampleGrad(texSampler, float3(uv, arrayIndex.x), duvdx, duvdy).xyz;
    float3 cTex2 = tex.SampleGrad(texSampler, float3(uv, arrayIndex.y), duvdx, duvdy).xyz;
    float3 cTex3 = tex.SampleGrad(texSampler, float3(uv, arrayIndex.z), duvdx, duvdy).xyz;
    float3 cTex4 = tex.SampleGrad(texSampler, float3(uv, arrayIndex.w), duvdx, duvdy).xyz;
    return UnpackNormal(
            lerp(
                lerp(cTex4, cTex3, weights.x), 
                lerp(cTex1, cTex2, weights.x),
                weights.y
            ), 
            normalStrength);
    }


    float3 SampleNormalMapNoTile(
        in Texture2DArray tex,
        in SamplerState texSampler,
        in float2 uv,
        in int4 arrayIndex,
        in float2 weights,
        in float normalStrength,
        in float noise
    )
    {
        float3 cTex1 = SampleNoTile(tex, texSampler, float3(uv, arrayIndex.x), noise).xyz;
        float3 cTex2 = SampleNoTile(tex, texSampler, float3(uv, arrayIndex.y), noise).xyz;
        float3 cTex3 = SampleNoTile(tex, texSampler, float3(uv, arrayIndex.z), noise).xyz;
        float3 cTex4 = SampleNoTile(tex, texSampler, float3(uv, arrayIndex.w), noise).xyz;
        return UnpackNormal(lerp(
                lerp(cTex4, cTex3, weights.x),
                lerp(cTex1, cTex2, weights.x),
                weights.y
            ), 
            normalStrength
        );
    }
//-------------------------------------------------------------------------------------------------------

// sample albedo
//-------------------------------------------------------------------------------------------------------

   //Sampling 4 adjacent tex of id map and bilinear interpolate 4 samples
    // float4 BilinearSampleAlbedoMap(in Texture2DArray tex, in SamplerState texSampler, in float2 uv, 
    //     in uint4 arrayIndex, in float2 weights)
    // {
    //     float4 cTex1 = sRGBToLinear(tex.Sample(texSampler, float3(uv, arrayIndex.x)));
    //     float4 cTex2 = sRGBToLinear(tex.Sample(texSampler, float3(uv, arrayIndex.y)));
    //     float4 cTex3 = sRGBToLinear(tex.Sample(texSampler, float3(uv, arrayIndex.z)));
    //     float4 cTex4 = tex.Sample(texSampler, float3(uv, arrayIndex.w));//w channel dont store color data
    //     return lerp(
    //         lerp(cTex4, cTex3, weights.x),
    //         lerp(cTex1, cTex2, weights.x),
    //         weights.y);    
    // }

    // float4 TriplanarProjectionSampleAlbedoMap(in Texture2DArray tex, in SamplerState texSampler, in uint arrayIndex,
    //     in float2 UVXZ, in float2 UVXY, in float2 UVYZ, 
    //     in float weightXZ, in float weightXY, in float weightYZ)
    // {
    //     float4 cTexXZ = weightXZ * sRGBToLinear(tex.Sample(texSampler, float3(UVXZ, arrayIndex)));
    //     float4 cTexXY = weightXY * sRGBToLinear(tex.Sample(texSampler, float3(UVXY, arrayIndex)));
    //     float4 cTexYZ = weightYZ * sRGBToLinear(tex.Sample(texSampler, float3(UVYZ, arrayIndex)));

    //     return cTexXZ + cTexXY + cTexYZ;
    // }

    // float4 TriplanarProjectionSampleAlbedoMapNoTile(in Texture2DArray tex, in SamplerState texSampler, in uint arrayIndex,
    //     in float2 UVXZ, in float2 UVXY, in float2 UVYZ,
    //     in float weightXZ, in float weightXY, in float weightYZ, in float noise)
    // {
    //     float4 cTexXZ = weightXZ * SampleAlbedoNoTile(tex, texSampler, float3(UVXZ, arrayIndex), noise);
    //     float4 cTexXY = weightXY * SampleAlbedoNoTile(tex, texSampler, float3(UVXY, arrayIndex), noise);
    //     float4 cTexYZ = weightYZ * SampleAlbedoNoTile(tex, texSampler, float3(UVYZ, arrayIndex), noise);

    //     return cTexXZ + cTexXY + cTexYZ;
    // }
//-------------------------------------------------------------------------------------------------------

// sample roughness/metalness/ao mix texture
//-------------------------------------------------------------------------------------------------------
    // float4 BilinearSampleMap(in Texture2DArray tex, in SamplerState texSampler, in float2 uv, 
    //     in uint4 arrayIndex, in float2 weights)
    // {
    //     float4 cTex1 = tex.Sample(texSampler, float3(uv, arrayIndex.x));
    //     float4 cTex2 = tex.Sample(texSampler, float3(uv, arrayIndex.y));
    //     float4 cTex3 = tex.Sample(texSampler, float3(uv, arrayIndex.z));
    //     float4 cTex4 = tex.Sample(texSampler, float3(uv, arrayIndex.w));
    //     return lerp(
    //         lerp(cTex4, cTex3, weights.x),
    //         lerp(cTex1, cTex2, weights.x),
    //         weights.y);
    // }

    // float4 TriplanarProjectionSampleMap(in Texture2DArray tex, in SamplerState texSampler, in uint arrayIndex,
    //     in float2 UVXZ, in float2 UVXY, in float2 UVYZ,
    //     in float weightXZ, in float weightXY, in float weightYZ)
    // {
    //     float4 cTexXZ = weightXZ * tex.Sample(texSampler, float3(UVXZ, arrayIndex));
    //     float4 cTexXY = weightXY * tex.Sample(texSampler, float3(UVXY, arrayIndex));
    //     float4 cTexYZ = weightYZ * tex.Sample(texSampler, float3(UVYZ, arrayIndex));
    //     return cTexXZ + cTexXY + cTexYZ;
    // }

    // float4 TriplanarProjectionSampleMapNoTile(in Texture2DArray tex, in SamplerState texSampler, in uint arrayIndex,
    //     in float2 UVXZ, in float2 UVXY, in float2 UVYZ,
    //     in float weightXZ, in float weightXY, in float weightYZ, in float noise)
    // {
    //     float4 cTexXZ = weightXZ * SampleNoTile(tex, texSampler, float3(UVXZ, arrayIndex), noise);
    //     float4 cTexXY = weightXY * SampleNoTile(tex, texSampler, float3(UVXY, arrayIndex), noise);
    //     float4 cTexYZ = weightYZ * SampleNoTile(tex, texSampler, float3(UVYZ, arrayIndex), noise);
    //     return cTexXZ + cTexXY + cTexYZ;
    // }
//-------------------------------------------------------------------------------------------------------

// test functions
//-------------------------------------------------------------------------------------------------------
    // float4 BiPlanarSampleAlbedoMap(in Texture2DArray tex, in SamplerState texSampler, in uint arrayIndex,
    //     in float2 UVXZ, in float2 UVXY, in float2 UVYZ,
    //     in float weightXZ, in float weightXY, in float weightYZ)
    // {
    //     float4 cTexXZ = 
    // }
//-------------------------------------------------------------------------------------------------------
