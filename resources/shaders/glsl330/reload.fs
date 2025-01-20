#version 330

// Input vertex attributes (from vertex shader)
// in vec2 fragTexCoord;           // Texture coordinates (sampler2D)
// in vec4 fragColor;              // Tint color

// // Output fragment color
// out vec4 finalColor;            // Output fragment color

// // Uniform inputs
// uniform vec2 resolution;        // Viewport resolution (in pixels)
// uniform vec2 mouse;             // Mouse pixel xy coordinates
// uniform float time;             // Total run time (in secods)

// // Draw circle
// vec4 DrawCircle(vec2 fragCoord, vec2 position, float radius, vec3 color)
// {
//     float d = length(position - fragCoord) - radius;
//     float t = clamp(d, 0.0, 1.0);
//     return vec4(color, 1.0 - t);
// }

// void main()
// {
//     vec2 fragCoord = gl_FragCoord.xy;
//     vec2 position = vec2(mouse.x, resolution.y - mouse.y);
//     float radius = 40.0;

//     // Draw background layer
//     vec4 colorA = vec4(0.4,0.2,0.8, 1.0);
//     vec4 colorB = vec4(1.0,0.7,0.2, 1.0);
//     vec4 layer1 = mix(colorA, colorB, abs(sin(time*0.1)));

//     // Draw circle layer
//     vec3 color = vec3(0.9, 0.16, 0.21);
//     vec4 layer2 = DrawCircle(fragCoord, position, radius, color);

//     // Blend the two layers
//     finalColor = mix(layer1, layer2, layer2.a);
// }

// precision mediump float;
in vec2 fragTexCoord;

uniform vec2 resolution;
uniform vec2 mouse;
uniform float time;

// https://iquilezles.org/articles/distfunctions2d/
float sdfCircle(vec2 p, float r) {
      // note: sqrt(pow(p.x, 2.0) + pow(p.y, 2.0)) - r;
    return length(p) - r;
}

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

    // // note: draw circle sdf
    // float radius = 2.5;
    // // radius = 3.0;
    // vec2 center = vec2(0.0, 0.0);
    // // vec2 center = vec2(sin(2.0 * time), 0.0);
    // float distanceToCircle = sdfCircle(uv - center, radius);
    // color = distanceToCircle > 0.0 ? orange : blue;

    // square
    vec2 size = vec2(1.618, 1.0);
    // vec2 center = vec2(0.0, 0.0);
    vec2 center = (vec2(mouse.x, resolution.y - mouse.y) / resolution - 0.5) * resolution / 100.0;
    float distanceToRect = sdfRectangle(uv - center, size);

    color = distanceToRect > 0.0 ? orange : blue;
    color *= (1.0 - exp(-5.0 * abs(distanceToRect)));
    color = color * 0.8 + color * 0.2 * sin(50.0 * distanceToRect - 4.0 * time);
    color = mix(white, color, smoothstep(0.0, 0.1, abs(distanceToRect)));

    // note: adding a black outline to the circle
    // color = color * exp(distanceToCircle);
    // color = color * exp(2.0 * distanceToCircle);
    // color = color * exp(-2.0 * abs(distanceToCircle));
    // color = color * (1.0 - exp(-2.0 * abs(distanceToCircle)));
    // color = color * (1.0 - exp(-5.0 * abs(distanceToCircle)));
    // color = color * (1.0 - exp(-5.0 * abs(distanceToCircle)));

    // note: adding waves
    // color = color * 0.8 + color * 0.2;
    // color = color * 0.8 + color * 0.2 * sin(distanceToCircle);
    // color = color * 0.8 + color * 0.2 * sin(50.0 * distanceToCircle);
    // color = color * 0.8 + color * 0.2 * sin(50.0 * distanceToCircle - 4.0 * time);

    // note: adding white border to the circle
    // color = mix(white, color, step(0.1, distanceToCircle));
    // color = mix(white, color, step(0.1, abs(distanceToCircle)));
    // color = mix(white, color, smoothstep(0.0, 0.1, abs(distanceToCircle)));

    // note: thumbnail?
    // color = mix(white, color, abs(distanceToCircle));
    // color = mix(white, color, 2.0 * abs(distanceToCircle));
    // color = mix(white, color, 4.0 * abs(distanceToCircle));

    gl_FragColor = vec4(color, 1.0);
}