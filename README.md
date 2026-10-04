# Black Hole — GPU Accretion Disk Simulation

A real-time 3D black-hole and accretion-disk simulation. On OpenGL 4.3 hardware, a compute shader simulates and renders 1,000,000 particles directly from an SSBO; a capped CPU fallback is used on older contexts.

A visual showcase project focused on rendering performance, particle physics, and cinematic atmosphere, with an ImGui panel for live tuning.

---

## Preview

*Add a screenshot or GIF here*

---

## What This Project Is

**Black Hole** is a from-scratch high-performance rendering project that simulates a black hole with an accretion disk, massive particle counts, and cinematic lighting.

The goal is a fluid-looking, interactive simulation with GPU-resident particle state and a compact project structure.

---

## Features

- Real-time 3D black hole visualization
- **1,000,000 GPU particles** with SSBO compute and recycling
- Velocity-Verlet gravity integration and Keplerian spawn orbits
- Additive, temperature-colored accretion disk
- Thin, near-edge-on default camera angle for a broad, clearly visible disk
- Event-horizon absorption and particle respawning
- Screen-space gravitational lensing that bends both the background and disk density around the black-hole shadow
- Doppler beaming, GPU-accumulated plasma density and velocity-aligned streaks
- Soft bloom, ACES-style tone mapping and history-clamped temporal smoothing
- Frustum-aware lower-frequency integration for distant particles outside the camera view
- Free camera movement with orbital controls
- Zoom, orbit, pan, and reset controls
- Background star field (1,000 stars)
- Interactive on-screen controls for lensing, mass, and gravity
- Dear ImGui control panel for simulation speed, pause, and reset
- CPU fallback capped at 50,000 particles if OpenGL 4.3 is unavailable
- Clean code structure with separation of concerns
- Cross-platform build setup via CMake

---

## Performance

Particle physics and plasma accumulation stay on the GPU; no per-frame particle readback or allocation is used. The default million-particle SSBO uses about 48 MB; increasing the particle count also increases compute and rendering work per frame, so 10–20 million particles are not assumed to run at 100+ FPS. Lensing is a screen-space deflection approximation, not full 3D general-relativistic ray tracing. Bloom and temporal smoothing use raylib render targets rather than floating-point HDR buffers, and DLSS is not currently integrated. Actual frame rate depends on the GPU, resolution, and driver.

---

## Visual Goals

The project aims for:
- Deep black contrast
- Glowing accretion disk
- Massive particle density
- Smooth camera movement
- Cinematic lighting
- A strong science-fiction look

The focus is on making the scene feel large, intense, and visually believable while maintaining high performance.

---

## Tech Stack

- **C++17**
- **raylib 6.0** built with an OpenGL 4.3 backend for windowing and rendering
- **Dear ImGui** with the raylib integration for live simulation controls
- **CMake 3.16+** for build system
- Compiler optimizations for maximum performance

---

## Controls

| Control | Action |
|---------|--------|
| Mouse Left (drag) | Orbit camera around black hole |
| Mouse Right (drag) | Pan camera |
| Mouse Wheel | Zoom in/out |
| Space | Pause / resume simulation |
| R | Reset camera and particle distribution |
| + / = | Increase simulation speed |
| - / _ | Decrease simulation speed |
| F1 | Toggle debug overlay |
| F2 | Toggle controls |
| F12 | Take screenshot (saved to `assets/screenshots/`) |

---

## Building

### Prerequisites

- C++17 compatible compiler (GCC, Clang, or MSVC)
- CMake 3.16 or higher
- raylib 6.0 (automatically downloaded and built with OpenGL 4.3 by default)

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd physicbh

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run
./bin/BlackHole
```

### Windows

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
.\bin\Release\BlackHole.exe
```

CMake builds the bundled raylib with OpenGL 4.3. This first configure needs internet access. To use an installed raylib instead, configure with `-DBLACKHOLE_USE_SYSTEM_RAYLIB=ON`; that library must be version 6.0+ and built for OpenGL 4.3.

### Linux/macOS

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
./bin/BlackHole
```

---

## Project Structure

```
physicbh/
├─ assets/                 # Fonts, textures, screenshots
├─ shaders/                # Runtime GLSL shaders
├─ src/
│  ├─ utils/
│  └─ ...                  # App, camera, scene, renderer, simulation
├─ .gitignore
├─ CMakeLists.txt
├─ LICENSE
└─ README.md
```

---

## Architecture

The project is organized into clear layers:

### Layer 1: App (`App.cpp/h`)
- Program startup and window creation
- Main loop management
- Input handling coordination
- Shutdown

### Layer 2: Simulation (`ParticleSystem.cpp/h`, `Gravity.cpp/h`, `Particle.cpp/h`)
- Particle updates and physics
- Gravity calculations
- Collision/absorption logic
- Spawn system

### Layer 3: Rendering (`Renderer.cpp/h`)
- Particle rendering
- Background star field
- Black hole visualization
- Post-processing effects

### Layer 4: Camera (`Camera.cpp/h`)
- Camera matrix management
- Orbital controls
- Zoom and rotation

### Layer 5: Utility (`utils/`)
- Math helpers
- Random number generation
- Timer utilities
- Logging system

---

## Configuration

Simulation parameters can be adjusted in `src/Settings.h`:

```cpp
struct Settings {
    int particleCount = 1000000;
    float blackHoleMass = 50.0f;
    float eventHorizonRadius = 1.5f;
    float gravityStrength = 1.0f;
    float lensingStrength = 0.85f;
};
```

---

## Future Enhancements

Potential additions for future versions:

- [ ] Motion blur for fast-moving particles
- [ ] Particle trails
- [ ] Custom shaders for advanced lighting
- [ ] Multiple black holes
- [ ] Config file support
- [ ] Preset camera angles

---

## License

This project is open source. See LICENSE file for details.

---

## Acknowledgments

Built with:
- [raylib](https://github.com/raysan5/raylib) - A simple and easy-to-use library to enjoy videogames programming
- [CMake](https://cmake.org/) - Cross-platform build system

---

## Contributing

This is a personal showcase project, but suggestions and improvements are welcome through issues and pull requests.
