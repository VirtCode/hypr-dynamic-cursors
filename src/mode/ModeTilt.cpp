#include "ModeTilt.hpp"
#include "../globals.hpp"
#include <hyprland/src/Compositor.hpp>

double function(double speed) {
    static auto const* PFUNCTION = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_FUNCTION)->getDataStaticPtr();
    static auto* const* PMASS = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_MASS)->getDataStaticPtr();
    double mass = **PMASS;

    double result = 0;
    if (!strcmp(*PFUNCTION, "linear")) {

        result = speed / **PMASS;

    } else if (!strcmp(*PFUNCTION, "quadratic")) {


        // (1 / m²) * x², is a quadratic function which will reach 1 at m
        result = (1.0 / (mass * mass)) * (speed * speed);
        result *= (speed > 0 ? 1 : -1);

    } else if (!strcmp(*PFUNCTION, "negative_quadratic")) {

        float x = std::abs(speed);
        // (-1 / m²) * (x - m)² + 1, is a quadratic function with the inverse curvature which will reach 1 at m
        result = (-1.0 / (mass * mass)) * ((x - mass) * (x - mass)) + 1;
        if (x > mass) result = 1; // need to clamp manually, as the function would decrease again

        result *= (speed > 0 ? 1 : -1);
    } else
        Debug::log(WARN, "[dynamic-cursors] unknown air function specified");

    return std::clamp(result, -1.0, 1.0);
}

EModeUpdate CModeTilt::strategy() {
    return TICK;
}

double CModeTilt::update(Vector2D pos) {

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

    return function(speed) * (PI / 3); // 120° in both directions
}
