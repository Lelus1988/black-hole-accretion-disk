#pragma once

#include <random>
#include <cstdint>

namespace Random {

inline std::mt19937& engine() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

inline float range(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(engine());
}

inline int rangeInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(engine());
}

inline float unit() {
    return range(0.0f, 1.0f);
}

inline bool chance(float probability) {
    return unit() < probability;
}

inline float gaussian(float mean, float stddev) {
    std::normal_distribution<float> dist(mean, stddev);
    return dist(engine());
}

} // namespace Random
