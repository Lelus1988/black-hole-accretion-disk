#include "Renderer.h"
#include "utils/Random.h"
#include "utils/Math.h"
#include "utils/Logger.h"
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <cmath>

Renderer::Renderer(const Settings& settings)
    : settings(settings)
    , initialized(false)
{
}

Renderer::~Renderer() {
}

void Renderer::initialize() {
    if (initialized) return;

    stars.resize(static_cast<std::size_t>(settings.starCount));
    for (auto& star : stars) {
        const float radius = Random::range(500.0f, 2000.0f);
        const float theta = Random::range(0.0f, Math::TWO_PI);
        const float phi = Random::range(0.0f, Math::MATH_PI);
        star.position.x = radius * std::sin(phi) * std::cos(theta);
        star.position.y = radius * std::sin(phi) * std::sin(theta);
        star.position.z = radius * std::cos(phi);
        star.brightness = Random::range(0.3f, 1.0f);
    }

    targetWidth = 0;
    targetHeight = 0;
    resizeTargetsIfNeeded();
    if (settings.enableLensing && sceneTarget.id != 0) {
        const char* shaderPath = FileExists("shaders/postprocess_lensing.frag")
            ? "shaders/postprocess_lensing.frag"
            : "bin/shaders/postprocess_lensing.frag";
        lensingShader = LoadShader(nullptr, shaderPath);
        if (lensingShader.id != 0) {
            historyTextureLocation = GetShaderLocation(lensingShader, "historyTexture");
        }
    }

    if (lensingShader.id == 0) {
        Logger::warning("Post-processing shader unavailable; rendering without lensing and temporal effects");
    }

    initialized = true;
    Logger::info("Renderer initialized");
}

void Renderer::render(const ParticleSystem& particleSystem, const OrbitCamera& camera) {
    resizeTargetsIfNeeded();
    if (sceneTarget.id == 0) {
        renderScene(particleSystem, camera);
        return;
    }

    const Camera3D cam = camera.getRaylibCamera();

    // Render the scene directly when the post-process shader is unavailable.
    if (lensingShader.id == 0) {
        BeginTextureMode(sceneTarget);
        renderScene(particleSystem, camera);
        EndTextureMode();
        if (particleSystem.isGPUActive()) {
            BeginTextureMode(sceneTarget);
            particleSystem.renderGPUDensity(cam, targetWidth, targetHeight);
            EndTextureMode();
        }
        drawFlippedTexture(sceneTarget.texture, targetWidth, targetHeight);
        return;
    }

    // The fragment shader renders the full scene by tracing one ray per pixel.
    // Camera movement invalidates old pixels; otherwise the history buffers smooth noise.
    const bool cameraMoved = hasPreviousCamera &&
        (Vector3Distance(cam.position, previousCameraPosition) > 0.01f ||
         Vector3Distance(cam.target, previousCameraTarget) > 0.01f);
    if (cameraMoved) historyValid = false;
    previousCameraPosition = cam.position;
    previousCameraTarget = cam.target;
    hasPreviousCamera = true;

    // Build the camera basis once; the full-screen shader casts one ray per pixel.
    const Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    const Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
    const Vector3 up = Vector3CrossProduct(right, forward);
    const float tanHalfFov = std::tan(cam.fovy * DEG2RAD * 0.5f);
    const float aspectRatio = static_cast<float>(targetWidth) / static_cast<float>(targetHeight);
    const float animationTime = static_cast<float>(GetTime());
    const int historyIsValid = historyValid ? 1 : 0;
    const int historyWriteIndex = 1 - historyReadIndex;

    auto setUniform = [&](const char* name, const void* value, int type) {
        SetShaderValue(lensingShader, GetShaderLocation(lensingShader, name), value, type);
    };

    setUniform("lensingStrength", &settings.lensingStrength, SHADER_UNIFORM_FLOAT);
    setUniform("aspectRatio", &aspectRatio, SHADER_UNIFORM_FLOAT);
    setUniform("temporalSmoothing", &settings.temporalSmoothing, SHADER_UNIFORM_FLOAT);
    setUniform("bloomStrength", &settings.bloomStrength, SHADER_UNIFORM_FLOAT);
    setUniform("animationTime", &animationTime, SHADER_UNIFORM_FLOAT);
    setUniform("historyValid", &historyIsValid, SHADER_UNIFORM_INT);

    setUniform("camPos", &cam.position, SHADER_UNIFORM_VEC3);
    setUniform("camRight", &right, SHADER_UNIFORM_VEC3);
    setUniform("camUp", &up, SHADER_UNIFORM_VEC3);
    setUniform("camForward", &forward, SHADER_UNIFORM_VEC3);
    setUniform("tanHalfFov", &tanHalfFov, SHADER_UNIFORM_FLOAT);
    setUniform("horizonRadius", &settings.eventHorizonRadius, SHADER_UNIFORM_FLOAT);
    setUniform("diskInner", &settings.spawnRadiusMin, SHADER_UNIFORM_FLOAT);
    setUniform("diskOuter", &settings.spawnRadiusMax, SHADER_UNIFORM_FLOAT);
    setUniform("diskThickness", &settings.diskThickness, SHADER_UNIFORM_FLOAT);

    SetShaderValueTexture(lensingShader, historyTextureLocation,
                          historyTargets[historyReadIndex].texture);

    // Ping-pong the history textures to avoid reading and writing the same render target.
    BeginTextureMode(historyTargets[historyWriteIndex]);
    ClearBackground(BLACK);
    BeginShaderMode(lensingShader);
    drawFlippedTexture(sceneTarget.texture, targetWidth, targetHeight);
    EndShaderMode();
    EndTextureMode();

    historyReadIndex = historyWriteIndex;
    historyValid = true;
    drawFlippedTexture(historyTargets[historyReadIndex].texture, targetWidth, targetHeight);
}

