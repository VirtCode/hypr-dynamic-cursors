#include "../globals.hpp"
#include "ModeRotate.hpp"

EModeUpdate CModeRotate::strategy() {
    return MOVE;
}

double CModeRotate::update(Vector2D pos) {
    static auto* const* PLENGTH = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_LENGTH)->getDataStaticPtr();

    // translate to origin
    end.x -= pos.x;
    end.y -= pos.y;

    // normalize
    double size = end.size();
    end.x /= size;
    end.y /= size;

    // scale to length
    end.x *= **PLENGTH;
    end.y *= **PLENGTH;

    // calculate angle
    double angle = -std::atan(end.x / end.y);
    if (end.y > 0) angle += PI;
    angle += PI;

    // translate back
    end.x += pos.x;
    end.y += pos.y;

    return angle;
}
