#pragma once

#include <hyprutils/math/Vector2D.hpp>
#include "../mode/utils.hpp"

using namespace Hyprutils::Math;

class CEdgeSquash {
  public:
    /* calculates the squash effect when near screen edges */
    SModeResult update(Vector2D pos);
    /* called when a cursor warp has happened */
    void warp(Vector2D old, Vector2D pos);

  private:
    Vector2D lastPos;
    double lastSquashFactor = 0.0;
    double squashVelocity = 0.0;
};
