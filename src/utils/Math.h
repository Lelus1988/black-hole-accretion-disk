#pragma once

#include <cmath>

namespace Math {

constexpr float MATH_PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * MATH_PI;
constexpr float DEG_TO_RAD = MATH_PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / MATH_PI;

inline float toRadians(float degrees) {
    return degrees * DEG_TO_RAD;
}

inline float toDegrees(float radians) {
    return radians * RAD_TO_DEG;
}

inline float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

inline float distance3D(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace Math
