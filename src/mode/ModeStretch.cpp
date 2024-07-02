#include "ModeStretch.hpp"
#include "utils.hpp"
#include "../globals.hpp"
#include <hyprland/src/Compositor.hpp>

EModeUpdate CModeStretch::strategy() {
    return TICK;
}

SModeResult CModeStretch::update(Vector2D pos) {
    static auto const* PFUNCTION = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_STRETCH_FUNCTION)->getDataStaticPtr();
    static auto* const* PLIMIT = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_STRETCH_LIMIT)->getDataStaticPtr();

    // create samples array
    int max = g_pHyprRenderer->m_pMostHzMonitor->refreshRate / 10; // 100ms worth of history
    samples.resize(max);

    // capture current sample
    samples[samples_index] = Vector2D{pos};
    int current = samples_index;
    samples_index = (samples_index + 1) % max; // increase for next sample
    int first = samples_index;

    // calculate speed and tilt
    Vector2D speed = (samples[current] - samples[first]) / 0.1;
    double mag = speed.size();

    double angle = -std::atan(speed.x / speed.y) + PI;
    if (speed.y > 0) angle += PI;
    if (mag == 0) angle = 0;

    double scale = activation(*PFUNCTION, **PLIMIT, mag);

    auto result = SModeResult();
    result.stretch.angle = angle;
    // we can't do more scaling than that because of how large our buffer around the cursor shape is
    result.stretch.magnitude = Vector2D{1.0 - scale * 0.5, 1.0 + scale * 1.0};

    return result;
}
