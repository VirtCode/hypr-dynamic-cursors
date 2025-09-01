#include "../globals.hpp"
#include "../config/config.hpp"
#include "Shake.hpp"

#include <algorithm>
#include <chrono>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/debug/Log.hpp>
#include <hyprland/src/managers/animation/AnimationManager.hpp>
#include <hyprland/src/managers/EventManager.hpp>
#include <hyprutils/animation/AnimationConfig.hpp>
#include <hyprland/src/render/Renderer.hpp>

CShake::CShake() {
    // the timing and the bezier are quite crucial, as things will break down if they are just changed slighly
    // this is not ideal and should be fixed some time in the future, then it may be made configurable (if it has a substatntial enough effect on behaviour)

    int time = 400;

    // add custom bezier (and readd it after config reload)
    static auto bezier = "dynamic-cursors-magnification";
    g_pAnimationManager->addBezierWithName(bezier, {0.22, 1.0}, {0.36, 1.0});
    static const auto PCALLBACK = HyprlandAPI::registerCallbackDynamic( PHANDLE, "configReloaded", [&](void* self, SCallbackInfo&, std::any data) {
        g_pAnimationManager->addBezierWithName(bezier, {0.22, 1.0}, {0.36, 1.0});
    });

    // wtf is this struct, what is pValues?
    static SP<SAnimationPropertyConfig> properties = makeShared<SAnimationPropertyConfig>();
    properties->internalBezier = bezier;
    properties->internalSpeed = time / 100.f;
    properties->internalEnabled = 1;
    properties->pValues = properties;

    g_pAnimationManager->createAnimation(1.f, zoom, properties, AVARDAMAGE_NONE);
}

double CShake::update(Vector2D pos) {
    static auto* const* PTHRESHOLD = (Hyprlang::FLOAT* const*) getConfig(CONFIG_SHAKE_THRESHOLD);
    static auto* const* PBASE = (Hyprlang::FLOAT* const*) getConfig(CONFIG_SHAKE_BASE);
    static auto* const* PSPEED = (Hyprlang::FLOAT* const*) getConfig(CONFIG_SHAKE_SPEED);
    static auto* const* PINFLUENCE = (Hyprlang::FLOAT* const*) getConfig(CONFIG_SHAKE_INFLUENCE);
    static auto* const* PLIMIT = (Hyprlang::FLOAT* const*) getConfig(CONFIG_SHAKE_LIMIT);
    static auto* const* PTIMEOUT = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE_TIMEOUT);

    static auto* const* PIPC = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE_IPC);

    int max = std::max(1, (int)(g_pHyprRenderer->m_mostHzMonitor->m_refreshRate)); // 1s worth of history, avoiding divide by 0
    samples.resize(max);
    samples_distance.resize(max);
    samples_index = std::min(samples_index, max - 1);

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

    // if diagonal sufficiently large and over threshold
    double amount = (trail / diagonal) - **PTHRESHOLD;
    if (diagonal > 100 && amount > 0) {
        float delta = 1.F / g_pHyprRenderer->m_mostHzMonitor->m_refreshRate;

        float next = this->zoom->goal();

        if (!started) next = **PBASE; // start on base zoom
        next += delta * (**PSPEED + (amount * amount) * **PINFLUENCE); // increase when moving
        if (**PLIMIT > 1) next = std::min(**PLIMIT, next); // limit overall zoom

        *this->zoom = next;
        this->end = steady_clock::now() + milliseconds(**PTIMEOUT);
        started = true;
    } else {
        if (started && end < std::chrono::steady_clock::now()) {
            *this->zoom = 1;
            started = false;
        }
    }

    if (**PIPC) {
        if (started || this->zoom->value() > 1) {
            if (!ipc) {
                g_pEventManager->postEvent(SHyprIPCEvent { IPC_SHAKE_START });
                ipc = true;
            }

            g_pEventManager->postEvent(SHyprIPCEvent { IPC_SHAKE_UPDATE, std::format("{},{},{},{},{}", (int) pos.x, (int) pos.y, trail, diagonal, this->zoom->value()) });
        } else {
            if (ipc) {
                g_pEventManager->postEvent(SHyprIPCEvent { IPC_SHAKE_END });
                ipc = false;
            }
        }
    }

    return this->zoom->value();
}

void CShake::force(std::optional<int> duration, std::optional<float> size) {
    static auto* const* PTIMEOUT = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE_TIMEOUT);
    static auto* const* PBASE = (Hyprlang::FLOAT* const*) getConfig(CONFIG_SHAKE_BASE);

    started = true;
    *this->zoom = size.value_or(**PBASE);
    this->end = steady_clock::now() + milliseconds(duration.value_or(**PTIMEOUT));
}

void CShake::warp(Vector2D old, Vector2D pos) {
    auto delta = pos - old;

    for (auto& sample : samples)
        sample += delta;
}
