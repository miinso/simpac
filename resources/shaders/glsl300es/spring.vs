#version 300 es
precision highp float;

// Per-vertex attributes
layout(location = 0) in vec3 a_pos_a;      // Spring start position
layout(location = 1) in vec3 a_pos_b;      // Spring end position
layout(location = 2) in float a_rest_len;  // Spring rest length
layout(location = 3) in float a_endpoint;  // 0.0 for start, 1.0 for end

// Uniforms
uniform mat4 u_viewproj;

// Output to fragment shader
out float v_strain;

void main() {
    // Select the appropriate endpoint position
    vec3 pos = mix(a_pos_a, a_pos_b, a_endpoint);

    // Transform to clip space
    gl_Position = u_viewproj * vec4(pos, 1.0);

    // Calculate strain for color mapping in fragment shader
    // strain = (current_length - rest_length) / rest_length
    vec3 diff = a_pos_a - a_pos_b;
    float current_len = length(diff);
    v_strain = (current_len - a_rest_len) / a_rest_len;
}
