#version 300 es
precision highp float;

// input from vertex shader
in vec3 v_normal;
in vec3 v_world_pos;

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

    vec3 color = u_color * lighting;
    fragColor = vec4(color, 1.0);
}
