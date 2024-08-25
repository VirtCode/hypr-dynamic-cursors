#include "../config/config.hpp"
#include "src/macros.hpp"
#include <cmath>
#include "ModeRotate.hpp"

EModeUpdate CModeRotate::strategy() {
    return MOVE;
}

SModeResult CModeRotate::update(Vector2D pos) {
    static auto* const* PLENGTH = (Hyprlang::INT* const*) getConfig(CONFIG_ROTATE_LENGTH);
    static auto* const* POFFSET = (Hyprlang::FLOAT* const*) getConfig(CONFIG_ROTATE_OFFSET);
    auto length = g_pShapeRuleHandler->getIntOr(CONFIG_ROTATE_LENGTH, **PLENGTH);
    auto offset = g_pShapeRuleHandler->getFloatOr(CONFIG_ROTATE_OFFSET, **POFFSET);

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
    angle += offset * ((2 * PI) / 360); // convert to radiants

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
