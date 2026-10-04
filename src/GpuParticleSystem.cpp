#include "GpuParticleSystem.h"
#include "utils/Logger.h"
#include "utils/Random.h"
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace {
// Must match layout(local_size_x = 256) in particle_compute.comp.
constexpr GlApi::UInt PARTICLE_GROUP_SIZE = 256;
}

bool GpuParticleSystem::initialize(const Settings& settings) {
    if (!loadFunctions()) return false;

    GlApi::Int major = 0;
    GlApi::Int minor = 0;
    getInteger(GlApi::MAJOR_VERSION, &major);
    getInteger(GlApi::MINOR_VERSION, &minor);
    if (major < 4 || (major == 4 && minor < 3)) {
        Logger::warning("OpenGL 4.3 is unavailable; using the CPU particle simulation");
        return false;
    }

    if (!loadShaders()) {
        shutdown();
        return false;
    }

    const std::vector<ParticleData> initialParticles = createInitialParticles(settings);
    const GlApi::Size bufferSize = static_cast<GlApi::Size>(initialParticles.size() * sizeof(ParticleData));

    genBuffers(1, &particleBuffer);
    bindBuffer(GlApi::SHADER_STORAGE_BUFFER, particleBuffer);
    bufferData(GlApi::SHADER_STORAGE_BUFFER, bufferSize, initialParticles.data(), GlApi::DYNAMIC_COPY);
    bindBufferBase(GlApi::SHADER_STORAGE_BUFFER, 0, particleBuffer);
    bindBuffer(GlApi::SHADER_STORAGE_BUFFER, 0);

    genVertexArrays(1, &vertexArray);
    if (particleBuffer == 0 || vertexArray == 0) {
        Logger::error("Failed to allocate GPU particle resources");
        shutdown();
        return false;
    }

    seed = static_cast<std::uint32_t>(GetTime() * 1000000.0);
    active = true;
    Logger::info("GPU particle simulation initialized: " +
                 std::to_string(settings.particleCount) + " particles");
    return true;
}

bool GpuParticleSystem::loadFunctions() {
#define LOAD_GL_FUNCTION(member, name) \
    member = reinterpret_cast<decltype(member)>(rlGetProcAddress(name)); \
    if (member == nullptr) { Logger::error(std::string("Missing OpenGL function: ") + name); return false; }

    LOAD_GL_FUNCTION(getString, "glGetString")
    LOAD_GL_FUNCTION(getInteger, "glGetIntegerv")
    LOAD_GL_FUNCTION(createShader, "glCreateShader")
    LOAD_GL_FUNCTION(shaderSource, "glShaderSource")
    LOAD_GL_FUNCTION(compileShaderFn, "glCompileShader")
    LOAD_GL_FUNCTION(getShaderiv, "glGetShaderiv")
    LOAD_GL_FUNCTION(getShaderInfoLog, "glGetShaderInfoLog")
    LOAD_GL_FUNCTION(deleteShader, "glDeleteShader")
    LOAD_GL_FUNCTION(createProgram, "glCreateProgram")
    LOAD_GL_FUNCTION(attachShader, "glAttachShader")
    LOAD_GL_FUNCTION(linkProgram, "glLinkProgram")
    LOAD_GL_FUNCTION(getProgramiv, "glGetProgramiv")
    LOAD_GL_FUNCTION(getProgramInfoLog, "glGetProgramInfoLog")
    LOAD_GL_FUNCTION(deleteProgram, "glDeleteProgram")
    LOAD_GL_FUNCTION(useProgram, "glUseProgram")
    LOAD_GL_FUNCTION(getUniformLocation, "glGetUniformLocation")
    LOAD_GL_FUNCTION(uniform1f, "glUniform1f")
    LOAD_GL_FUNCTION(uniform1ui, "glUniform1ui")
    LOAD_GL_FUNCTION(uniformMatrix4fv, "glUniformMatrix4fv")
    LOAD_GL_FUNCTION(genBuffers, "glGenBuffers")
    LOAD_GL_FUNCTION(bindBuffer, "glBindBuffer")
    LOAD_GL_FUNCTION(bufferData, "glBufferData")
    LOAD_GL_FUNCTION(bufferSubData, "glBufferSubData")
    LOAD_GL_FUNCTION(bindBufferBase, "glBindBufferBase")
    LOAD_GL_FUNCTION(deleteBuffers, "glDeleteBuffers")
    LOAD_GL_FUNCTION(dispatchCompute, "glDispatchCompute")
    LOAD_GL_FUNCTION(memoryBarrier, "glMemoryBarrier")
    LOAD_GL_FUNCTION(genVertexArrays, "glGenVertexArrays")
    LOAD_GL_FUNCTION(bindVertexArray, "glBindVertexArray")
    LOAD_GL_FUNCTION(deleteVertexArrays, "glDeleteVertexArrays")
    LOAD_GL_FUNCTION(drawArrays, "glDrawArrays")
    LOAD_GL_FUNCTION(enable, "glEnable")
    LOAD_GL_FUNCTION(disable, "glDisable")
    LOAD_GL_FUNCTION(blendFunc, "glBlendFunc")

#undef LOAD_GL_FUNCTION
    return getString(GlApi::VERSION) != nullptr;
}

