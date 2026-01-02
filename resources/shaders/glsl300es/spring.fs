#version 300 es
precision highp float;

// Input from vertex shader
in float v_strain;

// Output color
out vec4 fragColor;

// Uniforms
uniform float u_strain_scale;  // Default: 10.0

void main() {
    // Strain-based colors
    vec3 green = vec3(0.0, 1.0, 0.0);           // Rest state
    vec3 red = vec3(0.902, 0.161, 0.216);       // Tension (230, 41, 55)
    vec3 blue = vec3(0.0, 0.475, 0.945);        // Compression (0, 121, 241)

    vec3 color;
    if (v_strain > 0.0) {
        // Tension: interpolate green -> red
        float t = clamp(v_strain * u_strain_scale, 0.0, 1.0);
        color = mix(green, red, t);
    } else {
        // Compression: interpolate green -> blue
        float t = clamp(-v_strain * u_strain_scale, 0.0, 1.0);
        color = mix(green, blue, t);
    }

    fragColor = vec4(color, 1.0);
}
