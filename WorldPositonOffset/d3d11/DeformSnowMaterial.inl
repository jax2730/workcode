struct SnowData
{
	float SnowHeight;
	float Tile;
	float normalIntensity;
	float normalOffset;
};

struct ObjData
{
	float4 viewMat0;
	float4 viewMat1;
	float4 viewMat2;
	float4 viewMat3;
    float cameraNear;
	float cameraFar;
	float CameraExtend;
	uint textureSize;
	uint floatPow;
};

struct MaterialSampleSnow{
    uint matIndex[2];
    float weight;
};

void getAroundCoord(float2 matUV,in uint2 IdMapSize , out int3 aroundCoord[4],out uint aroundID[4],Texture2D<uint1> MatIDMap){
    float offset = 1.f / 512.f; //用于修复浮点数计算精度误差
    aroundCoord[0] = int3(clamp(floor(matUV + float2(-0.5f + offset,  0.5f + offset)), 0, IdMapSize - 1), 0); // 左下角
    aroundCoord[1] = int3(clamp(floor(matUV + float2( 0.5f + offset,  0.5f + offset)), 0, IdMapSize - 1), 0);  // 右下角
    aroundCoord[2] = int3(clamp(floor(matUV + float2( 0.5f + offset, -0.5f + offset)), 0, IdMapSize - 1), 0);  // 右上角
    aroundCoord[3] = int3(clamp(floor(matUV + float2(-0.5f + offset, -0.5f + offset)), 0, IdMapSize - 1), 0);   // 左上角
    aroundID[0] = MatIDMap.Load(aroundCoord[0]).r;
    aroundID[1] = MatIDMap.Load(aroundCoord[1]).r;
    aroundID[2] = MatIDMap.Load(aroundCoord[2]).r;
    aroundID[3] = MatIDMap.Load(aroundCoord[3]).r;
}

float2 SampleFBMUV(uint2 idMapSize,float2 currentUV,Texture2D FBM,SamplerState fbmSampler){
	    float2 fbmUV = currentUV * float2(1.0, float(idMapSize.y) / idMapSize.x) * 100.f;
        float2 fbm = FBM.SampleGrad(fbmSampler, fbmUV, 0,0).xy - 0.5;
        return (currentUV + fbm * 1.f / idMapSize);
}

MaterialSampleSnow SelectMaterialSnow(
        in float2 samplePoint, 
        in int3 aroundCoord[4], 
        in uint aroundID[4]

){
    MaterialSampleSnow materialSample;
    materialSample.matIndex[0] = 0;
    materialSample.matIndex[1] = 0;
    materialSample.weight = 1.f;
        
    int2 currentIDMapCoord = int2(floor(samplePoint)); // get discret coordinate of sample point
    uint currentID = 0;

    // loop over four pixels, find the pixel that current sample hits
    [unroll]
    for(int i = 0; i < 4; i++){
        if(aroundCoord[i].x == currentIDMapCoord.x && aroundCoord[i].y == currentIDMapCoord.y)
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
    materialSample.matIndex[0] = currentID;
    materialSample.matIndex[1] = nearestID;
    return materialSample;
}

