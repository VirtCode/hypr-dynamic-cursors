#pragma once

#include <hyprutils/math/Vector2D.hpp>
#include "utils.hpp"

using namespace Hyprutils::Math;

class IMode {
  public:
    /* returns the desired updating strategy for the given mode */
    virtual EModeUpdate strategy() = 0;
    /* updates the calculations and returns the new result */
    virtual SModeResult update(Vector2D pos) = 0;
    /* reset the internal stuff of the mode */
    virtual void reset() = 0;
    /* called on warp, an update will be sent afterwards (probably) */
    virtual void warp(Vector2D old, Vector2D pos) = 0;
};
