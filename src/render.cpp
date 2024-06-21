#include "globals.hpp"

#include <cmath>
#include <hyprland/src/Compositor.hpp>

#define private public
#include <hyprland/src/managers/PointerManager.hpp>
#undef private

#include "render.hpp"

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

    g_pHyprOpenGL->renderTextureWithDamage(texture, &box, &damage, 1.F);
}

double CDynamicCursors::calculate(Vector2D* pos) {
    // translate to origin
    this->end.x -= pos->x;
    this->end.y -= pos->y;

    // normalize
    double size = this->end.size();
    this->end.x /= size;
    this->end.y /= size;

    // scale to length
    this->end.x *= this->size;
    this->end.y *= this->size;

    // calculate angle
    double angle = -atan(this->end.x / this->end.y);
    if (this->end.y > 0) angle += PI;
    angle += PI;

    // translate back
    this->end.x += pos->x;
    this->end.y += pos->y;

    return angle;
}
