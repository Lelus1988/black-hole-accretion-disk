#include "App.h"
#include "utils/Logger.h"

int main() {
    Logger::info("Black Hole - High Performance Particle Simulation");

    App app;

    if (!app.initialize()) {
        Logger::error("Failed to initialize application");
        return 1;
    }

    app.run();

    return 0;
}
