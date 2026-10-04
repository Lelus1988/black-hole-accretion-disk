#pragma once

#include "Camera.h"
#include "Settings.h"
#include <raylib.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace GlApi {
using Enum = std::uint32_t;
using UInt = std::uint32_t;
using Int = std::int32_t;
using Size = std::ptrdiff_t;
using Char = char;
using Bool = std::uint8_t;

constexpr Enum VERSION = 0x1F02;
constexpr Enum MAJOR_VERSION = 0x821B;
constexpr Enum MINOR_VERSION = 0x821C;
constexpr Enum COMPUTE_SHADER = 0x91B9;
constexpr Enum VERTEX_SHADER = 0x8B31;
constexpr Enum FRAGMENT_SHADER = 0x8B30;
constexpr Enum COMPILE_STATUS = 0x8B81;
constexpr Enum LINK_STATUS = 0x8B82;
constexpr Enum INFO_LOG_LENGTH = 0x8B84;
constexpr Enum SHADER_STORAGE_BUFFER = 0x90D2;
constexpr Enum DYNAMIC_COPY = 0x88EA;
constexpr Enum SHADER_STORAGE_BARRIER_BIT = 0x2000;
constexpr Enum VERTEX_ATTRIB_ARRAY_BARRIER_BIT = 0x00000001;
constexpr Enum PROGRAM_POINT_SIZE = 0x8642;
constexpr Enum BLEND = 0x0BE2;
constexpr Enum SRC_ALPHA = 0x0302;
constexpr Enum ONE = 1;
constexpr Enum ONE_MINUS_SRC_ALPHA = 0x0303;
constexpr Enum POINTS = 0x0000;
constexpr Bool FALSE_VALUE = 0;

#if defined(_WIN32)
#define BLACKHOLE_GL_CALL __stdcall
#else
#define BLACKHOLE_GL_CALL
#endif
using GetStringFn = const unsigned char* (BLACKHOLE_GL_CALL *)(Enum);
using GetIntegervFn = void (BLACKHOLE_GL_CALL *)(Enum, Int*);
using CreateShaderFn = UInt (BLACKHOLE_GL_CALL *)(Enum);
using ShaderSourceFn = void (BLACKHOLE_GL_CALL *)(UInt, Int, const Char* const*, const Int*);
using CompileShaderFn = void (BLACKHOLE_GL_CALL *)(UInt);
using GetShaderivFn = void (BLACKHOLE_GL_CALL *)(UInt, Enum, Int*);
using GetShaderInfoLogFn = void (BLACKHOLE_GL_CALL *)(UInt, Int, Int*, Char*);
using DeleteShaderFn = void (BLACKHOLE_GL_CALL *)(UInt);
using CreateProgramFn = UInt (BLACKHOLE_GL_CALL *)(void);
using AttachShaderFn = void (BLACKHOLE_GL_CALL *)(UInt, UInt);
using LinkProgramFn = void (BLACKHOLE_GL_CALL *)(UInt);
using GetProgramivFn = void (BLACKHOLE_GL_CALL *)(UInt, Enum, Int*);
using GetProgramInfoLogFn = void (BLACKHOLE_GL_CALL *)(UInt, Int, Int*, Char*);
using DeleteProgramFn = void (BLACKHOLE_GL_CALL *)(UInt);
using UseProgramFn = void (BLACKHOLE_GL_CALL *)(UInt);
using GetUniformLocationFn = Int (BLACKHOLE_GL_CALL *)(UInt, const Char*);
using Uniform1fFn = void (BLACKHOLE_GL_CALL *)(Int, float);
using Uniform1uiFn = void (BLACKHOLE_GL_CALL *)(Int, UInt);
using UniformMatrix4fvFn = void (BLACKHOLE_GL_CALL *)(Int, Int, Bool, const float*);
using GenBuffersFn = void (BLACKHOLE_GL_CALL *)(Int, UInt*);
using BindBufferFn = void (BLACKHOLE_GL_CALL *)(Enum, UInt);
using BufferDataFn = void (BLACKHOLE_GL_CALL *)(Enum, Size, const void*, Enum);
using BufferSubDataFn = void (BLACKHOLE_GL_CALL *)(Enum, Size, Size, const void*);
using BindBufferBaseFn = void (BLACKHOLE_GL_CALL *)(Enum, UInt, UInt);
using DeleteBuffersFn = void (BLACKHOLE_GL_CALL *)(Int, const UInt*);
using DispatchComputeFn = void (BLACKHOLE_GL_CALL *)(UInt, UInt, UInt);
using MemoryBarrierFn = void (BLACKHOLE_GL_CALL *)(Enum);
using GenVertexArraysFn = void (BLACKHOLE_GL_CALL *)(Int, UInt*);
using BindVertexArrayFn = void (BLACKHOLE_GL_CALL *)(UInt);
using DeleteVertexArraysFn = void (BLACKHOLE_GL_CALL *)(Int, const UInt*);
using DrawArraysFn = void (BLACKHOLE_GL_CALL *)(Enum, Int, Int);
using EnableFn = void (BLACKHOLE_GL_CALL *)(Enum);
using DisableFn = void (BLACKHOLE_GL_CALL *)(Enum);
using BlendFuncFn = void (BLACKHOLE_GL_CALL *)(Enum, Enum);
#undef BLACKHOLE_GL_CALL
}

