#pragma once

#include "Particle.h"

struct Gravity {
    // The CPU fallback uses Newtonian gravity; the GPU path mirrors it in the compute shader.
    float mass;
    float eventHorizonRadius;
    float strength;

    Gravity(float m = 1000.0f, float radius = 50.0f, float str = 5000.0f)
        : mass(m), eventHorizonRadius(radius), strength(str) {}

    void apply(Particle& particle) const;
    bool isAbsorbed(const Particle& particle) const;
};
