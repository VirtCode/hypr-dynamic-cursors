#pragma once

#include <hyprutils/math/Vector2D.hpp>
#include "utils.hpp"

using namespace Hyprutils::Math;

class IMode {
  public:
    /* returns the desired updating strategy for the given mode */
    virtual EModeUpdate strategy() = 0;
    /* updates the calculations and returns the new angle */
    virtual SModeResult update(Vector2D pos) = 0;
};