class GpuParticleSystem {
public:
    GpuParticleSystem() = default;
    ~GpuParticleSystem() = default;

    bool initialize(const Settings& settings);
    void update(float deltaTime, const Camera3D& camera, const Settings& settings);
    void renderDensity(const Camera3D& camera, const Settings& settings,
                       int viewportWidth, int viewportHeight) const;
    void reset(const Settings& settings);
    void shutdown();
    bool isActive() const { return active; }

private:
    // Keep each field 16-byte aligned to match the GLSL std430 Particle layout.
    struct ParticleData {
        float position[4];
        float velocity[4];
        float properties[4];
    };

    bool loadFunctions();
    bool loadShaders();
    GlApi::UInt compileShader(GlApi::Enum type, const std::string& source) const;
    std::string readShader(const char* filename) const;
    std::vector<ParticleData> createInitialParticles(const Settings& settings) const;

    bool active = false;
    GlApi::UInt particleBuffer = 0;
    GlApi::UInt vertexArray = 0;
    GlApi::UInt computeProgram = 0;
    GlApi::UInt renderProgram = 0;
    GlApi::Int computeDeltaTime = -1;
    GlApi::Int computeMass = -1;
    GlApi::Int computeGravity = -1;
    GlApi::Int computeHorizon = -1;
    GlApi::Int computeSpawnMin = -1;
    GlApi::Int computeSpawnMax = -1;
    GlApi::Int computeDiskThickness = -1;
    GlApi::Int computeCount = -1;
    GlApi::Int computeSeed = -1;
    GlApi::Int computeViewProjection = -1;
    GlApi::Int renderViewProjection = -1;
    GlApi::Int renderPointSize = -1;
    GlApi::Int renderHorizon = -1;
    GlApi::Int renderTime = -1;
    GlApi::Int renderLightSpeed = -1;
    GlApi::Int renderPlasmaDensity = -1;
    std::uint32_t seed = 1;

    // raylib exposes OpenGL entry points at runtime; these are loaded after context creation.
    GlApi::GetStringFn getString = nullptr;
    GlApi::GetIntegervFn getInteger = nullptr;
    GlApi::CreateShaderFn createShader = nullptr;
    GlApi::ShaderSourceFn shaderSource = nullptr;
    GlApi::CompileShaderFn compileShaderFn = nullptr;
    GlApi::GetShaderivFn getShaderiv = nullptr;
    GlApi::GetShaderInfoLogFn getShaderInfoLog = nullptr;
    GlApi::DeleteShaderFn deleteShader = nullptr;
    GlApi::CreateProgramFn createProgram = nullptr;
    GlApi::AttachShaderFn attachShader = nullptr;
    GlApi::LinkProgramFn linkProgram = nullptr;
    GlApi::GetProgramivFn getProgramiv = nullptr;
    GlApi::GetProgramInfoLogFn getProgramInfoLog = nullptr;
    GlApi::DeleteProgramFn deleteProgram = nullptr;
    GlApi::UseProgramFn useProgram = nullptr;
    GlApi::GetUniformLocationFn getUniformLocation = nullptr;
    GlApi::Uniform1fFn uniform1f = nullptr;
    GlApi::Uniform1uiFn uniform1ui = nullptr;
    GlApi::UniformMatrix4fvFn uniformMatrix4fv = nullptr;
    GlApi::GenBuffersFn genBuffers = nullptr;
    GlApi::BindBufferFn bindBuffer = nullptr;
    GlApi::BufferDataFn bufferData = nullptr;
    GlApi::BufferSubDataFn bufferSubData = nullptr;
    GlApi::BindBufferBaseFn bindBufferBase = nullptr;
    GlApi::DeleteBuffersFn deleteBuffers = nullptr;
    GlApi::DispatchComputeFn dispatchCompute = nullptr;
    GlApi::MemoryBarrierFn memoryBarrier = nullptr;
    GlApi::GenVertexArraysFn genVertexArrays = nullptr;
    GlApi::BindVertexArrayFn bindVertexArray = nullptr;
    GlApi::DeleteVertexArraysFn deleteVertexArrays = nullptr;
    GlApi::DrawArraysFn drawArrays = nullptr;
    GlApi::EnableFn enable = nullptr;
    GlApi::DisableFn disable = nullptr;
    GlApi::BlendFuncFn blendFunc = nullptr;
};