void Renderer::renderScene(const ParticleSystem& particleSystem, const OrbitCamera& camera) {
    ClearBackground(BLACK);
    const Camera3D cam = camera.getRaylibCamera();
    BeginMode3D(cam);

    if (settings.enableStars) {
        const float forwardX = cam.target.x - cam.position.x;
        const float forwardZ = cam.target.z - cam.position.z;
        const float rightLength = std::sqrt(forwardX * forwardX + forwardZ * forwardZ);
        if (rightLength > 0.0f) {
            const float pointHalfLength = Vector3Distance(cam.position, cam.target) *
                std::tan(cam.fovy * DEG2RAD * 0.5f) / static_cast<float>(GetScreenHeight());
            const float segmentX = forwardZ / rightLength * pointHalfLength;
            const float segmentZ = -forwardX / rightLength * pointHalfLength;
            rlBegin(RL_LINES);
            for (const auto& star : stars) {
                const auto brightness = static_cast<unsigned char>(255.0f * star.brightness);
                rlColor4ub(brightness, brightness, brightness, 255);
                rlVertex3f(star.position.x - segmentX, star.position.y, star.position.z - segmentZ);
                rlVertex3f(star.position.x + segmentX, star.position.y, star.position.z + segmentZ);
            }
            rlEnd();
        }
    }

    if (particleSystem.isGPUActive() && sceneTarget.id == 0) {
        particleSystem.renderGPUDensity(cam, GetScreenWidth(), GetScreenHeight());
    } else if (!particleSystem.isGPUActive() && lensingShader.id == 0) {
        const float forwardX = cam.target.x - cam.position.x;
        const float forwardZ = cam.target.z - cam.position.z;
        const float rightLength = std::sqrt(forwardX * forwardX + forwardZ * forwardZ);
        if (rightLength > 0.0f) {
            const float halfLength = Vector3Distance(cam.position, cam.target) *
                std::tan(cam.fovy * DEG2RAD * 0.5f) / static_cast<float>(GetScreenHeight());
            const float segmentX = forwardZ / rightLength * halfLength;
            const float segmentZ = -forwardX / rightLength * halfLength;
            rlBegin(RL_LINES);
            for (const auto& particle : particleSystem.getParticles()) {
                if (!particle.alive) continue;
                const Color color = particle.getColor();
                rlColor4ub(color.r, color.g, color.b, color.a);
                rlVertex3f(particle.position.x - segmentX, particle.position.y,
                           particle.position.z - segmentZ);
                rlVertex3f(particle.position.x + segmentX, particle.position.y,
                           particle.position.z + segmentZ);
            }
            rlEnd();
        }
    }

    if (lensingShader.id == 0) renderBlackHole(settings.eventHorizonRadius);
    EndMode3D();
}

