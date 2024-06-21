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

void CDynamicCursors::renderSoftware(CPointerManager* pointers, SP<CMonitor> pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos) {

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

    // we rotate the cursor by our calculated amount
    box.rot = this->calculate(&pointers->pointerPos);

    // now pass the hotspot to rotate around
    renderCursorTextureInternalWithDamage(texture, &box, &damage, 1.F, pointers->currentCursorImage.hotspot);
}

void CDynamicCursors::damageSoftware(CPointerManager* pointers) {

    // we damage a 3x3 area around the cursor, to accomodate for all possible hotspots and rotations
    Vector2D size = pointers->currentCursorImage.size / pointers->currentCursorImage.scale;
    CBox b = CBox{pointers->pointerPos, size * 3}.translate(-(pointers->currentCursorImage.hotspot + size));

    static auto PNOHW = CConfigValue<Hyprlang::INT>("cursor:no_hardware_cursors");

    for (auto& mw : pointers->monitorStates) {
        if (mw->monitor.expired())
            continue;

        if ((mw->softwareLocks > 0 || mw->hardwareFailed || *PNOHW) && b.overlaps({mw->monitor->vecPosition, mw->monitor->vecSize})) {
            g_pHyprRenderer->damageBox(&b, mw->monitor->shouldSkipScheduleFrameOnMouseEvent());
            break;
        }
    }
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