std::string GpuParticleSystem::readShader(const char* filename) const {
    // Support launching from either the project root or the build output directory.
    const std::filesystem::path paths[] = {
        std::filesystem::path("shaders") / filename,
        std::filesystem::path("bin/shaders") / filename,
        std::filesystem::path("../shaders") / filename
    };
    for (const auto& path : paths) {
        std::ifstream file(path, std::ios::binary);
        if (file) return std::string(std::istreambuf_iterator<char>(file), {});
    }
    Logger::error(std::string("Unable to read shader: ") + filename);
    return {};
}

GlApi::UInt GpuParticleSystem::compileShader(GlApi::Enum type, const std::string& source) const {
    const GlApi::UInt shader = createShader(type);
    const char* sourceText = source.c_str();
    shaderSource(shader, 1, &sourceText, nullptr);
    compileShaderFn(shader);

    GlApi::Int compiled = GlApi::FALSE_VALUE;
    getShaderiv(shader, GlApi::COMPILE_STATUS, &compiled);
    if (compiled == GlApi::FALSE_VALUE) {
        GlApi::Int logLength = 0;
        getShaderiv(shader, GlApi::INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<std::size_t>(std::max(logLength, 1)));
        getShaderInfoLog(shader, logLength, nullptr, log.data());
        Logger::error(std::string("GPU shader compile failed: ") + log.data());
        deleteShader(shader);
        return 0;
    }
    return shader;
}

bool GpuParticleSystem::loadShaders() {
    const std::string computeSource = readShader("particle_compute.comp");
    const std::string vertexSource = readShader("particle_gpu.vert");
    const std::string fragmentSource = readShader("particle_gpu.frag");
    if (computeSource.empty() || vertexSource.empty() || fragmentSource.empty()) return false;

    const GlApi::UInt computeShader = compileShader(GlApi::COMPUTE_SHADER, computeSource);
    const GlApi::UInt vertexShader = compileShader(GlApi::VERTEX_SHADER, vertexSource);
    const GlApi::UInt fragmentShader = compileShader(GlApi::FRAGMENT_SHADER, fragmentSource);
    if (computeShader == 0 || vertexShader == 0 || fragmentShader == 0) {
        if (computeShader != 0) deleteShader(computeShader);
        if (vertexShader != 0) deleteShader(vertexShader);
        if (fragmentShader != 0) deleteShader(fragmentShader);
        return false;
    }

    // The simulation and point renderer share the particle buffer but use separate programs.
    computeProgram = createProgram();
    attachShader(computeProgram, computeShader);
    linkProgram(computeProgram);
    GlApi::Int linked = GlApi::FALSE_VALUE;
    getProgramiv(computeProgram, GlApi::LINK_STATUS, &linked);
    deleteShader(computeShader);
    if (linked == GlApi::FALSE_VALUE) {
        GlApi::Int logLength = 0;
        getProgramiv(computeProgram, GlApi::INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<std::size_t>(std::max(logLength, 1)));
        getProgramInfoLog(computeProgram, logLength, nullptr, log.data());
        Logger::error(std::string("Compute shader link failed: ") + log.data());
        deleteShader(vertexShader);
        deleteShader(fragmentShader);
        return false;
    }

    renderProgram = createProgram();
    attachShader(renderProgram, vertexShader);
    attachShader(renderProgram, fragmentShader);
    linkProgram(renderProgram);
    getProgramiv(renderProgram, GlApi::LINK_STATUS, &linked);
    deleteShader(vertexShader);
    deleteShader(fragmentShader);
    if (linked == GlApi::FALSE_VALUE) {
        GlApi::Int logLength = 0;
        getProgramiv(renderProgram, GlApi::INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<std::size_t>(std::max(logLength, 1)));
        getProgramInfoLog(renderProgram, logLength, nullptr, log.data());
        Logger::error(std::string("Particle render shader link failed: ") + log.data());
        return false;
    }

    computeDeltaTime = getUniformLocation(computeProgram, "deltaTime");
    computeMass = getUniformLocation(computeProgram, "blackHoleMass");
    computeGravity = getUniformLocation(computeProgram, "gravityStrength");
    computeHorizon = getUniformLocation(computeProgram, "eventHorizonRadius");
    computeSpawnMin = getUniformLocation(computeProgram, "spawnRadiusMin");
    computeSpawnMax = getUniformLocation(computeProgram, "spawnRadiusMax");
    computeDiskThickness = getUniformLocation(computeProgram, "diskThickness");
    computeCount = getUniformLocation(computeProgram, "particleCount");
    computeSeed = getUniformLocation(computeProgram, "rngSeed");
    computeViewProjection = getUniformLocation(computeProgram, "viewProjection");
    renderViewProjection = getUniformLocation(renderProgram, "viewProjection");
    renderPointSize = getUniformLocation(renderProgram, "pointSize");
    renderHorizon = getUniformLocation(renderProgram, "eventHorizonRadius");
    renderTime = getUniformLocation(renderProgram, "time");
    renderLightSpeed = getUniformLocation(renderProgram, "effectiveLightSpeed");
    renderPlasmaDensity = getUniformLocation(renderProgram, "plasmaDensity");
    return true;
}

