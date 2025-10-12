#include "../globals.hpp"
#include "../config/config.hpp"
#include "EdgeSquash.hpp"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <cmath>

SModeResult CEdgeSquash::update(Vector2D pos) {
    static auto* const* PENABLED = (Hyprlang::INT* const*) getConfig(CONFIG_EDGE_ENABLED);
    static auto* const* PDISTANCE = (Hyprlang::INT* const*) getConfig(CONFIG_EDGE_DISTANCE);
    static auto* const* PSTRENGTH = (Hyprlang::FLOAT* const*) getConfig(CONFIG_EDGE_STRENGTH);
    static auto* const* PCORNER_RADIUS = (Hyprlang::INT* const*) getConfig(CONFIG_EDGE_CORNER_RADIUS);
    static auto* const* PSPRING_STIFFNESS = (Hyprlang::FLOAT* const*) getConfig(CONFIG_EDGE_SPRING_STIFFNESS);
    static auto* const* PSPRING_DAMPING = (Hyprlang::FLOAT* const*) getConfig(CONFIG_EDGE_SPRING_DAMPING);

    auto result = SModeResult();

    if (!**PENABLED)
        return result;

    // Find the monitor containing the cursor
    SP<CMonitor> pMonitor = nullptr;
    for (auto& m : g_pCompositor->m_monitors) {
        if (!m->m_output)
            continue;

        auto box = m->logicalBox();
        if (box.containsPoint(pos)) {
            pMonitor = m;
            break;
        }
    }

    if (!pMonitor)
        return result;

    auto box = pMonitor->logicalBox();
    int distance = **PDISTANCE;
    float strength = **PSTRENGTH;

    // Calculate distance to each edge in pixels
    double distLeft = pos.x - box.x;
    double distRight = (box.x + box.w) - pos.x;
    double distTop = pos.y - box.y;
    double distBottom = (box.y + box.h) - pos.y;

    // Find closest edges - we need this to determine which edge(s) are affecting the cursor
    double minHorizontal = std::min(distLeft, distRight);
    double minVertical = std::min(distTop, distBottom);
    double minDist = std::min(minHorizontal, minVertical);

    // Calculate raw squash factor using cubic easing
    // This determines how much squashing we want before applying spring physics
    double rawSquashFactor = 0.0;
    if (minDist <= distance) {
        // t ranges from 0.0 (at trigger distance) to 1.0 (at edge)
        double t = 1.0 - (minDist / distance);
        // Cubic easing (t³) gives smooth acceleration as cursor approaches edge
        // Linear would feel mechanical, quadratic too sudden, cubic feels natural
        rawSquashFactor = t * t * t * strength;
    }

    // Spring physics for bounce-back effect
    // This simulates a spring connecting the current squash to the target squash
    double springStiffness = **PSPRING_STIFFNESS;
    double springDamping = **PSPRING_DAMPING;

    // Hooke's law: F = k * x
    // Force is proportional to displacement from target (rawSquashFactor - lastSquashFactor)
    // Higher stiffness = faster response to changes
    double springForce = (rawSquashFactor - lastSquashFactor) * springStiffness;
    squashVelocity += springForce;
    // Damping reduces velocity each frame to prevent oscillation
    // Without damping, the cursor would bounce back and forth endlessly
    squashVelocity *= springDamping;

    // Semi-implicit Euler integration: update position using new velocity
    double squashFactor = lastSquashFactor + squashVelocity;
    squashFactor = std::clamp(squashFactor, 0.0, 1.0);

    lastSquashFactor = squashFactor;

    if (squashFactor < 0.01)
        return result;

    // Corner detection: determine if we should blend between edge squash and diagonal squash
    double cornerThreshold = **PCORNER_RADIUS;

    // cornerInfluence ranges from 0.0 (pure edge) to 1.0 (pure corner diagonal)
    // This controls the blend between cardinal angles (0°/90°) and natural diagonal angles
    double cornerInfluence = 0.0;
    if (minHorizontal <= cornerThreshold && minVertical <= cornerThreshold) {
        // Both edges are close enough - calculate corner influence
        // Ratios range from 0.0 (at threshold distance) to 1.0 (at corner)
        double horizontalRatio = 1.0 - (minHorizontal / cornerThreshold);
        double verticalRatio = 1.0 - (minVertical / cornerThreshold);
        // Use minimum so both edges must be close for full corner effect
        // This prevents corners from activating when only one edge is near
        cornerInfluence = std::min(horizontalRatio, verticalRatio);
        // Apply cubic function (x³) for ultra-smooth transition
        // This makes the transition feel more organic and less sudden
        cornerInfluence = cornerInfluence * cornerInfluence * cornerInfluence;
    }

    // Calculate natural diagonal angle based on relative position between edges
    // angleRatio represents the balance between horizontal and vertical edge influence
    // Example: if minHorizontal=30, minVertical=90, then angleRatio=30/(30+90)=0.25 (mostly horizontal)
    double angleRatio = minHorizontal / (minHorizontal + minVertical + 0.001); // add epsilon to avoid division by zero
    // Map angleRatio (0.0 to 1.0) to angle (0° to 90°)
    // angleRatio=0.0 means horizontal edge is closest → 0° (horizontal squash)
    // angleRatio=1.0 means vertical edge is closest → 90° (vertical squash)
    double naturalAngle = angleRatio * PI / 2;

    // Determine which cardinal direction (0° or 90°) we should use when not in a corner
    // This provides the "snap to edge" behavior when far from corners
    double cardinalAngle;
    if (minHorizontal < minVertical) {
        cardinalAngle = PI / 2; // closer to vertical edge → 90° (vertical squash)
    } else {
        cardinalAngle = 0; // closer to horizontal edge → 0° (horizontal squash)
    }

    // Linear interpolation (lerp) between cardinal and natural angle
    // When cornerInfluence = 0: use cardinalAngle (snap to 0° or 90°)
    // When cornerInfluence = 1: use naturalAngle (smooth diagonal from 0° to 90°)
    // Formula: result = A * (1 - t) + B * t, where t is the blend factor
    result.stretch.angle = cardinalAngle * (1.0 - cornerInfluence) + naturalAngle * cornerInfluence;

    // Apply squash/stretch to cursor
    // x: stretch along the edge (3.5x at full squash)
    // y: compress perpendicular to edge (5% of original at full squash)
    result.stretch.magnitude = Vector2D{1.0 + squashFactor * 3.5, 1.0 - squashFactor * 0.95};

    lastPos = pos;
    return result;
}

void CEdgeSquash::warp(Vector2D old, Vector2D pos) {
    lastPos = pos;
}
