#include "App.h"
#include "utils/Logger.h"
#include <imgui.h>
#include <rlImGui.h>
#include <algorithm>
#include <raylib.h>

App::App()
    : scene(settings)
    , running(false)
    , showDebug(false)
    , showControls(false)
    , lastFrameTime(Timer::now())
{
}

App::~App() {
    shutdown();
}

bool App::initialize() {
    Logger::info("Initializing Black Hole...");

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(settings.windowWidth, settings.windowHeight, settings.windowTitle.c_str());
    if (!IsWindowReady()) {
        Logger::error("Failed to create window");
        return false;
    }

    rlImGuiSetup(true);
    SetTargetFPS(settings.targetFPS);

    // Initialize scene
    scene.initialize();

    running = true;
    lastFrameTime = Timer::now();

    Logger::info("Initialization complete");
    return true;
}

void App::run() {
    while (running && !WindowShouldClose()) {
        // Cap long frames so a pause in the debugger doesn't launch particles across the scene.
        Timer::TimePoint currentTime = Timer::now();
        float deltaTime = std::min(Timer::delta(lastFrameTime, currentTime), 0.05f);
        lastFrameTime = currentTime;

        handleInput();
        // The camera should still move while the ImGui panel is hidden.
        scene.getCamera().setMouseCaptured(
            showControls && ImGui::GetIO().WantCaptureMouse);

        scene.update(deltaTime);

        BeginDrawing();
        scene.render();
        if (showControls) {
            renderControlPanel();
        }

        if (showDebug) {
            renderDebugOverlay();
        }

        EndDrawing();
    }
}

void App::shutdown() {
    if (!running) return;
    Logger::info("Shutting down...");
    rlImGuiShutdown();
    scene.shutdown();
    CloseWindow();
    running = false;
}

void App::handleInput() {
    // Pause/Resume
    if (IsKeyPressed(KEY_SPACE)) {
        scene.setPaused(!scene.isPaused());
        Logger::info(scene.isPaused() ? "Simulation paused" : "Simulation resumed");
    }

    // Reset camera
    if (IsKeyPressed(KEY_R)) {
        scene.getCamera().reset();
        scene.resetParticles();
        Logger::info("Camera and particle distribution reset");
    }

    // Time scale
    if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL)) {
        float newScale = scene.getTimeScale() + 0.25f;
        scene.setTimeScale(newScale);
        Logger::info("Time scale: " + std::to_string(newScale));
    }
    if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS)) {
        float newScale = scene.getTimeScale() - 0.25f;
        if (newScale >= 0.0f) {
            scene.setTimeScale(newScale);
            Logger::info("Time scale: " + std::to_string(newScale));
        }
    }

    // Toggle debug overlay
    if (IsKeyPressed(KEY_F1)) {
        showDebug = !showDebug;
    }
    if (IsKeyPressed(KEY_F2)) {
        showControls = !showControls;
    }

    // Screenshot
    if (IsKeyPressed(KEY_F12)) {
        TakeScreenshot("assets/screenshots/screenshot.png");
        Logger::info("Screenshot saved");
    }
}

void App::renderDebugOverlay() {
    int fps = GetFPS();
    float timeScale = scene.getTimeScale();
    std::string pausedText = scene.isPaused() ? " (PAUSED)" : "";

    DrawText("=== DEBUG OVERLAY ===", 10, 10, 16, WHITE);
    DrawText(TextFormat("FPS: %d%s", fps, pausedText.c_str()), 10, 30, 16, WHITE);
    DrawText(TextFormat("Time Scale: %.2fx", timeScale), 10, 50, 16, WHITE);

    DrawText("", 10, 70, 16, WHITE);
    DrawText("Controls:", 10, 90, 16, WHITE);
    DrawText("Mouse Left - Orbit", 10, 110, 16, WHITE);
    DrawText("Mouse Right - Pan", 10, 130, 16, WHITE);
    DrawText("Scroll - Zoom", 10, 150, 16, WHITE);
    DrawText("Space - Pause", 10, 170, 16, WHITE);
    DrawText("R - Reset disk/camera", 10, 190, 16, WHITE);
    DrawText("+/- - Time scale", 10, 210, 16, WHITE);
    DrawText("F1 - Toggle debug", 10, 230, 16, WHITE);
    DrawText("F2 - Toggle controls", 10, 250, 16, WHITE);
    DrawText("F12 - Screenshot", 10, 270, 16, WHITE);
}

void App::renderControlPanel() {
    rlImGuiBegin();
    ImGui::SetNextWindowSize(ImVec2(350.0f, 320.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Black Hole");
    ImGui::TextColored(ImVec4(1.0f, 0.57f, 0.19f, 1.0f), "Accretion Disk Simulation");
    ImGui::Separator();
    ImGui::Text("Renderer: %s", scene.isGPUActive() ? "OpenGL 4.3 Compute" : "CPU fallback");
    ImGui::Text("Particles: %d  |  FPS: %d", scene.getActiveParticleCount(), GetFPS());

    float lensing = scene.getSettings().lensingStrength;
    if (ImGui::SliderFloat("Lensing", &lensing, 0.0f, 2.0f)) {
        scene.setLensingStrength(lensing);
    }
    float doppler = scene.getSettings().dopplerIntensity;
    if (ImGui::SliderFloat("Relativistic beaming", &doppler, 0.0f, 1.5f)) {
        scene.setDopplerIntensity(doppler);
    }
    float temporalSmoothing = scene.getSettings().temporalSmoothing;
    if (ImGui::SliderFloat("Temporal smoothing", &temporalSmoothing, 0.0f, 0.94f)) {
        scene.setTemporalSmoothing(temporalSmoothing);
    }
    float bloom = scene.getSettings().bloomStrength;
    if (ImGui::SliderFloat("Bloom", &bloom, 0.0f, 2.0f)) {
        scene.setBloomStrength(bloom);
    }
    float mass = scene.getSettings().blackHoleMass;
    if (ImGui::SliderFloat("Mass", &mass, 10.0f, 100.0f)) {
        scene.setBlackHoleMass(mass);
    }
    float gravity = scene.getSettings().gravityStrength;
    if (ImGui::SliderFloat("Gravity", &gravity, 0.1f, 2.0f)) {
        scene.setGravityStrength(gravity);
    }
    float timeScale = scene.getTimeScale();
    if (ImGui::SliderFloat("Time scale", &timeScale, 0.0f, 4.0f)) {
        scene.setTimeScale(timeScale);
    }
    bool paused = scene.isPaused();
    if (ImGui::Checkbox("Pause simulation", &paused)) scene.setPaused(paused);
    if (ImGui::Button("Reset camera and particle disk")) {
        scene.getCamera().reset();
        scene.resetParticles();
    }
    ImGui::Separator();
    ImGui::Text("LMB orbit  |  RMB pan  |  wheel zoom");
    ImGui::Text("R reset  |  Space pause  |  F1 diagnostics");
    ImGui::End();
    rlImGuiEnd();
}
