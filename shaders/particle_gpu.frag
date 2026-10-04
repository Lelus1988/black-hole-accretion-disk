#version 430 core

in vec3 plasmaColor;
in float plasmaIntensity;
in vec2 motionAxis;

uniform float plasmaDensity;

out vec4 finalColor;

void main() {
    // A soft circular point keeps the particle cloud from looking like square pixels.
    vec2 point = gl_PointCoord * 2.0 - 1.0;
    float radius = length(point);
    float gaussian = exp(-radius * radius * 4.0);
    float density = gaussian * plasmaDensity * plasmaIntensity;
    finalColor = vec4(vec3(density), density);
}
