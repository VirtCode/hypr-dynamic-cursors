#pragma once

#include <hyprutils/math/Vector2D.hpp>

using namespace Hyprutils::Math;

/* specifies when a mode wants to be updated */
enum EModeUpdate {
    MOVE, // on mouse move
    TICK  // on tick (i.e. every frame)
};

class IMode {
  public:
    /* returns the desired updating strategy for the given mode */
    virtual EModeUpdate strategy() = 0;
    /* updates the calculations and returns the new angle */
    virtual double update(Vector2D pos) = 0;
};
