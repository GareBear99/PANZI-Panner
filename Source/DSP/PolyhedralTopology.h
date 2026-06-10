#pragma once
#include <array>
#include <cmath>

struct Vec3 { float x, y, z; };

// ── Diamond (Octahedron + 2 equatorial fills) ──────────────────────────
// 6 octahedron vertices + 2 equatorial fills for front-left/right imaging.
// Mirrored upper/lower pyramid: symmetric intensity boundary conditions
// for height panning above and below the listener plane.
static constexpr std::array<Vec3, 8> kDiamondNodes = {{
    { 1.000f,  0.000f,  0.000f },  // 0: Right
    {-1.000f,  0.000f,  0.000f },  // 1: Left
    { 0.000f,  1.000f,  0.000f },  // 2: Front
    { 0.000f, -1.000f,  0.000f },  // 3: Rear
    { 0.000f,  0.000f,  1.000f },  // 4: Top
    { 0.000f,  0.000f, -1.000f },  // 5: Bottom
    { 0.707f,  0.707f,  0.000f },  // 6: Front-Right
    {-0.707f,  0.707f,  0.000f }   // 7: Front-Left
}};

// ── Cube ──────────────────────────────────────────────────────────────
// 8 corners of a unit cube (normalised to unit sphere).
// Uniform corner coverage; slight diagonal aliasing.
static constexpr float kC = 0.5773502692f; // 1/sqrt(3)
static constexpr std::array<Vec3, 8> kCubeNodes = {{
    { kC,  kC,  kC },   // 0: Front-Right-Top
    {-kC,  kC,  kC },   // 1: Front-Left-Top
    { kC, -kC,  kC },   // 2: Rear-Right-Top
    {-kC, -kC,  kC },   // 3: Rear-Left-Top
    { kC,  kC, -kC },   // 4: Front-Right-Bottom
    {-kC,  kC, -kC },   // 5: Front-Left-Bottom
    { kC, -kC, -kC },   // 6: Rear-Right-Bottom
    {-kC, -kC, -kC }    // 7: Rear-Left-Bottom
}};

// ── Cylinder ──────────────────────────────────────────────────────────
// 4 floor nodes + 4 ceiling nodes, azimuth-uniform (45° spacing, offset 22.5°).
// Optimised for height panning with uniform horizontal imaging.
static constexpr std::array<Vec3, 8> kCylinderNodes = {{
    { 1.000f,  0.000f,  0.500f },  // 0: Right-Top
    { 0.000f,  1.000f,  0.500f },  // 1: Front-Top
    {-1.000f,  0.000f,  0.500f },  // 2: Left-Top
    { 0.000f, -1.000f,  0.500f },  // 3: Rear-Top
    { 1.000f,  0.000f, -0.500f },  // 4: Right-Bottom
    { 0.000f,  1.000f, -0.500f },  // 5: Front-Bottom
    {-1.000f,  0.000f, -0.500f },  // 6: Left-Bottom
    { 0.000f, -1.000f, -0.500f }   // 7: Rear-Bottom
}};

// ── Sphere (Icosahedron 8-node subset) ────────────────────────────────
// Selected to maximise minimum solid angle between adjacent nodes.
// Provides most spatially uniform coverage of the unit sphere.
// φ = (1+√5)/2 ≈ 1.6180339887 (golden ratio)
static constexpr float kPhi = 1.6180339887f;
// Pre-normalised: magnitude = sqrt(1 + phi²) ≈ 1.902
static constexpr float kSN = 0.5257311121f; // 1/sqrt(1+phi²)
static constexpr float kSP = 0.8506508084f; // phi/sqrt(1+phi²)
static constexpr std::array<Vec3, 8> kSphereNodes = {{
    {  0.0f,   kSN,  kSP },   // 0
    {  0.0f,  -kSN,  kSP },   // 1
    {  0.0f,   kSN, -kSP },   // 2
    {  0.0f,  -kSN, -kSP },   // 3
    {  kSN,   kSP,  0.0f },   // 4
    { -kSN,   kSP,  0.0f },   // 5
    {  kSN,  -kSP,  0.0f },   // 6
    { -kSN,  -kSP,  0.0f }    // 7
}};

enum class Topology { Diamond = 0, Cube = 1, Cylinder = 2, Sphere = 3 };

inline const Vec3* getTopologyNodes (Topology t) noexcept
{
    switch (t)
    {
        case Topology::Diamond:  return kDiamondNodes.data();
        case Topology::Cube:     return kCubeNodes.data();
        case Topology::Cylinder: return kCylinderNodes.data();
        case Topology::Sphere:   return kSphereNodes.data();
        default:                 return kDiamondNodes.data();
    }
}

inline const char* getTopologyName (Topology t) noexcept
{
    switch (t)
    {
        case Topology::Diamond:  return "Diamond";
        case Topology::Cube:     return "Cube";
        case Topology::Cylinder: return "Cylinder";
        case Topology::Sphere:   return "Sphere";
        default:                 return "Diamond";
    }
}
