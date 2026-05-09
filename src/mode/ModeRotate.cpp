#include "../config/config.hpp"
#include "src/macros.hpp"
#include <cmath>
#include "ModeRotate.hpp"

EModeUpdate CModeRotate::strategy() {
    return MOVE;
}

SModeResult CModeRotate::update(Vector2D pos) {
    auto length = g_pConfigHandler->m_shapeRules.getIntOr("rotate:length", CONFIG(rotateLength));
    auto offset = g_pConfigHandler->m_shapeRules.getFloatOr("rotate:offset", CONFIG(rotateOffset));

    // this mode has just started, start at upright orientation
    if (end.y == 0 && end.x == 0) {
        end.x = pos.x;
        end.y = pos.y + length;
    }

    // translate to origin
    end.x -= pos.x;
    end.y -= pos.y;

    // normalize
    double size = end.size();
    if (size == 0) size = 1; // do not divide by 0

    end.x /= size;
    end.y /= size;

    // scale to length
    end.x *= length;
    end.y *= length;

    // calculate angle
    double angle = -std::atan(end.x / end.y);
    if (end.y > 0) angle += PI;
    angle += PI;
    angle += offset * ((2 * PI) / 360); // convert to radians

    if (end.y == 0) angle = 0; // do not divide by 0 above, leave untransformed in these cases

    // translate back
    end.x += pos.x;
    end.y += pos.y;

    auto result = SModeResult();
    result.rotation = angle;

    return result;
}

void CModeRotate::warp(Vector2D old, Vector2D pos) {
    end += (pos - old);
}

void CModeRotate::reset() {
    end = {0, 0};
}
