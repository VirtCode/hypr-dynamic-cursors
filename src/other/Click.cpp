#include "Click.hpp"
#include "../config/ConfigManager.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace std::chrono;

CClick::CClick() = default;

bool CClick::onButton(const IPointer::SButtonEvent& event) {
    if (event.state == WL_POINTER_BUTTON_STATE_PRESSED) {
        if (!CONFIG(clickEnabled))
            return false;

        int configuredButton = CONFIG(clickButton);
        if (configuredButton > 0 && event.button != static_cast<uint32_t>(configuredButton))
            return false;

        bool wasEmpty          = heldButtons.empty();
        auto [_, wasInserted]  = heldButtons.insert(event.button);
        if (!wasInserted)
            return false;

        if (wasEmpty || phase != PRESSING) {
            auto now          = steady_clock::now();
            pressStartScale   = sampleAt(now);
            targetPressScale  = std::clamp(static_cast<double>(CONFIG(clickScale)), 0.1, 2.0);
            targetOvershoot   = std::clamp(static_cast<double>(CONFIG(clickOvershoot)), 0.5, 2.0);
            pressDurationMs   = std::max<int>(1, CONFIG(clickPressTime));
            releaseDurationMs = std::max<int>(1, CONFIG(clickReleaseTime));
            pressStartTime    = now;
            phase             = PRESSING;
            return true;
        }

        return false;
    }

    if (event.state == WL_POINTER_BUTTON_STATE_RELEASED) {
        if (heldButtons.erase(event.button) == 0)
            return false;

        if (!heldButtons.empty() || phase != PRESSING)
            return false;

        auto now          = steady_clock::now();
        targetOvershoot   = std::clamp(static_cast<double>(CONFIG(clickOvershoot)), 0.5, 2.0);
        releaseDurationMs = std::max<int>(1, CONFIG(clickReleaseTime));

        // Let quick taps finish the depression before springing back
        auto pressEndTime = pressStartTime + milliseconds(pressDurationMs);
        releaseStartTime  = std::max(now, pressEndTime);
        releaseStartScale = targetPressScale;
        phase             = RELEASING;
        return true;
    }

    return false;
}

double CClick::sampleAt(steady_clock::time_point now) const {
    if (phase == IDLE)
        return 1.0;

    if (phase == PRESSING || (phase == RELEASING && now < releaseStartTime)) {
        double elapsedMs = duration<double, std::milli>(now - pressStartTime).count();
        if (elapsedMs >= pressDurationMs)
            return targetPressScale;

        double t    = std::clamp(elapsedMs / pressDurationMs, 0.0, 1.0);
        double inv  = 1.0 - t;
        double ease = 1.0 - (inv * inv * inv);
        return pressStartScale + (targetPressScale - pressStartScale) * ease;
    }

    double elapsedMs = duration<double, std::milli>(now - releaseStartTime).count();
    if (elapsedMs >= releaseDurationMs)
        return 1.0;

    double           t     = std::clamp(elapsedMs / releaseDurationMs, 0.0, 1.0);
    constexpr double SPLIT = 0.4;

    if (t < SPLIT) {
        double u    = t / SPLIT;
        double ease = std::sin(u * (std::numbers::pi / 2.0));
        return releaseStartScale + (targetOvershoot - releaseStartScale) * ease;
    } else {
        double v    = (t - SPLIT) / (1.0 - SPLIT);
        double ease = 0.5 * (1.0 - std::cos(v * std::numbers::pi));
        return targetOvershoot + (1.0 - targetOvershoot) * ease;
    }
}

double CClick::update() {
    if (phase == IDLE)
        return 1.0;

    auto now = steady_clock::now();
    if (phase == RELEASING && now >= releaseStartTime + milliseconds(releaseDurationMs)) {
        phase = IDLE;
        return 1.0;
    }

    return sampleAt(now);
}

void CClick::reset() {
    phase = IDLE;
    heldButtons.clear();
}

bool CClick::isActive() const {
    return phase != IDLE;
}
