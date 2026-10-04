#pragma once

#include <raylib.h>
#include "Settings.h"

class OrbitCamera {
public:
    OrbitCamera(const Settings& settings);
    ~OrbitCamera() = default;

    void update();
    void handleInput();
    void reset();
    void setMouseCaptured(bool captured) { mouseCaptured = captured; }

    Camera3D getRaylibCamera() const { return raycam; }

private:
    Camera3D raycam;
    Vector3 target;
    float distance;
    float yaw;
    float pitch;
    bool mouseCaptured = false;
    const Settings& settings;
};