void Renderer::renderBlackHole(float eventHorizonRadius) {
    const float wireRadius = eventHorizonRadius * 3.0f;
    constexpr int latitudeCount = 10;
    constexpr int longitudeCount = 18;
    constexpr int segments = 48;
    constexpr float pi = 3.14159265358979323846f;
    rlColor4ub(42, 42, 42, 255);
    rlBegin(RL_LINES);

    // Draw a latitude/longitude grid, leaving the poles uncluttered.
    for (int latitude = 1; latitude < latitudeCount; ++latitude) {
        const float angle = -0.5f * pi + pi * static_cast<float>(latitude) /
                            static_cast<float>(latitudeCount);
        const float ringRadius = wireRadius * std::cos(angle);
        const float y = wireRadius * std::sin(angle);
        for (int segment = 0; segment < segments; ++segment) {
            const float a = 2.0f * pi * static_cast<float>(segment) / segments;
            const float b = 2.0f * pi * static_cast<float>(segment + 1) / segments;
            rlVertex3f(ringRadius * std::cos(a), y, ringRadius * std::sin(a));
            rlVertex3f(ringRadius * std::cos(b), y, ringRadius * std::sin(b));
        }
    }

    for (int longitude = 0; longitude < longitudeCount; ++longitude) {
        const float angle = 2.0f * pi * static_cast<float>(longitude) /
                            static_cast<float>(longitudeCount);
        for (int segment = 0; segment < segments; ++segment) {
            const float a = -0.5f * pi + pi * static_cast<float>(segment) / segments;
            const float b = -0.5f * pi + pi * static_cast<float>(segment + 1) / segments;
            rlVertex3f(wireRadius * std::cos(a) * std::cos(angle),
                       wireRadius * std::sin(a),
                       wireRadius * std::cos(a) * std::sin(angle));
            rlVertex3f(wireRadius * std::cos(b) * std::cos(angle),
                       wireRadius * std::sin(b),
                       wireRadius * std::cos(b) * std::sin(angle));
        }
    }
    rlEnd();
    DrawSphere({0, 0, 0}, eventHorizonRadius * 0.72f, {8, 8, 8, 255});
}

void Renderer::resizeTargetsIfNeeded() {
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    if (width <= 0 || height <= 0 ||
        (width == targetWidth && height == targetHeight && sceneTarget.id != 0)) return;

    if (sceneTarget.id != 0) UnloadRenderTexture(sceneTarget);
    for (auto& history : historyTargets) {
        if (history.id != 0) UnloadRenderTexture(history);
        history = LoadRenderTexture(width, height);
    }

    targetWidth = width;
    targetHeight = height;
    sceneTarget = LoadRenderTexture(width, height);
    historyReadIndex = 0;
    historyValid = false;
    hasPreviousCamera = false;
}

void Renderer::drawFlippedTexture(Texture2D texture, int width, int height) {
    DrawTextureRec(texture,
                   {0.0f, 0.0f, static_cast<float>(width), -static_cast<float>(height)},
                   {0.0f, 0.0f}, WHITE);
}

void Renderer::shutdown() {
    if (sceneTarget.id != 0) {
        UnloadRenderTexture(sceneTarget);
        sceneTarget = {};
    }
    for (auto& history : historyTargets) {
        if (history.id != 0) UnloadRenderTexture(history);
        history = {};
    }
    if (lensingShader.id != 0) {
        UnloadShader(lensingShader);
        lensingShader = {};
    }
    historyValid = false;
    hasPreviousCamera = false;
}