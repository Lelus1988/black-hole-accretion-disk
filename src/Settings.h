#pragma once

#include <string>

struct Settings {
    // Window settings
    int windowWidth = 1280;
    int windowHeight = 640;
    std::string windowTitle = "Black Hole";
    int targetFPS = 60;

    // Simulation and post-process defaults; the control panel can change most of these live.
    int particleCount = 1000000;
    float blackHoleMass = 50.0f;
    float eventHorizonRadius = 1.5f;
    float gravityStrength = 1.0f;
    float timeScale = 1.0f;
    float lensingStrength = 1.0f;
    float dopplerIntensity = 0.0f;
    float temporalSmoothing = 0.35f;
    float plasmaDensity = 0.025f;
    float bloomStrength = 1.0f;
    float diskThickness = 0.18f;
    float particlePointSize = 0.8f;

    // Performance settings
    bool useGPUCompute = true;
    int cpuParticleLimit = 50000;
    bool useInstancedRendering = true;
    bool useFrustumCulling = true;
    bool useLOD = true;
    int maxParticlesPerBatch = 10000;
    int lodLevels = 3;

    // Inner and outer spawn radii define the accretion disk.
    float spawnRadiusMin = 4.5f;
    float spawnRadiusMax = 20.0f;
    float initialVelocityMin = 2.0f;
    float initialVelocityMax = 5.0f;

    // Particle settings
    float particleSizeMin = 1.0f;
    float particleSizeMax = 3.0f;
    float trailLength = 0.1f;

    // Visual settings
    bool enableBloom = true;
    bool enableTrails = true;
    bool enableStars = false;
    bool enableLensing = true;
    int starCount = 1000;

    // Camera settings
    float cameraDistance = 27.0f;
    float cameraRotationSpeed = 0.005f;
    float cameraZoomSpeed = 20.0f;
};