std::vector<GpuParticleSystem::ParticleData>
GpuParticleSystem::createInitialParticles(const Settings& settings) const {
    std::vector<ParticleData> particles(static_cast<std::size_t>(settings.particleCount));
    for (auto& particle : particles) {
        const float angle = Random::range(0.0f, 6.28318530718f);
        const float minRadiusSq = settings.spawnRadiusMin * settings.spawnRadiusMin;
        const float maxRadiusSq = settings.spawnRadiusMax * settings.spawnRadiusMax;
        const float radius = std::sqrt(Random::range(minRadiusSq, maxRadiusSq));
        const float speed = std::sqrt(settings.gravityStrength * settings.blackHoleMass / radius);
        const float y = Random::range(-settings.diskThickness, settings.diskThickness);
        particle.position[0] = std::cos(angle) * radius;
        particle.position[1] = y;
        particle.position[2] = std::sin(angle) * radius;
        particle.position[3] = 1.0f;
        particle.velocity[0] = -std::sin(angle) * speed;
        particle.velocity[1] = Random::range(-0.01f, 0.01f);
        particle.velocity[2] = std::cos(angle) * speed;
        particle.velocity[3] = 0.0f;
        particle.properties[0] = Random::range(0.5f, 1.5f);
        particle.properties[1] = 0.0f;
        particle.properties[2] = 0.0f;
        particle.properties[3] = 1.0f;
    }
    return particles;
}

