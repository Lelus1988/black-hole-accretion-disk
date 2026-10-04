#pragma once

#include "ParticleSystem.h"
#include "Camera.h"
#include "Settings.h"
#include <vector>
#include <raylib.h>
#include <array>

struct Star {
    Vector3 position;
    float brightness;
};

class Renderer {
public:
    Renderer(const Settings& settings);
    ~Renderer();

    void initialize();
    void shutdown();
    void render(const ParticleSystem& particleSystem, const OrbitCamera& camera);

private:
    void renderBlackHole(float eventHorizonRadius);

    const Settings& settings;
    std::vector<Star> stars;
    bool initialized;
    RenderTexture2D sceneTarget{};
    Shader lensingShader{};
    int targetWidth = 0;
    int targetHeight = 0;
    int lensStrengthLocation = -1;
    int lensCenterLocation = -1;
    int aspectRatioLocation = -1;
    int historyTextureLocation = -1;
    int historyValidLocation = -1;
    int temporalSmoothingLocation = -1;
    int shadowRadiusLocation = -1;
    int bloomStrengthLocation = -1;
    int animationTimeLocation = -1;
    // Alternate read/write targets so the post-process shader never samples its output.
    std::array<RenderTexture2D, 2> historyTargets{};
    int historyReadIndex = 0;
    bool historyValid = false;
    bool hasPreviousCamera = false;
    Vector3 previousCameraPosition{};
    Vector3 previousCameraTarget{};

    void resizeTargetsIfNeeded();
    void renderScene(const ParticleSystem& particleSystem, const OrbitCamera& camera);
    static void drawFlippedTexture(Texture2D texture, int width, int height);
};
