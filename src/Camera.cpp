#include "Camera.h"
#include "utils/Math.h"

OrbitCamera::OrbitCamera(const Settings& settings)
    : target{0.0f, 0.0f, 0.0f}
    , distance(settings.cameraDistance)
    , yaw(0.0f)
    , pitch(Math::MATH_PI * 0.5f - 0.15f)
    , settings(settings)
{
    raycam.position = {0, 100, distance};
    raycam.target = target;
    raycam.up = {0, 1, 0};
    raycam.fovy = 45.0f;
    raycam.projection = CAMERA_PERSPECTIVE;
}

void OrbitCamera::reset() {
    target = {0.0f, 0.0f, 0.0f};
    distance = settings.cameraDistance;
    yaw = 0.0f;
    pitch = Math::MATH_PI * 0.5f - 0.15f;
}

void OrbitCamera::handleInput() {
    const Vector2 mouseDelta = GetMouseDelta();
    if (!mouseCaptured && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float dx = mouseDelta.x * settings.cameraRotationSpeed;
        float dy = mouseDelta.y * settings.cameraRotationSpeed;

        yaw -= dx;
        pitch -= dy;

        // Clamp pitch to avoid flipping
        pitch = Math::clamp(pitch, 0.1f, Math::MATH_PI - 0.1f);
    }

    if (!mouseCaptured && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        // Pan in the camera's horizontal plane instead of moving along world X/Z.
        const float panScale = distance * settings.cameraRotationSpeed;
        const float rightX = std::cos(yaw);
        const float rightZ = -std::sin(yaw);
        target.x -= mouseDelta.x * panScale * rightX;
        target.z -= mouseDelta.x * panScale * rightZ;
        target.y += mouseDelta.y * panScale;
    }

    // Zoom with scroll wheel
    float wheel = mouseCaptured ? 0.0f : GetMouseWheelMove();
    if (wheel != 0.0f) {
        distance -= wheel * settings.cameraZoomSpeed;
        distance = Math::clamp(distance, 6.0f, 250.0f);
    }
}

void OrbitCamera::update() {
    raycam.target = target;
    // Convert the orbit angles back to a position around the target.
    raycam.position.x = target.x + distance * std::sin(pitch) * std::sin(yaw);
    raycam.position.y = target.y + distance * std::cos(pitch);
    raycam.position.z = target.z + distance * std::sin(pitch) * std::cos(yaw);
}
