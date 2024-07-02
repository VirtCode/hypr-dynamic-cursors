#include "../globals.hpp"
#include "ModeRotate.hpp"

EModeUpdate CModeRotate::strategy() {
    return MOVE;
}

SModeResult CModeRotate::update(Vector2D pos) {
    static auto* const* PLENGTH = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_ROTATE_LENGTH)->getDataStaticPtr();
    static auto* const* POFFSET = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_ROTATE_OFFSET)->getDataStaticPtr();

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
    angle += **POFFSET * ((2 * PI) / 360); // convert to radiants

    // translate back
    end.x += pos.x;
    end.y += pos.y;

    auto result = SModeResult();
    result.rotation = angle;

    return result;
}
