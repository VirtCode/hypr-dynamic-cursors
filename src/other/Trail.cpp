#include "../globals.hpp"
#include "../config/ConfigManager.hpp"
#include "Trail.hpp"

#include <algorithm>
#include <chrono>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/animation/AnimationManager.hpp>
#include <hyprland/src/managers/EventManager.hpp>
#include <hyprutils/animation/AnimationConfig.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/event/EventBus.hpp>

using namespace std::chrono;

float CTrail::timeSinceLastPush(void) {
    return duration_cast<std::chrono::milliseconds>(
            high_resolution_clock::now() - 
            last_push_time).count();
}

bool CTrail::hasChanged(Vector2D pos, SModeResult& given_result) {
    /* returns true if there's a difference in position, scale, rotation,
     * stretch from last point in trail.
     * Or if the trail is empty, so we can calculate differences.*/

    if(samples.empty()) {
        return true;
    }

    bool position_change = samples.back().pos != pos,
         rotation_change = samples.back().result.rotation != given_result.rotation,
         scale_change    = samples.back().result.scale != given_result.scale,
         stretch_change = samples.back().result.stretch.angle != given_result.stretch.angle || samples.back().result.stretch.magnitude != given_result.stretch.magnitude;

    return position_change || rotation_change || scale_change || stretch_change;
}

bool CTrail::push(Vector2D pos, const Pointer::CPointerManager::SCursorImage& img, SModeResult& given_result, const SP<Render::ITexture> highres_tex, Vector2D highHotspotFrac, Vector2D highSize) {
    /* returns true if an element was actually pushed or updated */

    //remove oldest cursor first if its lifetime is exceeded.
    std::erase_if(samples, [](CTrail::TrailPoint& x) { return x.age() >= CONFIG(trailLifetime); });

    //determine if it's the right time to add to trail (according to rate)
    if (timeSinceLastPush() >= CONFIG(trailRate)) {
        
        //only add to trail if position/scale/rotation/stretch have changed.
        if (hasChanged(pos, given_result)) {
            samples.push_back({
                pos, img.bufferTex, 
                highres_tex, highHotspotFrac, highSize,
                given_result, img.size, img.hotspot, img.scale, high_resolution_clock::now()
            });

            last_push_time = high_resolution_clock::now();

            return true;
        }

        //if we've exceeded the max trail length, remove earliest trail point
        if (samples.size() >= CONFIG(trailLength)) {
            samples.pop_front();
        }
    }

    return false;
}

void CTrail::warp(void) {
    samples.clear();
}
