mat2 rotate2d(float r) {
    return mat2(cos(r), sin(r), -sin(r), cos(r));
}

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float value_noise(vec2 p) {
    vec2 cell = floor(p);
    vec2 f = fract(p);
    vec2 w = f * f * (3.0 - 2.0 * f);

    float a = hash21(cell);
    float b = hash21(cell + vec2(1.0, 0.0));
    float c = hash21(cell + vec2(0.0, 1.0));
    float d = hash21(cell + vec2(1.0, 1.0));

    return mix(mix(a, b, w.x), mix(c, d, w.x), w.y);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 P = (fragCoord - iResolution.xy * 0.4)
           / iResolution.y * 600.0;

    vec2 u = vec2(dot(P, vec2(0.6))) / 600.0;
    vec3 col = vec3(0.0);

    for (float i = 1.0; i < 16.0; i += 1.0 / i) {
        mat2 m = mat2(2.0, -2.0, 1.0, 4.0);
        vec2 p = m * (P + u * i) / 40.0;

        // 避免中心点 log(0)。
        float l = max(length(p), 1e-5);

        float d = cos(
            sin(ceil(log(l) * 30.0) * 100.0) * 200.0
            + 0.4 * iTime
        );

        float n = value_noise(0.5 * vec2(l))
                * value_noise(rotate2d(0.2 * d) * (p * 5.0));

        u = rotate2d(2.4) * u;

        // 避免 atan(0, 0)。
        float angle = dot(p, p) > 0.0 ? atan(p.y, p.x) : 0.0;
        vec3 hue = cos(
            angle * 3.0 + d * 1.39 + vec3(0.0, 0.6, 1.2)
        ) + 1.1;

        col += pow(
            max(n * hue, vec3(0.0)) * sqrt(l) * 0.1,
            vec3(3.0)
        );
    }

    fragColor = vec4(sqrt(col / (1.0 + col)), 1.0);
}