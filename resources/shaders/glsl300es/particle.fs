#version 300 es
precision highp float;

// input from vertex shader
in vec3 v_normal;
in vec3 v_world_pos;
in float v_state;
in float v_mass;

// uniforms
uniform vec3 u_color;

// output color
out vec4 fragColor;

void main() {
    // simple directional light from top-right
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 normal = normalize(v_normal);

    // diffuse lighting
    float diffuse = max(dot(normal, light_dir), 0.0);
    float ambient = 0.3;
    float lighting = ambient + diffuse * 0.7;

    int flags = int(v_state + 0.5);
    vec3 color = u_color;

    // check particle state
    if ((flags & 2) != 0) {
        color = mix(color, vec3(1.0, 0.6, 0.2), 0.7); // selected
    } else if ((flags & 1) != 0) {
        color = mix(color, vec3(1.0, 0.95, 0.2), 0.6); // hovered
    }
    float log10m = log2(max(v_mass, 1.0)) / 3.321928;
    float tint = 1.0 - clamp(log10m / 10.0, 0.0, 1.0); // 1->1.0, 1e10->0.0
    color *= tint;
    color *= lighting;
    fragColor = vec4(color, 1.0);
}

// enum Flag : uint8_t {
//     None = 0,
//     Hovered = 1 << 0,
//     Selected = 1 << 1,
//     Disabled = 1 << 2,
//     Pinned = 1 << 3,
// };
