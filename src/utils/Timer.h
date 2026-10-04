#pragma once

#include <chrono>

namespace Timer {

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Duration = std::chrono::duration<float>;

inline TimePoint now() {
    return Clock::now();
}

inline float elapsed(TimePoint start) {
    return std::chrono::duration<float>(Clock::now() - start).count();
}

inline float delta(TimePoint start, TimePoint end) {
    return std::chrono::duration<float>(end - start).count();
}

} // namespace Timer
