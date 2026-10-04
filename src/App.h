#pragma once

#include "Scene.h"
#include "Settings.h"
#include "utils/Timer.h"

class App {
public:
    App();
    ~App();

    bool initialize();
    void run();
    void shutdown();

private:
    void handleInput();
    void renderControlPanel();
    void renderDebugOverlay();

    Settings settings;
    Scene scene;
    bool running;
    bool showDebug;
    bool showControls;
    Timer::TimePoint lastFrameTime;
};
