#version 300 es
precision highp float;

in vec2 fragTexCoord;

uniform vec2 resolution;
uniform vec2 mouse;
uniform float time;

out vec4 finalColor;

float sdfRectangle(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    vec2 d2 = max(d, 0.0);
    return length(d2) + min(max(d.x, d.y), 0.0);
}

void main() {
    vec2 fragCoord = gl_FragCoord.xy;

    // note: set up uv coordinates
    vec2 uv = gl_FragCoord.xy / resolution;
    uv = uv - 0.5;
    uv = uv * resolution / 100.0;

      // note: set up basic colors
    vec3 black = vec3(0.0);
    vec3 white = vec3(1.0);
    vec3 red = vec3(1.0, 0.0, 0.0);
    vec3 blue = vec3(0.65, 0.85, 1.0);
    vec3 orange = vec3(0.9, 0.6, 0.3);
    vec3 color = black;
    color = vec3(uv.x, uv.y, 0.0);

    // square
    vec2 size = vec2(1.618, 2.0);
    // vec2 center = vec2(0.0, 0.0);
    vec2 center = (vec2(mouse.x, resolution.y - mouse.y) / resolution - 0.5) * resolution / 100.0;
    float distanceToRect = sdfRectangle(uv - center, size);

    color = distanceToRect > 0.0 ? orange : blue;
    color *= (1.0 - exp(-5.0 * abs(distanceToRect)));
    color = color * 0.8 + color * 0.2 * sin(50.0 * distanceToRect - 4.0 * time);
    color = mix(white, color, smoothstep(0.0, 0.1, abs(distanceToRect)));

    finalColor = vec4(color, 1.0);
}