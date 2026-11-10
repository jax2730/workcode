#define INV_PI 0.3183098861837907

//环境光绕Y轴旋转，旋转法线或反射光方向
half3 worldToEnvironment(half3 N, half rotateAngle){
	half3x3 rotateMatrix;
	half radianQ = radians(rotateAngle);
	rotateMatrix[0] = half3(cos(radianQ), 0, -sin(radianQ));
	rotateMatrix[1] = half3(0, 1, 0);
	rotateMatrix[2] = half3(sin(radianQ), 0, cos(radianQ));

	return mul(rotateMatrix, N);
}

half3 ApproxDiffuseIrradiance(half3 N, 
                            half4x4 rMatrix, 
                            half4x4 gMatrix, 
                            half4x4 bMatrix, 
                            half rotateAngle)
{				
	//需要额外旋转90度与环境图对齐
	half3x3 rotateYasixMatrix;
	rotateYasixMatrix[0] = float3(0.h, 0.h, 1.h);
	rotateYasixMatrix[1] = float3(0.h, 1.h, 0.h);
	rotateYasixMatrix[2] = float3(-1.h, 0.h, 0.h);
	N = mul(rotateYasixMatrix, N);

	//生成球谐的坐标系手系为左手系，因此在此处交换zy
	half4 nDir = half4(worldToEnvironment(N, rotateAngle).xzy, 1.0h);

	return max(0.0h, half3(
		dot(nDir, mul(rMatrix,nDir)),
		dot(nDir, mul(gMatrix,nDir)),
		dot(nDir, mul(bMatrix,nDir))
	));	
}

half3 CalculateDiffuseEnvColor(half3 N, 
                        half3 baseColor, 
                        half metallic, 
                        half ao, 
                        half4x4 rMat, 
                        half4x4 gMat, 
                        half4x4 bMat, 
                        half rotateAngle)
{
    half3 albedo = baseColor * (1.h - metallic);
    half3 diffuseLighting = ApproxDiffuseIrradiance(N, rMat, gMat, bMat,rotateAngle);
    return albedo * ao * diffuseLighting;
    // return albedo * diffuseLighting;
}


// see https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
//拟合预滤波的glossy brdf
half3 ApproxGlossyBRDF(half3 specularColor, half roughness, half NoV)
{
	half4 c0 = half4(-1.0h,	-0.0275h,	-0.572h,		0.022h);
	half4 c1 = half4(1.0h,		0.0425h,		1.04h,		-0.04h);
	
	half4 r = roughness * c0 + c1;
	half a004 = min(r.x * r.x, exp2(-9.28h * NoV)) * r.x + r.y;
	half2 AB = half2(-1.04h, 1.04h) * a004 + r.zw;
	return max(specularColor * AB.x + AB.y, 1e-6h);
}

half3 ApproxGlossyBRDF(half3 N, 
                       half3 V, 
                       half3 baseColor, 
                       half metallic,
                       half roughness,
                       half wetFinal)
{
    half NdotV = max(dot(N, V), 1e-6h);
    half3 specularColor = lerp(0.04h, baseColor, metallic);
    specularColor = lerp(specularColor, 0.5h, wetFinal);

    return ApproxGlossyBRDF(specularColor, roughness, NdotV);
}

//计算各向同性的GGX高光，各项异性材质最好不要使用
half3 ApproxGlossyIrradiance(half3 N, 
                        half3 V, 
                        half roughness,
                        half rotateAngle, 
                        half mipmapMaxLevel,
                        TextureCube envMap,
                        SamplerState envMapSamplerState                        
                        )
{
    half4 refl;
    refl.xyz = reflect(-V, N);
    refl.xyz = worldToEnvironment(refl.xyz, rotateAngle);
    refl.w = clamp(mipmapMaxLevel * roughness, 0.0h, mipmapMaxLevel);

    // 预滤波的环境图在生成时就在线性空间存储颜色，不需要在采样后做gamma
    half4 mapColor = envMap.SampleLevel(envMapSamplerState, refl.xyz, refl.w);
    return mapColor.xyz * mapColor.w * 10.h;//alpha 通道存放亮度，最大亮度为10
}

half3 CalculateGlossyEnvColor(half3 N, 
                        half3 V, 
                        half3 baseColor, 
                        half metallic, 
                        half roughness,
                        half ao,
                        half wetFinal,
                        half rotateAngle, 
                        half mipmapMaxLevel,
                        TextureCube envMap,
                        SamplerState envMapSamplerState)
{
    half NdotV = max(dot(N, V), 1e-6h);
    half3 specularColor = lerp(0.04h, baseColor, metallic);
    specularColor = lerp(specularColor, 0.5h, wetFinal);

    half3 filteredBRDF = ApproxGlossyBRDF(specularColor, roughness, NdotV);
    half specularAO = lerp(ao, 1.0h, metallic * (1.0h - roughness) * (1.0h - roughness));
    
    half3 envLighting = ApproxGlossyIrradiance(N, V, roughness, rotateAngle, mipmapMaxLevel,
            envMap, envMapSamplerState);

    return filteredBRDF * specularAO * envLighting; 
    // return filteredBRDF * envLighting; 
}

//未使用环境贴图时，默认环境光为rgb = 1.0
half3 CalculateDefaultEnvColor(half3 N, 
                            half3 V, 
                            half3 baseColor,
                            half metallic, 
                            half roughness, 
                            half ao, 
                            half wetFinal)
{
    half NdotV = max(dot(N, V), 1e-6h);
    half3 specularColor = lerp(0.04h, baseColor, metallic);
    half specularAO = lerp(ao, 1.0h, metallic * (1.0h - roughness) * (1.0h - roughness));
    half3 filteredBRDF = max(ApproxGlossyBRDF(specularColor, roughness, NdotV), 1e-6h);

    half3 albedo = baseColor * (1.0h - metallic);

    return albedo * ao * INV_PI + filteredBRDF * specularAO;
    // return albedo * INV_PI + filteredBRDF;
}

half3 ComputeAmbientLight(
    half3 skyLight,
    half3 groundLight,
    half3 normal,
    half blendThreshold, half3 gravityDir)
{

    //half normalY = normal.y;
    half normalY = dot(normal, gravityDir);
    half blendParam = smoothstep(1e-5h, 1.h, (normalY * 0.5h + 0.5h * blendThreshold) / (blendThreshold + 1e-5h));
    half3 ambientColor = lerp(groundLight, skyLight, blendParam);

    return ambientColor;
}