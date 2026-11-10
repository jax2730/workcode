struct GBuffers
{
#ifndef SHADOWPASS
    float4 g0 : SV_TARGET0; // (rgb8)color, (a8)alpha
    float4 g1 : SV_TARGET1; // (rgb8)normal, (a8)customdata
    float4 g2 : SV_TARGET2; // (r8)metalness, (g8)roughness, (b8)ao, (a8)shadingmodel
	float4 sc : SV_TARGET3; // Scene color (Indirect lighting), (r11g11b10)
#endif
};
