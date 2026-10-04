#pragma once

#include "ParticleSystem.h"
#include "Camera.h"
#include "Renderer.h"
#include "Settings.h"
#include "PerformanceMonitor.h"

class Scene {
public:
    Scene(const Settings& settings);
    ~Scene() = default;

    void initialize();
    void update(float deltaTime);
    void render();
    void shutdown();

    void setPaused(bool paused) { this->paused = paused; }
    bool isPaused() const { return paused; }

    void setTimeScale(float scale) { settings.timeScale = scale; }
    float getTimeScale() const { return settings.timeScale; }
    void setLensingStrength(float value) { settings.lensingStrength = value; }
    void setTemporalSmoothing(float value) { settings.temporalSmoothing = value; }
    void setDopplerIntensity(float value) { settings.dopplerIntensity = value; }
    void setPlasmaDensity(float value) { settings.plasmaDensity = value; }
    void setBloomStrength(float value) { settings.bloomStrength = value; }
    void setBlackHoleMass(float value) { settings.blackHoleMass = value; }
    void setGravityStrength(float value) { settings.gravityStrength = value; }
    void resetParticles() { particleSystem.reset(); }
    int getActiveParticleCount() const { return particleSystem.getAliveCount(); }
    bool isGPUActive() const { return particleSystem.isGPUActive(); }
    void renderParticleDensity(const Camera3D& view, int width, int height) const {
        particleSystem.renderGPUDensity(view, width, height);
    }

    OrbitCamera& getCamera() { return camera; }
    const Settings& getSettings() const { return settings; }
    PerformanceMonitor& getPerfMonitor() { return perfMonitor; }

private:
    // Scene owns the simulation, camera and renderer and updates them in frame order.
    Settings settings;
    ParticleSystem particleSystem;
    OrbitCamera camera;
    Renderer renderer;
    PerformanceMonitor perfMonitor;
    bool paused;
};
