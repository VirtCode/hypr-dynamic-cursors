#include "ModeTilt.hpp"
#include "utils.hpp"
#include "../config/config.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Renderer.hpp>

EModeUpdate CModeTilt::strategy() {
    return TICK;
}

SModeResult CModeTilt::update(Vector2D pos) {
    static auto const* PFUNCTION = (Hyprlang::STRING const*) getConfig(CONFIG_TILT_FUNCTION);
    static auto* const* PLIMIT = (Hyprlang::INT* const*) getConfig(CONFIG_TILT_LIMIT);
    static auto* const* PWINDOW = (Hyprlang::INT* const*) getConfig(CONFIG_TILT_WINDOW);
    static auto* const* PFULL_TILT = (Hyprlang::INT* const*) getConfig(CONFIG_FULL_TILT);
    auto function = g_pShapeRuleHandler->getStringOr(CONFIG_TILT_FUNCTION, *PFUNCTION);
    auto limit = g_pShapeRuleHandler->getIntOr(CONFIG_TILT_LIMIT, **PLIMIT);
    auto window = g_pShapeRuleHandler->getIntOr(CONFIG_TILT_WINDOW, **PWINDOW);
    auto full_tilt = g_pShapeRuleHandler->getIntOr(CONFIG_FULL_TILT, **PFULL_TILT);

    // create samples array
    int max = std::max(1, (int)(g_pHyprRenderer->m_mostHzMonitor->m_refreshRate / 1000 * window)); // [window]ms worth of history, avoiding divide by 0
    samples.resize(max, pos);
    samples_index = std::min(samples_index, max - 1);

    // capture current sample
    samples[samples_index] = pos;
    int current = samples_index;
    samples_index = (samples_index + 1) % max; // increase for next sample
    int first = samples_index;

    // calculate speed and tilt
    double speed = (samples[current].x - samples[first].x) / window * 1000;

    auto result = SModeResult();
    result.rotation = activation(function, limit, speed) * (PI / (180.0 / full_tilt));
    return result;
}

void CModeTilt::warp(Vector2D old, Vector2D pos) {
    auto delta = pos - old;

    for (auto& sample : samples)
        sample += delta;
}

void CModeTilt::reset() {
    samples.clear();
    samples_index = 0;
}
