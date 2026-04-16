#version 300 es
precision mediump float;

// per-instance (divisor=1)
in vec3 a_p0;
in vec3 a_p1;
in vec3 a_p2;
in float a_rest_area;

uniform mat4 u_viewproj;

out vec3 v_normal;
out float v_strain;

void main() {
    vec3 pos = (gl_VertexID == 0) ? a_p0 : (gl_VertexID == 1) ? a_p1 : a_p2;

    vec3 e1 = a_p1 - a_p0;
    vec3 e2 = a_p2 - a_p0;
    vec3 n = cross(e1, e2);
    v_normal = normalize(n);

    float current_area = length(n) * 0.5;
    v_strain = (a_rest_area > 0.0) ? (current_area / a_rest_area - 1.0) : 0.0;

    gl_Position = u_viewproj * vec4(pos, 1.0);
}
