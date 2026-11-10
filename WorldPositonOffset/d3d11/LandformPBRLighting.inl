#define PI 3.14159265358979f

/*
float D_GGX(float NoH, float a) {
	float a2 = a * a;
	float f = (NoH * a2 - NoH) * NoH + 1.0;//TODO:clamp when f == 0
	return a2 / (PI * f * f);
}

//some notes: F_Schick拟合函数除了粗糙度之外有两个参数：
//在入射角为0度，垂直入射平面时的菲尼尔系数F0：金属与非金属不一样，二者的值通过金属度插值
//在入射角为90度，掠射平面时的菲尼尔系数F90：对于大部分平面都是1，所以在这里省略不算了
float3 F_Schlick(float u, float3 f0) {
	return f0 + (1.0 - f0) * pow(1.0 - u, 5.0);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a) {
	float a2 = a * a;
	float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
	float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
	return 0.5 / max(GGXV + GGXL, 1e-5f);
}
*/
float Fd_Lambert() {
	return 1.0 / PI;
}

float3 brdf_cooktorrance_specular(in float3 n, in float3 l, in float3 v, in float3 f0, in float roughness)
{
	float3 h = normalize(l + v);
	float NoH = saturate(dot(n, h));
	float HoL = saturate(dot(h, l));
	float NoV = saturate(dot(n, v));
	float NoL = saturate(dot(n, l));

	return D_GGX(NoH, roughness) * F_Schlick(HoL, f0) * V_SmithGGXCorrelated(NoV, NoL, roughness);
}

float3 pbr_lighting(in float3 n, in float3 l, in float3 v, in float3 albedo,
		in float3 f0, in float roughness, in float3 lightcolor)
{
	float3 h = normalize(l + v);
	float NoH = saturate(dot(n, h));
	float HoL = saturate(dot(h, l));
	float NoV = saturate(dot(n, v));
	float NoL = saturate(dot(n, l));

    //return V_SmithGGXCorrelated(NoV, NoL, roughness);
    //return NoL;

	float3 fr = D_GGX(NoH, roughness) * F_Schlick(HoL, f0) * V_SmithGGXCorrelated(NoV, NoL, roughness);
	float3 fd = albedo * Fd_Lambert();
	return PI * (fr + fd) * lightcolor * NoL;
}

// 调整光照效果，diffuse与glossy的权重由fresnel决定
float3 pbr_lighting_fresnel_weighted(in float3 n, in float3 l, in float3 v, in float3 baseColor, 
        in float metallic, in float roughness, in float3 lightColor)
{
    float3 h = normalize(l + v);
    float NoH = saturate(dot(n, h));
    float HoL = saturate(dot(h, l));
    float NoV = saturate(dot(n, v));
    float NoL = saturate(dot(n, l));

    //remapping alpha
    float alpha = roughness * roughness;

    float3 f0 = lerp(0.04f, baseColor, metallic);
    float3 F  = F_Schlick(HoL, f0);
    
    //diffuse term模拟的现象为光线被物体在物体内u吧
    //只有电解质会有diffuse散射
    //金属只发生反射，没有diffuse项
    float3 diffuseColor = baseColor * (1.f - metallic);

    float3 Fd = Fd_Lambert() * diffuseColor * (1.f - F);
    float3 Fr = F * D_GGX(NoH, alpha) * V_SmithGGXCorrelated(NoV, NoL, alpha);

    return lightColor * NoL * (Fr + Fd);
}