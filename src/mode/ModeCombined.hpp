#pragma once
#include "Mode.hpp"
#include <hyprutils/math/Vector2D.hpp>
#include <vector>

/*
    COMBINED MODE - the ultimate cursor experience
    stacks tilt + rotate + stretch all at once for maximum realism
*/
class CModeCombined : public IMode {
  public:
    virtual EModeUpdate strategy();
    virtual SModeResult update(Vector2D pos);
    virtual void reset();
    virtual void warp(Vector2D old, Vector2D pos);

  private:
    // for rotate - end of simulated stick
    Vector2D rotateEnd;

    // for tilt & stretch - ring buffer of position samples
    std::vector<Vector2D> samples;
    int samples_index = 0;
};