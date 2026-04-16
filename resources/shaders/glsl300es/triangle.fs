#version 300 es
precision mediump float;

in vec3 v_normal;
in float v_strain;
flat in float v_flags;
uniform float u_strain_scale;
out vec4 fragColor;

void main() {
    vec3 light_dir = normalize(vec3(0.3, 1.0, 0.5));
    vec3 n = normalize(v_normal);
    float diffuse = abs(dot(n, light_dir)); // two-sided
    float lighting = 0.3 + 0.7 * diffuse;

    vec3 color;
    float s = v_strain * u_strain_scale;
    if (s > 0.0) {
        float t = clamp(s, 0.0, 1.0);
        color = mix(vec3(0.0, 1.0, 0.0), vec3(0.902, 0.161, 0.216), t);
    } else {
        float t = clamp(-s, 0.0, 1.0);
        color = mix(vec3(0.0, 1.0, 0.0), vec3(0.0, 0.475, 0.945), t);
    }

    color *= lighting;

    int flags = int(v_flags);
    bool is_hovered = (flags & 1) != 0;
    bool is_selected = (flags & 2) != 0;
    if (is_selected) {
        color = mix(color, vec3(1.0, 0.5, 0.0), 0.4);
    } else if (is_hovered) {
        color *= 1.3;
    }

    fragColor = vec4(color, 1.0);
}
