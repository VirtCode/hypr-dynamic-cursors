#pragma once

#include <chrono>
#include <cstdint>
#include <unordered_set>
#include <hyprland/src/devices/IPointer.hpp>
#include <hyprutils/signal/Listener.hpp>

class CClick {
  public:
    CClick();

    /* handles a mouse button press/release event; returns true if animation state changed */
    bool onButton(const IPointer::SButtonEvent& event);

    /* calculates the current click scale factor for the current timestamp */
    double update();

    /* resets any active click state back to 1.0 */
    void reset();

    /* returns whether a click animation or hold is currently active */
    bool isActive() const;

  private:
    enum EPhase {
        IDLE,
        PRESSING,
        RELEASING,
    };

    double sampleAt(std::chrono::steady_clock::time_point now) const;

    EPhase                       phase = IDLE;
    std::unordered_set<uint32_t> heldButtons;

    std::chrono::steady_clock::time_point pressStartTime;
    std::chrono::steady_clock::time_point releaseStartTime;

    double pressStartScale   = 1.0;
    double targetPressScale  = 0.82;
    double releaseStartScale = 0.82;
    double targetOvershoot   = 1.05;

    int pressDurationMs   = 60;
    int releaseDurationMs = 150;
};