void GpuParticleSystem::update(float deltaTime, const Camera3D& camera, const Settings& settings) {
    if (!active) return;

    const Matrix projection = MatrixPerspective(
        camera.fovy * DEG2RAD,
        static_cast<double>(GetScreenWidth()) / static_cast<double>(GetScreenHeight()),
        0.1, 10000.0);
    const Matrix viewProjection = MatrixMultiply(projection, GetCameraMatrix(camera));
    const float matrix[] = {
        viewProjection.m0, viewProjection.m1, viewProjection.m2, viewProjection.m3,
        viewProjection.m4, viewProjection.m5, viewProjection.m6, viewProjection.m7,
        viewProjection.m8, viewProjection.m9, viewProjection.m10, viewProjection.m11,
        viewProjection.m12, viewProjection.m13, viewProjection.m14, viewProjection.m15
    };
    useProgram(computeProgram);
    bindBufferBase(GlApi::SHADER_STORAGE_BUFFER, 0, particleBuffer);
    uniform1f(computeDeltaTime, deltaTime);
    uniform1f(computeMass, settings.blackHoleMass);
    uniform1f(computeGravity, settings.gravityStrength);
    uniform1f(computeHorizon, settings.eventHorizonRadius);
    uniform1f(computeSpawnMin, settings.spawnRadiusMin);
    uniform1f(computeSpawnMax, settings.spawnRadiusMax);
    uniform1f(computeDiskThickness, settings.diskThickness);
    uniform1ui(computeCount, static_cast<GlApi::UInt>(settings.particleCount));
    uniform1ui(computeSeed, ++seed);
    uniformMatrix4fv(computeViewProjection, 1, GlApi::FALSE_VALUE, matrix);
    // Round up so the last work group can handle any remaining particles.
    const GlApi::UInt groups = (static_cast<GlApi::UInt>(settings.particleCount) +
                           PARTICLE_GROUP_SIZE - 1) / PARTICLE_GROUP_SIZE;
    dispatchCompute(groups, 1, 1);
    // Make compute writes visible before the render shader reads the SSBO.
    memoryBarrier(GlApi::SHADER_STORAGE_BARRIER_BIT | GlApi::VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    useProgram(0);
    rlSetShader(rlGetShaderIdDefault(), rlGetShaderLocsDefault());
}

void GpuParticleSystem::renderDensity(const Camera3D& camera, const Settings& settings,
                                      int viewportWidth, int viewportHeight) const {
    if (!active) return;

    rlDrawRenderBatchActive();
    const Matrix projection = MatrixPerspective(
        camera.fovy * DEG2RAD,
        static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight),
        0.1, 10000.0);
    const Matrix viewProjection = MatrixMultiply(projection, GetCameraMatrix(camera));
    const float matrix[] = {
        viewProjection.m0, viewProjection.m1, viewProjection.m2, viewProjection.m3,
        viewProjection.m4, viewProjection.m5, viewProjection.m6, viewProjection.m7,
        viewProjection.m8, viewProjection.m9, viewProjection.m10, viewProjection.m11,
        viewProjection.m12, viewProjection.m13, viewProjection.m14, viewProjection.m15
    };

    useProgram(renderProgram);
    uniformMatrix4fv(renderViewProjection, 1, GlApi::FALSE_VALUE, matrix);
    uniform1f(renderPointSize, settings.particlePointSize);
    uniform1f(renderHorizon, settings.eventHorizonRadius);
    uniform1f(renderTime, static_cast<float>(GetTime()));
    uniform1f(renderLightSpeed, settings.dopplerIntensity);
    uniform1f(renderPlasmaDensity, settings.plasmaDensity);
    bindBufferBase(GlApi::SHADER_STORAGE_BUFFER, 0, particleBuffer);
    bindVertexArray(vertexArray);
    rlDisableDepthTest();
    rlDisableDepthMask();
    enable(GlApi::PROGRAM_POINT_SIZE);
    enable(GlApi::BLEND);
    blendFunc(GlApi::ONE, GlApi::ONE);
    drawArrays(GlApi::POINTS, 0, settings.particleCount);
    blendFunc(GlApi::SRC_ALPHA, GlApi::ONE_MINUS_SRC_ALPHA);
    disable(GlApi::PROGRAM_POINT_SIZE);
    bindVertexArray(0);
    useProgram(0);
    rlEnableDepthMask();
    rlEnableDepthTest();
    rlSetShader(rlGetShaderIdDefault(), rlGetShaderLocsDefault());
}

void GpuParticleSystem::reset(const Settings& settings) {
    if (!active) return;
    const std::vector<ParticleData> particles = createInitialParticles(settings);
    bindBuffer(GlApi::SHADER_STORAGE_BUFFER, particleBuffer);
    bufferSubData(GlApi::SHADER_STORAGE_BUFFER, 0,
                  static_cast<GlApi::Size>(particles.size() * sizeof(ParticleData)),
                  particles.data());
    bindBuffer(GlApi::SHADER_STORAGE_BUFFER, 0);
}

void GpuParticleSystem::shutdown() {
    if (particleBuffer != 0 && deleteBuffers != nullptr) {
        deleteBuffers(1, &particleBuffer);
        particleBuffer = 0;
    }
    if (vertexArray != 0 && deleteVertexArrays != nullptr) {
        deleteVertexArrays(1, &vertexArray);
        vertexArray = 0;
    }
    if (computeProgram != 0 && deleteProgram != nullptr) {
        deleteProgram(computeProgram);
        computeProgram = 0;
    }
    if (renderProgram != 0 && deleteProgram != nullptr) {
        deleteProgram(renderProgram);
        renderProgram = 0;
    }
    active = false;
}
