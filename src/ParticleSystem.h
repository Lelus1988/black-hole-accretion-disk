#pragma once

#include "Particle.h"
#include "Gravity.h"
#include "GpuParticleSystem.h"
#include "Settings.h"
#include <vector>

class ParticleSystem {
public:
    ParticleSystem(const Settings& settings);
    ~ParticleSystem() = default;

    void initialize();
    void update(float deltaTime, const Camera3D& camera);
    void respawnParticle(Particle& particle);
    void reset();
    void shutdown();
    bool isGPUActive() const { return gpuParticles.isActive(); }
    void renderGPUDensity(const Camera3D& camera, int width, int height) const {
        gpuParticles.renderDensity(camera, settings, width, height);
    }

    const std::vector<Particle>& getParticles() const { return particles; }
    int getAliveCount() const { return aliveCount; }

private:
    // Contiguous storage keeps the CPU fallback iteration cache-friendly.
    std::vector<Particle> particles;
    Gravity gravity;
    const Settings& settings;
    int aliveCount;
    GpuParticleSystem gpuParticles;
};
