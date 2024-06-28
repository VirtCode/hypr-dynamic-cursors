#include "../globals.hpp"
#include "Shake.hpp"
#include <hyprland/src/Compositor.hpp>

double CShake::update(Vector2D pos) {
    static auto* const* PTHRESHOLD = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_SHAKE_THRESHOLD)->getDataStaticPtr();
    static auto* const* PFACTOR = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_SHAKE_FACTOR)->getDataStaticPtr();

    int max = g_pHyprRenderer->m_pMostHzMonitor->refreshRate; // 1s worth of history
    samples.resize(max);
    samples_distance.resize(max);

    int previous = samples_index == 0 ? max - 1 : samples_index - 1;
    samples[samples_index] = Vector2D{pos};
    samples_distance[samples_index] = samples[samples_index].distance(samples[previous]);
    samples_index = (samples_index + 1) % max; // increase for next sample

    // The idea for this algorith was largely inspired by KDE Plasma
    // https://invent.kde.org/plasma/kwin/-/blob/master/src/plugins/shakecursor/shakedetector.cpp

    // calculate total distance travelled
    double trail = 0;
    for (double distance : samples_distance) trail += distance;

    // calculate diagonal of bounding box travelled within
    double left = 1e100, right = 0, bottom = 0, top = 1e100;
    for (Vector2D position : samples) {
        left = std::min(left, position.x);
        right = std::max(right, position.x);
        top = std::min(top, position.y);
        bottom = std::max(bottom, position.y);
    }
    double diagonal = Vector2D{left, top}.distance(Vector2D(right, bottom));

    // discard when the diagonal is small, so we don't have issues with inaccuracies
    if (diagonal < 100) return 1.0;

    return std::max(1.0, ((trail / diagonal) - **PTHRESHOLD) * **PFACTOR);
}
