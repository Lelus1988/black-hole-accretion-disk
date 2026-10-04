#include "Scene.h"
#include "utils/Logger.h"
#include <numeric>

Scene::Scene(const Settings& settings)
    : settings(settings)
    , particleSystem(settings)
    , camera(settings)
    , renderer(settings)
    , paused(false)
{
}

void Scene::initialize() {
    Logger::info("Initializing Scene...");

    particleSystem.initialize();
    renderer.initialize();

    // Initialize performance monitoring
    perfMonitor.beginFrame();

    Logger::info("Scene initialization complete");
}

void Scene::update(float deltaTime) {
    perfMonitor.beginFrame();

    camera.handleInput();
    camera.update();

    if (paused) {
        // Keep camera input and frame timing active while only the simulation is paused.
        perfMonitor.endFrame();
        return;
    }

    perfMonitor.beginSimulation();

    float scaledDelta = deltaTime * settings.timeScale;
    particleSystem.update(scaledDelta, camera.getRaylibCamera());

    perfMonitor.endSimulation();
}

void Scene::render() {
    perfMonitor.beginRender();
    renderer.render(particleSystem, camera);

    perfMonitor.endRender();
    perfMonitor.endFrame();
}

void Scene::shutdown() {
    particleSystem.shutdown();
    renderer.shutdown();
}

PerformanceMonitor::PerformanceMonitor()
    : fps(0.0f)
    , frameTime(0.0f)
    , simulationTime(0.0f)
    , renderTime(0.0f)
    , frameCount(0)
{
    frameTimes.reserve(MAX_FRAME_SAMPLES);
}

void PerformanceMonitor::beginFrame() {
    frameStart = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::endFrame() {
    frameEnd = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration<float, std::milli>(frameEnd - frameStart);
    frameTime = duration.count();

    frameTimes.push_back(frameTime);
    if (frameTimes.size() > MAX_FRAME_SAMPLES) {
        frameTimes.erase(frameTimes.begin());
    }

    frameCount++;
    calculateFPS();
}

void PerformanceMonitor::beginSimulation() {
    simStart = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::endSimulation() {
    simEnd = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration<float, std::milli>(simEnd - simStart);
    simulationTime = duration.count();
}

void PerformanceMonitor::beginRender() {
    renderStart = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::endRender() {
    renderEnd = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration<float, std::milli>(renderEnd - renderStart);
    renderTime = duration.count();
}

void PerformanceMonitor::calculateFPS() {
    if (frameTimes.empty()) {
        fps = 0.0f;
        return;
    }

    const float avgFrameTime =
        std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
    fps = avgFrameTime > 0.0f ? 1000.0f / avgFrameTime : 0.0f;
}

void PerformanceMonitor::update(float deltaTime) {
    (void)deltaTime;
}

std::string PerformanceMonitor::getStatsString() const {
    return "FPS: " + std::to_string(static_cast<int>(fps)) +
           " | Frame: " + std::to_string(frameTime).substr(0, 4) + "ms" +
           " | Sim: " + std::to_string(simulationTime).substr(0, 4) + "ms" +
           " | Render: " + std::to_string(renderTime).substr(0, 4) + "ms";
}
