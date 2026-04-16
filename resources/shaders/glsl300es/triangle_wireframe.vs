#version 300 es
precision mediump float;

in vec3 a_p0;
in vec3 a_p1;
in vec3 a_p2;
in float a_rest_area;

uniform mat4 u_viewproj;

void main() {
    // 6 vertices -> 3 line segments: p0-p1, p1-p2, p2-p0
    vec3 pos;
    int vid = gl_VertexID;
    if (vid == 0)      pos = a_p0;
    else if (vid == 1) pos = a_p1;
    else if (vid == 2) pos = a_p1;
    else if (vid == 3) pos = a_p2;
    else if (vid == 4) pos = a_p2;
    else               pos = a_p0;

    gl_Position = u_viewproj * vec4(pos, 1.0);
}
