#version 300 es
precision highp float;

// per-vertex attributes (icosphere mesh)
layout(location = 0) in vec3 a_position;  // mesh vertex position
layout(location = 1) in vec3 a_normal;    // mesh vertex normal

// per-instance attributes
layout(location = 2) in vec3 a_instance_pos;     // particle world position
layout(location = 3) in float a_instance_mass;   // particle mass
layout(location = 4) in float a_instance_state;  // particle state flags

// uniforms
uniform mat4 u_viewproj;
uniform float u_base_radius;

// output to fragment shader
out vec3 v_normal;
out vec3 v_world_pos;
out float v_state;
out float v_mass;

void main() {
    // 10000x mass -> 2x radius
    float mass = max(a_instance_mass, 1e-12);
    float radius_scale = pow(mass, 0.0752575);
    vec3 world_pos = a_instance_pos + a_position * (u_base_radius * radius_scale);
    gl_Position = u_viewproj * vec4(world_pos, 1.0);

    // pass through normal and world pos for lighting
    v_normal = a_normal;
    v_world_pos = world_pos;
    v_state = a_instance_state;
    v_mass = mass;
}
