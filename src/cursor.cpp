#include "globals.hpp"

#include <cmath>

#define private public
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#undef private

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigValue.hpp>

#include "cursor.hpp"
#include "renderer.hpp"

void CDynamicCursors::render(CPointerManager* pointers, SP<CMonitor> pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos) {

    if (!pointers->hasCursor())
        return;

    auto state = pointers->stateFor(pMonitor);

    if ((!state->hardwareFailed && state->softwareLocks == 0)) {
        return;
    }

    auto box = state->box.copy();
    if (overridePos.has_value()) {
        box.x = overridePos->x;
        box.y = overridePos->y;
    }

    if (box.intersection(CBox{{}, {pMonitor->vecSize}}).empty())
        return;

    auto texture = pointers->getCurrentCursorTexture();
    if (!texture)
        return;

    box.scale(pMonitor->scale);
    box.rot = this->calculate(&pointers->pointerPos);

    renderCursorTextureInternalWithDamage(texture, &box, &damage, 1.F, pointers->currentCursorImage.hotspot);
}

double CDynamicCursors::calculate(Vector2D* pos) {
    static auto* const* PLENGTH = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_LENGTH)->getDataStaticPtr();

    // translate to origin
    end.x -= pos->x;
    end.y -= pos->y;

    // normalize
    double size = end.size();
    end.x /= size;
    end.y /= size;

    // scale to length
    end.x *= **PLENGTH;
    end.y *= **PLENGTH;

    // calculate angle
    double angle = -atan(end.x / end.y);
    if (end.y > 0) angle += PI;
    angle += PI;

    // translate back
    end.x += pos->x;
    end.y += pos->y;

    return angle;
}
