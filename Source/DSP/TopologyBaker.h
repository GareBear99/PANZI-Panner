#pragma once
#include "BakedCoefficients.h"
#include "PolyhedralTopology.h"
#include <cmath>
#include <algorithm>

// TopologyBaker computes the full BakedCoefficients set for a given
// topology, room scale, and source position.
//
// Must be called on a background thread — never from processBlock().
// Cost: ~1 µs for 8 channels (floating-point sqrt/exp/atan2/asin × 8).
//
// The 5D vector field dimensions map to output fields as follows:
//   Ix, Iy, Iz → speakerPhi, speakerTheta (directional intensity)
//   P          → gainScalar  (inverse-distance pressure law)
//   φ          → delaySamples + svfCutoffHz (time-of-flight + air absorption)
class TopologyBaker
{
public:
    static BakedCoefficients bake (Topology    topo,
                                   float       roomScaleM,
                                   float       sourceX,
                                   float       sourceY,
                                   float       sourceZ,
                                   float       sampleRate) noexcept
    {
        BakedCoefficients c;
        c.numChannels = 8;
        c.roomScaleM  = roomScaleM;
        c.sampleRate  = sampleRate;
        c.topology    = static_cast<int>(topo);

        const Vec3* nodes = getTopologyNodes(topo);

        for (int n = 0; n < 8; ++n)
        {
            // Scale unit-sphere node to physical room dimensions
            const float nx = nodes[n].x * roomScaleM;
            const float ny = nodes[n].y * roomScaleM;
            const float nz = nodes[n].z * roomScaleM;

            // Vector from source to speaker n
            const float dx = nx - sourceX;
            const float dy = ny - sourceY;
            const float dz = nz - sourceZ;

            // Distance (clamped to avoid near-field gain explosion)
            const float dist = std::max(kMinDistance,
                                        std::sqrt(dx*dx + dy*dy + dz*dz));

            // 4D spacetime: time-of-flight delay
            c.delaySamples[n] = (dist / kSpeedOfSound) * sampleRate;

            // 5D P: inverse-distance pressure gain
            c.gainScalar[n] = 1.0f / dist;

            // 5D φ: air absorption (ISO 9613-1 approximation)
            // fc = fc_max × exp(−k_air × dist)
            c.svfCutoffHz[n] = kFcMax * std::exp(-kKAir * dist);

            // Ix, Iy, Iz: speaker direction in spherical coords
            c.speakerPhi[n]   = std::atan2(dy, dx);
            c.speakerTheta[n] = std::asin(std::clamp(dz / dist, -1.0f, 1.0f));
        }

        return c;
    }
};
