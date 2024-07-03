#include "../globals.hpp"
#include "../config/config.hpp"
#include "src/managers/EventManager.hpp"
#include "Shake.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/debug/Log.hpp>

double CShake::update(Vector2D pos) {
    static auto* const* PTHRESHOLD = (Hyprlang::FLOAT* const*) getConfig(CONFIG_SHAKE_THRESHOLD);
    static auto* const* PFACTOR = (Hyprlang::FLOAT* const*) getConfig(CONFIG_SHAKE_FACTOR);
    static auto* const* PIPC = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE_IPC);

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

    // discard when the diagonal is small, return so we don't have issues with inaccuracies
    if (diagonal < 100) return 1.0;

    double zoom = ((trail / diagonal) - **PTHRESHOLD);

    if (**PIPC) {
        if (zoom > 1) {
            if (!ipc) {
                g_pEventManager->postEvent(SHyprIPCEvent { IPC_SHAKE_START });
                ipc = true;
            }

            g_pEventManager->postEvent(SHyprIPCEvent { IPC_SHAKE_UPDATE, std::format("{},{},{},{}", (int) pos.x, (int) pos.y, trail, diagonal) });
        } else {
            if (ipc) {
                g_pEventManager->postEvent(SHyprIPCEvent { IPC_SHAKE_END });
                ipc = false;
            }
        }
    }

    // fix jitter by allowing the diagonal to only grow, until we are below the threshold again
    if (zoom > 0) { // larger than 0 because of factor
        if (diagonal > this->diagonal)
            this->diagonal = diagonal;

        zoom = ((trail / this->diagonal) - **PTHRESHOLD);
    } else this->diagonal = 0;

    // we want ipc to work with factor = 0, so we use it here
    return std::max(1.0, zoom * **PFACTOR);
}
