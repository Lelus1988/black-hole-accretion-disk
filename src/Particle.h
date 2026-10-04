#pragma once

#include <raylib.h>

struct Particle {
    // CPU-only particle state; GPU particles use a separate packed layout.
    Vector3 position;
    Vector3 velocity;
    Vector3 acceleration;
    float mass;
    float size;
    float temperature;
    float age;
    bool alive;

    Particle() : position{0, 0, 0}, velocity{0, 0, 0}, acceleration{0, 0, 0},
                 mass(1.0f), size(1.0f), temperature(0.0f), age(0.0f), alive(true) {}

    void update(float deltaTime);
    Color getColor() const;
};
