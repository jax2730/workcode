void computeTangentSpaceVectors(in float3 n,
    out float3 tangent1, out float3 biTangent1,
    out float3 tangent2, out float3 biTangent2)
{
    float3 absN = abs(n);
    float epsilon = 1e-5f;
    //如果法线对应的平面垂直于UV投影平面，设置tangent于bitangent为0，此时该平面的采样结果对于最终混合的结果没有贡献并且切线空间也没有意义
    // XY、YZ平面的投影对应tangent space 1 
    tangent1 = absN.z < epsilon && absN.x < epsilon ? 0.f : float3(n.z, 0.f, -n.x) / (n.z * n.z + n.x * n.x);
    biTangent1 = dot(tangent1, tangent1) < epsilon ? 0.f : normalize(-cross(n.xyz, tangent1));
    
    // XZ平面的投影对应tangent space 2
    tangent2 = absN.y < epsilon && absN.x < epsilon ? 0.f : float3(n.y, -n.x, 0.f) / (n.x * n.x + n.y * n.y);
    biTangent2 = dot(tangent2, tangent2) < epsilon ? 0.f : normalize(-cross(n.xyz, tangent2));
}