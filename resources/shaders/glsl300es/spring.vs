#version 300 es
precision highp float;

// per-instance attributes
layout(location = 0) in vec3 a_pos_a;      // spring start position
layout(location = 1) in vec3 a_pos_b;      // spring end position
layout(location = 2) in float a_rest_len;  // spring rest length

// uniforms
uniform mat4 u_viewproj;

// output to fragment shader
out float v_strain;

void main() {
    // use gl_VertexID to select endpoint (0 = start, 1 = end)
    vec3 pos = (gl_VertexID & 1) == 0 ? a_pos_a : a_pos_b;

    // transform to clip space
    gl_Position = u_viewproj * vec4(pos, 1.0);

    // calc strain for color mapping (computed once per spring, both verts get same value)
    vec3 diff = a_pos_a - a_pos_b;
    float current_len = length(diff);
    v_strain = (current_len - a_rest_len) / a_rest_len;
}
