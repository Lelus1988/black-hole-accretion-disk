#pragma once

#include <string>
#include <vector>
#include <chrono>

class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor() = default;

    void beginFrame();
    void endFrame();
    void beginSimulation();
    void endSimulation();
    void beginRender();
    void endRender();
    void update(float deltaTime);

    float getFPS() const { return fps; }
    float getFrameTime() const { return frameTime; }
    float getSimulationTime() const { return simulationTime; }
    float getRenderTime() const { return renderTime; }

    std::string getStatsString() const;

private:
    void calculateFPS();

    std::chrono::high_resolution_clock::time_point frameStart;
    std::chrono::high_resolution_clock::time_point frameEnd;
    std::chrono::high_resolution_clock::time_point simStart;
    std::chrono::high_resolution_clock::time_point simEnd;
    std::chrono::high_resolution_clock::time_point renderStart;
    std::chrono::high_resolution_clock::time_point renderEnd;

    float fps;
    float frameTime;
    float simulationTime;
    float renderTime;

    std::vector<float> frameTimes;
    // Averaging a short window avoids FPS readings jumping every frame.
    static constexpr int MAX_FRAME_SAMPLES = 60;
    int frameCount;
};
