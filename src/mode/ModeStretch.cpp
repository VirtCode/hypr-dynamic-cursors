#include "ModeStretch.hpp"
#include "utils.hpp"
#include "../config/config.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Renderer.hpp>

EModeUpdate CModeStretch::strategy() {
    return TICK;
}

SModeResult CModeStretch::update(Vector2D pos) {
    static auto const* PFUNCTION = (Hyprlang::STRING const*) getConfig(CONFIG_STRETCH_FUNCTION);
    static auto* const* PLIMIT = (Hyprlang::INT* const*) getConfig(CONFIG_STRETCH_LIMIT);
    auto function = g_pShapeRuleHandler->getStringOr(CONFIG_STRETCH_FUNCTION, *PFUNCTION);
    auto limit = g_pShapeRuleHandler->getIntOr(CONFIG_STRETCH_LIMIT, **PLIMIT);

    // create samples array
    int max = std::max(1, (int)(g_pHyprRenderer->m_pMostHzMonitor->refreshRate / 10)); // 100ms worth of history, avoiding divide by 0
    samples.resize(max, pos);

    // capture current sample
    samples[samples_index] = pos;
    int current = samples_index;
    samples_index = (samples_index + 1) % max; // increase for next sample
    int first = samples_index;

    // calculate speed and tilt
    Vector2D speed = (samples[current] - samples[first]) / 0.1;
    double mag = speed.size();

    double angle = -std::atan(speed.x / speed.y) + PI;
    if (speed.y > 0) angle += PI;
    if (mag == 0) angle = 0;

    double scale = activation(function, limit, mag);

    auto result = SModeResult();
    result.stretch.angle = angle;
    // we can't do more scaling than that because of how large our buffer around the cursor shape is
    result.stretch.magnitude = Vector2D{1.0 - scale * 0.5, 1.0 + scale * 1.0};

    return result;
}

void CModeStretch::warp(Vector2D old, Vector2D pos) {
    auto delta = pos - old;

    for (auto& sample : samples)
        sample += delta;
}

void CModeStretch::reset() {
    samples.clear();
    samples_index = 0;
}
