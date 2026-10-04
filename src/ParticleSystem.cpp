#include "ParticleSystem.h"
#include "utils/Random.h"
#include "utils/Math.h"
#include "utils/Logger.h"
#include <cmath>
#include <algorithm>

void Particle::update(float deltaTime) {
    velocity.x += acceleration.x * deltaTime;
    velocity.y += acceleration.y * deltaTime;
    velocity.z += acceleration.z * deltaTime;

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z += velocity.z * deltaTime;

    acceleration = {0, 0, 0};
    age += deltaTime;
}

Color Particle::getColor() const {
    const float heat = Math::clamp(temperature, 0.0f, 1.0f);
    const auto brightness = static_cast<unsigned char>(58.0f + heat * 88.0f);
    return {brightness, brightness, brightness, 255};
}

void Gravity::apply(Particle& particle) const {
    float dx = -particle.position.x;
    float dy = -particle.position.y;
    float dz = -particle.position.z;

    float distSq = dx*dx + dy*dy + dz*dz;
    float dist = std::sqrt(distSq);
    if (dist < 1.0f) dist = 1.0f;

    const float force = strength * mass * particle.mass / distSq;
    particle.acceleration.x += (dx / dist) * force / particle.mass;
    particle.acceleration.y += (dy / dist) * force / particle.mass;
    particle.acceleration.z += (dz / dist) * force / particle.mass;

    particle.temperature = 1.0f - Math::clamp(
        dist / (eventHorizonRadius * 10.0f), 0.0f, 1.0f);
}

bool Gravity::isAbsorbed(const Particle& particle) const {
    const float dist = Math::distance3D(
        0, 0, 0, particle.position.x, particle.position.y, particle.position.z);
    return dist < eventHorizonRadius;
}

ParticleSystem::ParticleSystem(const Settings& settings)
    : gravity(settings.blackHoleMass, settings.eventHorizonRadius, settings.gravityStrength)
    , settings(settings)
    , aliveCount(0)
{
}

void ParticleSystem::initialize() {
    if (settings.useGPUCompute && gpuParticles.initialize(settings)) {
        aliveCount = settings.particleCount;
        return;
    }

    // Keep the fallback bounded when the GPU path was requested but isn't available.
    const int cpuCount = settings.useGPUCompute
        ? std::min(settings.particleCount, settings.cpuParticleLimit)
        : settings.particleCount;
    particles.resize(static_cast<std::size_t>(cpuCount));
    for (auto& particle : particles) {
        respawnParticle(particle);
    }
    aliveCount = particles.size();
    if (settings.useGPUCompute) {
        Logger::warning("GPU compute is unavailable; CPU fallback is limited to " +
                        std::to_string(cpuCount) + " particles");
    }
}

void ParticleSystem::respawnParticle(Particle& particle) {
    // Spawn in a ring around the black hole
    float angle = Random::range(0.0f, Math::TWO_PI);
    const float minRadiusSq = settings.spawnRadiusMin * settings.spawnRadiusMin;
    const float maxRadiusSq = settings.spawnRadiusMax * settings.spawnRadiusMax;
    const float radius = std::sqrt(Random::range(minRadiusSq, maxRadiusSq));

    particle.position.x = std::cos(angle) * radius;
    particle.position.z = std::sin(angle) * radius;
    particle.position.y = Random::range(-settings.diskThickness, settings.diskThickness);

    // Circular-orbit speed gives new particles a tangential starting velocity.
    float speed = std::sqrt(settings.gravityStrength * settings.blackHoleMass / radius);
    particle.velocity.x = -std::sin(angle) * speed;
    particle.velocity.z = std::cos(angle) * speed;
    particle.velocity.y = Random::range(-0.01f, 0.01f);

    particle.acceleration = {0, 0, 0};
    particle.mass = Random::range(0.5f, 2.0f);
    particle.size = Random::range(settings.particleSizeMin, settings.particleSizeMax);
    particle.temperature = 0.0f;
    particle.age = 0.0f;
    particle.alive = true;
}

void ParticleSystem::update(float deltaTime, const Camera3D& camera) {
    if (gpuParticles.isActive()) {
        gpuParticles.update(deltaTime, camera, settings);
        aliveCount = settings.particleCount;
        return;
    }

    aliveCount = 0;
    const float horizonRadiusSq = settings.eventHorizonRadius * settings.eventHorizonRadius;
    const float gravityScale = settings.gravityStrength * settings.blackHoleMass;

    for (Particle& p : particles) {
        if (!p.alive) {
            respawnParticle(p);
            ++aliveCount;
            continue;
        }

        float distSq = p.position.x * p.position.x +
                       p.position.y * p.position.y +
                       p.position.z * p.position.z;
        if (distSq < 1.0f) distSq = 1.0f;

        // Clamp the force near the singularity; absorbed particles are recycled below.
        const float inverseDistance = 1.0f / std::sqrt(distSq);
        const float distance = distSq * inverseDistance;
        const float accelerationScale = gravityScale * inverseDistance / distSq;

        p.velocity.x -= p.position.x * accelerationScale * deltaTime;
        p.velocity.y -= p.position.y * accelerationScale * deltaTime;
        p.velocity.z -= p.position.z * accelerationScale * deltaTime;

        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.position.z += p.velocity.z * deltaTime;
        p.age += deltaTime;

        const float tempFactor = 1.0f - Math::clamp(
            distance / (settings.eventHorizonRadius * 10.0f), 0.0f, 1.0f);
        p.temperature = tempFactor;

        const float newDistSq = p.position.x * p.position.x +
                                p.position.y * p.position.y +
                                p.position.z * p.position.z;
        if (newDistSq < horizonRadiusSq) {
            p.alive = false;
        } else {
            aliveCount++;
        }
    }
}

void ParticleSystem::reset() {
    if (gpuParticles.isActive()) {
        gpuParticles.reset(settings);
        aliveCount = settings.particleCount;
        return;
    }

    for (auto& particle : particles) respawnParticle(particle);
    aliveCount = static_cast<int>(particles.size());
}

void ParticleSystem::shutdown() {
    gpuParticles.shutdown();
}
