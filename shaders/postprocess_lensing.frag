#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D historyTexture;
uniform vec4 colDiffuse;

uniform float lensingStrength;    // Values above 1.0 exaggerate the lensing.
uniform float aspectRatio;
uniform float temporalSmoothing;
uniform float bloomStrength;
uniform float animationTime;
uniform int historyValid;

uniform vec3 camPos;
uniform vec3 camRight;
uniform vec3 camUp;
uniform vec3 camForward;
uniform float tanHalfFov;
uniform float horizonRadius;
uniform float diskInner;
uniform float diskOuter;
uniform float diskThickness;

out vec4 finalColor;

const float PI = 3.14159265359;
const int   MAX_STEPS = 320;      // Lower this value if performance is limited.

// Visual settings
const vec3  BACKGROUND   = vec3(0.012, 0.013, 0.016);  // Use vec3(0.42) for a gray background.
const vec3  SILVER_COLD  = vec3(0.34, 0.36, 0.40);
const vec3  SILVER_HOT   = vec3(1.00, 0.97, 0.92);
const vec3  GRID_COLOR   = vec3(0.78, 0.85, 0.95);
const vec3  FILL_COLOR   = vec3(0.10, 0.11, 0.13);
const float EXPOSURE     = 1.0;
const float DISK_DENSITY = 0.7;    // Accretion disk opacity.
const float DISK_SPEED   = 6.0;    // Disk rotation speed.
const float DOPPLER_BETA = 0.55;   // Doppler beaming strength.
const float RING_GAIN    = 0.12;   // Photon ring brightness.
const float GRID_STEP    = PI / 12.0;  // Sphere grid spacing (15 degrees).
// Hash functions
float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float hash13(vec3 p3) {
    p3 = fract(p3 * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 hash33(vec3 p3) {
    p3 = fract(p3 * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}

vec3 acesToneMap(vec3 c) {
    return clamp((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14), 0.0, 1.0);
}

// Procedural star field
vec3 starfield(vec3 d) {
    vec3 col = vec3(0.0);
    for (int k = 0; k < 2; ++k) {
        float scale = (k == 0) ? 60.0 : 130.0;
        float sharp = (k == 0) ? 200.0 : 350.0;
        vec3 p = d * scale;
        vec3 cell = floor(p);
        vec3 f = fract(p);
        vec3 rnd = hash33(cell + float(k) * 19.0);
        float present = step(0.93, hash13(cell * 1.7 + 5.0));
        vec3 center = 0.25 + 0.5 * rnd;
        float dd = length(f - center);
        float s = present * exp(-dd * dd * sharp) * (0.4 + 1.6 * rnd.z * rnd.z);
        col += vec3(0.8 + 0.2 * rnd.x, 0.85, 0.9 + 0.1 * rnd.y) * s;
    }
    return col;
}

// Wireframe sphere
vec3 sphereColor(vec3 n, vec3 rd) {
    float lat = asin(clamp(n.y, -1.0, 1.0));
    float lon = atan(n.z, n.x) + animationTime * 0.04;
    vec2 g = vec2(lat, lon) / GRID_STEP;
    vec2 f = abs(fract(g + 0.5) - 0.5);
    float cl = max(cos(lat), 0.0);

    float latLine = 1.0 - smoothstep(0.03, 0.09, f.x);
    float lonLine = (1.0 - smoothstep(0.03, 0.09, f.y * cl)) * smoothstep(0.1, 0.4, cl);
    float line = max(latLine, lonLine);

    float facing = clamp(dot(n, -rd), 0.0, 1.0);
    float rim = pow(1.0 - facing, 3.0);

    vec3 fill = FILL_COLOR * (0.5 + 0.9 * facing);
    vec3 lines = GRID_COLOR * (0.55 + 1.2 * rim);
    return fill + lines * line + GRID_COLOR * rim * 0.18;
}

// Particle disk grain
// Each ring rotates at its own Keplerian speed to preserve differential rotation.
float diskGrain(vec3 p, float rxz, float H, out float haze) {
    const float CELL = 0.10;
    float ring  = floor(rxz / CELL);
    float rc    = (ring + 0.5) * CELL;
    // Each radial band gets its own orbital speed, so the grain doesn't wind into one spiral.
    float omega = DISK_SPEED * inversesqrt(rc * rc * rc);
    float ang   = atan(p.z, p.x);
    float turns = fract(ang * (0.5 / PI) - omega * animationTime * (0.5 / PI));
    float cells = max(floor(2.0 * PI * rc / CELL), 8.0);
    float u     = turns * cells;
    float cellId = floor(u);
    float yId   = floor(p.y / CELL + 0.5);
    vec3  local = vec3(fract(u), fract(rxz / CELL), fract(p.y / CELL + 0.5));

    // Add irregular edges, ring structure and clumps.
    float edgeIn  = smoothstep(diskInner * 0.92, diskInner * 1.10,
                               rxz + (hash11(ring * 0.37 + 11.0) - 0.5) * 0.4);
    float edgeOut = 1.0 - smoothstep(diskOuter * 0.72, diskOuter,
                               rxz + (hash11(ring * 0.71 + 3.0) - 0.5) * 4.0);
    float coarse = floor(rxz / 0.55);
    float bands  = 0.45 + 0.55 * hash11(coarse * 1.31 + 7.0);
    float omegaC = DISK_SPEED * inversesqrt(pow((coarse + 0.5) * 0.55, 3.0));
    float clump  = 0.65 + 0.35 * cos(2.0 * (ang - omegaC * animationTime)
                                     + 6.2831 * hash11(coarse + 21.0));

    float vert = exp(-0.5 * (p.y * p.y) / (H * H));
    float presence = 0.85 * pow(diskInner / rxz, 0.6) * vert * edgeIn * edgeOut * bands * clump;
    haze = presence;

    vec3 rnd = hash33(vec3(cellId, ring, yId));
    float keep = hash13(vec3(cellId + 31.0, ring + 5.0, yId + 9.0));
    vec3 d = local - (0.3 + 0.4 * rnd);
    float particle = exp(-dot(d, d) * 45.0) * (0.4 + 0.6 * rnd.x);
    return keep < presence ? particle : 0.0;
}

void main() {
    vec2 ndc = fragTexCoord * 2.0 - 1.0;
    ndc.x *= aspectRatio;
    vec3 dir = normalize(camForward + (camRight * ndc.x + camUp * ndc.y) * tanHalfFov);
    vec3 pos = camPos;

    float R = horizonRadius;
    float camDist = length(camPos);
    float rEscape = max(camDist * 1.3, 60.0);

    vec3  acc = vec3(0.0);
    float T = 1.0;
    float photon = 0.0;
    float halo = 0.0;
    bool  escaped = false;
    float jitter = hash12(gl_FragCoord.xy + fract(animationTime) * 91.7);

    // March along the bent view ray, accumulating disk light until it escapes or hits the horizon.
    for (int i = 0; i < MAX_STEPS; ++i) {
        float r = length(pos);

        if (r < R) {                       // The ray hit the event horizon.
            acc += T * sphereColor(pos / r, dir);
            T = 0.0;
            break;
        }
        if (r > rEscape || (r > camDist * 1.05 && dot(pos, dir) > 0.0)) {
            escaped = true;
            break;
        }

        float rxz = length(pos.xz);
        // Smaller steps near the hole and through the disk preserve the thin silhouette.
        float dt = clamp(0.07 * (r - 0.8 * R), 0.03, 1.2);
        float H = diskThickness * (0.7 + 0.05 * max(rxz - diskInner, 0.0));
        bool inSlab = abs(pos.y) < 4.0 * H && rxz > diskInner * 0.8 && rxz < diskOuter * 1.05;
        if (inSlab) dt = min(dt, 0.1);
        if (i == 0) dt *= jitter;

        // Rays that orbit near 1.5 R build up the photon ring and its soft halo.
        float dr = (r - 1.5 * R) / (0.22 * R);
        photon += dt * exp(-dr * dr);
        float dh = (r - 1.5 * R) / (1.8 * R);
        halo += dt * exp(-dh * dh);

        if (inSlab) {
            float haze;
            float grain = diskGrain(pos, rxz, H, haze);
            float sigma = (grain * 9.0 + haze * 0.15) * DISK_DENSITY;
            if (sigma > 0.001) {
                float a = 1.0 - exp(-sigma * dt);

                vec3 vdir = vec3(-pos.z, 0.0, pos.x) / max(rxz, 1e-4);
                float beta  = DOPPLER_BETA * sqrt(diskInner / rxz);
                float gamma = inversesqrt(1.0 - beta * beta);
                float D = 1.0 / (gamma * (1.0 + beta * dot(vdir, dir)));
                float grav = sqrt(max(1.0 - R / r, 0.02));
                float beam = clamp(D * D * D, 0.12, 5.0) * grav;

                float heat = clamp(pow(diskInner / rxz, 1.2) * pow(D, 1.2), 0.0, 1.0);
                vec3 col = mix(SILVER_COLD, SILVER_HOT, heat);
                float radial = 0.25 + 1.4 * pow(diskInner / rxz, 1.4);

                acc += T * a * col * beam * radial * (0.3 + 1.2 * grain);
                T *= 1.0 - 0.9 * a;
            }
        }

        // Approximate Schwarzschild light bending while marching this pixel's view ray.
        vec3 L = cross(pos, dir);
        float r2 = r * r;
        dir = normalize(dir - lensingStrength * 1.5 * R * dot(L, L) * pos / (r2 * r2 * r) * dt);
        pos += dir * dt;

        if (T < 0.015) break;
    }

    vec3 bg = BACKGROUND + starfield(normalize(dir));
    float bgWeight = escaped ? T : 0.0;

    vec3 hdr = acc + bg * bgWeight
             + vec3(1.0, 0.97, 0.92) * photon * RING_GAIN
             + vec3(0.55, 0.62, 0.75) * halo * 0.02 * bloomStrength;
    hdr *= EXPOSURE;

    vec3 ldr = acesToneMap(hdr);
    ldr = pow(ldr, vec3(1.0 / 2.2));

    vec2 uv = fragTexCoord * 2.0 - 1.0;
    ldr *= 1.0 - 0.3 * dot(uv * vec2(0.7, 1.0), uv * vec2(0.7, 1.0));

    // History is stored after tone mapping, so clamp it to the current frame's color range.
    vec3 history = texture(historyTexture, fragTexCoord).rgb;
    history = clamp(history, ldr - vec3(0.25), ldr + vec3(0.25));
    float w = historyValid != 0 ? temporalSmoothing : 0.0;
    ldr = mix(ldr, history, w);

    ldr += (hash12(gl_FragCoord.xy + fract(animationTime) * 113.0) - 0.5) * 0.02;

    finalColor = vec4(ldr, 1.0) * colDiffuse * fragColor;
}