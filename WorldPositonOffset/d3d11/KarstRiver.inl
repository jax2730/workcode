
float3 riverShade(float3 N, float3 E, float3 H, float3 mainLightColor, float3 reflectColor, float fNDotL)
{
    float NdotH = dot(N, H);
    float fresnel = (0.04 + (1.0 - 0.04) * (pow(1.0 - max(0.0, dot(-N, E)), 5.0)));
    float3 waterColor = float3(0.015f, 0.05f, 0.16f);
    reflectColor *= waterColor;
    float3 scattering = float3(0.1117, 0.1217, 0.1317) * 0.2;
    return (waterColor * (1.0 - fresnel) * fNDotL + pow(NdotH, 25.0)) * mainLightColor  + fresnel * reflectColor + scattering;
}