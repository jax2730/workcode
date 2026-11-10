#version 330 core

// Rainbow Travel：根据所给片段补全的演示

vec3 getPath(float z) {
    return vec3(
        12.0 * cos(z * vec2(0.1, 0.12)),
        z
    );
}

// 非负输入的 tanh，避免依赖 GLSL 版本是否支持 tanh。
vec3 toneMap(vec3 x) {
    vec3 e = exp(-2.0 * x);
    return (1.0 - e) / (1.0 + e);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (2.0 * fragCoord - iResolution.xy)
            / iResolution.y;

    // 相机沿弯曲路径前进。
    float z = iTime * 4.0;
    vec3 cameraPos = getPath(z);

    vec3 forward = normalize(getPath(z + 1.0) - cameraPos);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);

    vec3 rayDir = normalize(
        forward * 1.6 + right * uv.x + up * uv.y
    );

    vec3 accumulatedColor = vec3(0.0);
    float travel = 0.1;

    // 前方的发光球随相机一起沿轨迹移动。
    vec3 orbCenter = getPath(z + 18.0);

    for (int sampleIndex = 0; sampleIndex < 80; ++sampleIndex) {
        float i = float(sampleIndex + 1);
        vec3 rayPos = cameraPos + rayDir * travel;

        // 将位置转换到弯曲通道附近的局部坐标。
        vec3 fluidPos = vec3(
            rayPos.xy - getPath(rayPos.z).xy,
            rayPos.z
        ) * 0.35;

        // 七次递归正弦扰动。
        for (int octave = 1; octave <= 7; ++octave) {
            float j = float(octave);
            fluidPos += sin(
                fluidPos.yzx * j + iTime + 0.5 * i
            ) / j;
        }

        float radius = length(
            rayPos.xy - getPath(rayPos.z).xy
        );

        // 艺术化通道距离估计，并非严格的有符号距离场。
        float tunnelRadius = 4.0 + 0.6 * sin(fluidPos.y);
        float stepDist = max(
            abs(radius - tunnelRadius) * 0.3,
            0.04
        );

        float orbDist = abs(
            length(rayPos - orbCenter) - 1.2
        );

        vec3 palette = 1.0 + cos(
            fluidPos.y + i * 0.4 + vec3(6.0, 1.0, 2.0)
        );

        accumulatedColor += (
            2.5 * palette / max(stepDist, 0.01)
            + 10.0 * palette / max(orbDist, 0.6)
        ) / i;

        travel += clamp(stepDist, 0.08, 0.8);

        if (travel > 60.0) {
            break;
        }
    }

    // 降低演示曝光，保留色彩层次。
    vec3 exposed = accumulatedColor * 0.12;
    vec3 color = toneMap(exposed * exposed / 1500.0);

    fragColor = vec4(color, 1.0);
}