#include "ModeTilt.hpp"
#include "utils.hpp"
#include "../config/config.hpp"
#include <hyprland/src/Compositor.hpp>

EModeUpdate CModeTilt::strategy() {
    return TICK;
}

SModeResult CModeTilt::update(Vector2D pos) {
    static auto const* PFUNCTION = (Hyprlang::STRING const*) getConfig(CONFIG_TILT_FUNCTION);
    static auto* const* PLIMIT = (Hyprlang::INT* const*) getConfig(CONFIG_TILT_LIMIT);
    auto function = g_pShapeRuleHandler->getStringOr(CONFIG_TILT_FUNCTION, *PFUNCTION);
    auto limit = g_pShapeRuleHandler->getIntOr(CONFIG_TILT_LIMIT, **PLIMIT);

    // create samples array
    int max = g_pHyprRenderer->m_pMostHzMonitor->refreshRate / 10; // 100ms worth of history
    samples.resize(max);

    // capture current sample
    samples[samples_index] = Vector2D{pos};
    int current = samples_index;
    samples_index = (samples_index + 1) % max; // increase for next sample
    int first = samples_index;

    // calculate speed and tilt
    double speed = (samples[current].x - samples[first].x) / 0.1;

    auto result = SModeResult();
    result.rotation = activation(function, limit, speed) * (PI / 3); // 120° in both directions
    return result;
}
