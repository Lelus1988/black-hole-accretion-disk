#version 430 core

struct Particle {
    vec4 position;
    vec4 velocity;
    vec4 properties;
};

layout(std430, binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
};

uniform mat4 viewProjection;
uniform float pointSize;
uniform float eventHorizonRadius;
uniform float time;
uniform float effectiveLightSpeed;

out vec3 plasmaColor;
out float plasmaIntensity;
out vec2 motionAxis;

vec3 thermalColor(float temperature) {
    float heat = clamp(temperature, 0.0, 1.0);
    return vec3(mix(0.16, 0.48, smoothstep(0.0, 1.0, heat)));
}

void main() {
    // gl_VertexID addresses the matching particle directly in the shared SSBO.
    Particle particle = particles[gl_VertexID];
    vec3 position = particle.position.xyz;
    float radius = length(position);
    float temperature = clamp(1.0 - radius / (eventHorizonRadius * 9.0), 0.0, 1.0);

    vec4 clipPosition = viewProjection * vec4(position, 1.0);
    if (clipPosition.w <= 0.0 || radius > eventHorizonRadius * 18.0) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        gl_PointSize = 0.0;
        plasmaColor = vec3(0.0);
        plasmaIntensity = 0.0;
        motionAxis = vec2(1.0, 0.0);
        return;
    }

    gl_Position = clipPosition;
    gl_PointSize = clamp(pointSize * (55.0 / max(clipPosition.w, 1.0)), 1.0, 3.0);

    vec3 toCamera = normalize(-position);
    vec3 velocity = particle.velocity.xyz;
    float speed = length(velocity);
    float beta = clamp(dot(normalize(velocity + vec3(0.00001)), toCamera) *
                       effectiveLightSpeed * speed / (speed + 1.0), -0.72, 0.72);
    float doppler = sqrt((1.0 + beta) / (1.0 - beta));
    vec3 baseColor = thermalColor(temperature);
    plasmaColor = baseColor;

    vec4 previousClip = viewProjection * vec4(position - velocity * 0.035, 1.0);
    vec2 currentNdc = clipPosition.xy / clipPosition.w;
    vec2 previousNdc = previousClip.xy / previousClip.w;
    vec2 screenMotion = currentNdc - previousNdc;
    motionAxis = length(screenMotion) > 0.00001 ? normalize(screenMotion) : vec2(1.0, 0.0);

    float turbulence = 0.88 + 0.12 * sin(time * 1.7 + particle.properties.z * 0.11);
    plasmaIntensity = (0.45 + temperature * 0.65) * mix(0.85, 1.0, pow(doppler, 2.4)) * turbulence;
}
