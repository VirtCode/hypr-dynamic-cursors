#pragma once

#include <hyprutils/math/Vector2D.hpp>

using namespace Hyprutils::Math;

/* specifies when a mode wants to be updated */
enum EModeUpdate {
    MOVE, // on mouse move
    TICK  // on tick (i.e. every frame)
};

/* result determined by the mode  */
struct SModeResult {
    // rotation of the shape around hotspot
    double rotation = 0;

    // uniform scaling of the shape around hotspot
    double scale = 1;

    // stretch along axis with angle, going through hotspot
    struct {
        double angle = 0;
        Vector2D magnitude = Vector2D{1,1};
    } stretch;

    void clamp(double angle, double scale, double stretch);
    bool hasDifference(SModeResult* other, double angle, double scale, double stretch);
};

double activation(std::string function, double max, double value);
