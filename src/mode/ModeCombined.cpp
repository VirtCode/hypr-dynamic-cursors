#include "ModeCombined.hpp"
#include "utils.hpp"
#include "../config/config.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <cmath>

EModeUpdate CModeCombined::strategy() {
    return MOVE;
}

SModeResult CModeCombined::update(Vector2D pos) {
    // cfg values
    // rotate
    static auto* const* PROTATE_LENGTH = (Hyprlang::INT* const*) getConfig(CONFIG_ROTATE_LENGTH);
    static auto* const* PROTATE_OFFSET = (Hyprlang::FLOAT* const*) getConfig(CONFIG_ROTATE_OFFSET);
    auto rotateLength = g_pShapeRuleHandler->getIntOr(CONFIG_ROTATE_LENGTH, **PROTATE_LENGTH);
    auto rotateOffset = g_pShapeRuleHandler->getFloatOr(CONFIG_ROTATE_OFFSET, **PROTATE_OFFSET);

    // tilt
    static auto const* PTILT_FUNCTION = (Hyprlang::STRING const*) getConfig(CONFIG_TILT_FUNCTION);
    static auto* const* PTILT_LIMIT = (Hyprlang::INT* const*) getConfig(CONFIG_TILT_LIMIT);
    static auto* const* PTILT_WINDOW = (Hyprlang::INT* const*) getConfig(CONFIG_TILT_WINDOW);
    auto tiltFunction = g_pShapeRuleHandler->getStringOr(CONFIG_TILT_FUNCTION, *PTILT_FUNCTION);
    auto tiltLimit = g_pShapeRuleHandler->getIntOr(CONFIG_TILT_LIMIT, **PTILT_LIMIT);
    auto tiltWindow = g_pShapeRuleHandler->getIntOr(CONFIG_TILT_WINDOW, **PTILT_WINDOW);

    // stretch
    static auto const* PSTRETCH_FUNCTION = (Hyprlang::STRING const*) getConfig(CONFIG_STRETCH_FUNCTION);
    static auto* const* PSTRETCH_LIMIT = (Hyprlang::INT* const*) getConfig(CONFIG_STRETCH_LIMIT);
    static auto* const* PSTRETCH_WINDOW = (Hyprlang::INT* const*) getConfig(CONFIG_STRETCH_WINDOW);
    auto stretchFunction = g_pShapeRuleHandler->getStringOr(CONFIG_STRETCH_FUNCTION, *PSTRETCH_FUNCTION);
    auto stretchLimit = g_pShapeRuleHandler->getIntOr(CONFIG_STRETCH_LIMIT, **PSTRETCH_LIMIT);
    auto stretchWindow = g_pShapeRuleHandler->getIntOr(CONFIG_STRETCH_WINDOW, **PSTRETCH_WINDOW);

    // samples setup
    // use the larger window for sample buffer (covers both tilt and stretch needs)
    int windowMs = std::max(tiltWindow, stretchWindow);
    int max = std::max(1, (int)(g_pHyprRenderer->m_mostHzMonitor->m_refreshRate / 1000 * windowMs));
    samples.resize(max, pos);
    samples_index = std::min(samples_index, max - 1);

    samples[samples_index] = pos;
    int current = samples_index;
    samples_index = (samples_index + 1) % max;
    int first = samples_index;

    // rotate calc
    // simulated stick being dragged
    if (rotateEnd.y == 0 && rotateEnd.x == 0) {
        rotateEnd.x = pos.x;
        rotateEnd.y = pos.y + rotateLength;
    }

    // translate to origin
    rotateEnd.x -= pos.x;
    rotateEnd.y -= pos.y;

    // normalize
    double size = rotateEnd.size();
    if (size == 0) size = 1;
    rotateEnd.x /= size;
    rotateEnd.y /= size;

    // scale to length
    rotateEnd.x *= rotateLength;
    rotateEnd.y *= rotateLength;

    // calculate rotation angle
    double rotateAngle = -std::atan(rotateEnd.x / rotateEnd.y);
    if (rotateEnd.y > 0) rotateAngle += PI;
    rotateAngle += PI;
    rotateAngle += rotateOffset * ((2 * PI) / 360);
    if (rotateEnd.y == 0) rotateAngle = 0;

    // translate back
    rotateEnd.x += pos.x;
    rotateEnd.y += pos.y;

    // tilt calc
    // tilt based on horizontal velocity
    double tiltSpeed = (samples[current].x - samples[first].x) / tiltWindow * 1000;
    double tiltAngle = activation(tiltFunction, tiltLimit, tiltSpeed) * (PI / 3);

    // stretch calc
    // stretch based on overall velocity
    Vector2D velocity = (samples[current] - samples[first]) / stretchWindow * 1000;
    double mag = velocity.size();
    double stretchAngle = -std::atan(velocity.x / velocity.y) + PI;
    if (velocity.y > 0) stretchAngle += PI;
    if (mag == 0) stretchAngle = 0;
    double stretchScale = activation(stretchFunction, stretchLimit, mag);

    // combine all stuff
    auto result = SModeResult();

    // rotation: combine rotate + tilt
    // rotate gives base direction, tilt adds the "air drag" wobble on top
    result.rotation = rotateAngle + tiltAngle;

    // stretch: use stretch mode's calculations directly
    result.stretch.angle = stretchAngle;
    result.stretch.magnitude = Vector2D{1.0 - stretchScale * 0.5, 1.0 + stretchScale * 1.0};

    // scale: leave at 1 (none of the original modes touch this)
    result.scale = 1.0;

    return result;
}

void CModeCombined::warp(Vector2D old, Vector2D pos) {
    auto delta = pos - old;

    // warp rotate end point
    rotateEnd += delta;

    // warp all samples
    for (auto& sample : samples)
        sample += delta;
}

void CModeCombined::reset() {
    rotateEnd = {0, 0};
    samples.clear();
    samples_index = 0;
}