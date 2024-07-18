#include "config/config.hpp"
#include "mode/Mode.hpp"
#include "src/debug/Log.hpp"
#include "src/managers/eventLoop/EventLoopManager.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <hyprlang.hpp>

#define private public
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/Compositor.hpp>
#undef private

#include <hyprland/src/config/ConfigValue.hpp>
#include "hyprland/cursor.hpp"
#include <hyprland/src/protocols/core/Compositor.hpp>

#include <hyprland/wlr/interfaces/wlr_output.h>
#include <hyprland/wlr/render/interface.h>
#include <hyprland/wlr/render/wlr_renderer.h>

#include "cursor.hpp"
#include "renderer.hpp"

void tickRaw(SP<CEventLoopTimer> self, void* data) {
    static auto* const* PENABLED = (Hyprlang::INT* const*) getConfig(CONFIG_ENABLED);

    if (**PENABLED && g_pDynamicCursors)
        g_pDynamicCursors->onTick(g_pPointerManager.get());

    const int TIMEOUT = g_pHyprRenderer->m_pMostHzMonitor ? 1000.0 / g_pHyprRenderer->m_pMostHzMonitor->refreshRate : 16;
    self->updateTimeout(std::chrono::milliseconds(TIMEOUT));
}

CDynamicCursors::CDynamicCursors() {
    this->tick = SP<CEventLoopTimer>(new CEventLoopTimer(std::chrono::microseconds(500), tickRaw, nullptr));
    g_pEventLoopManager->addTimer(this->tick);
}

CDynamicCursors::~CDynamicCursors() {
    // stop and deallocate timer
    g_pEventLoopManager->removeTimer(this->tick);
    this->tick.reset();

    // release software lock
    if (zoomSoftware) {
        g_pPointerManager->unlockSoftwareAll();
        zoomSoftware = false;
    }
}

/*
Reimplements rendering of the software cursor.
Is also largely identical to hyprlands impl, but uses our custom rendering to rotate the cursor.
*/
void CDynamicCursors::renderSoftware(CPointerManager* pointers, SP<CMonitor> pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos) {
    static auto* const* PNEAREST = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE_NEAREST);

    if (!pointers->hasCursor())
        return;

    auto state = pointers->stateFor(pMonitor);
    auto zoom = resultShown.scale;

    if ((!state->hardwareFailed && state->softwareLocks == 0)) {
        if (pointers->currentCursorImage.surface)
                pointers->currentCursorImage.surface->resource()->frame(now);

        return;
    }

    auto box = state->box.copy();
    if (overridePos.has_value()) {
        box.x = overridePos->x;
        box.y = overridePos->y;
    }

    // poperly transform hotspot, this first has to undo the hotspot transform from getCursorBoxGlobal
    box.x = box.x + pointers->currentCursorImage.hotspot.x - pointers->currentCursorImage.hotspot.x * zoom;
    box.y = box.y + pointers->currentCursorImage.hotspot.y - pointers->currentCursorImage.hotspot.y * zoom;

    if (box.intersection(CBox{{}, {pMonitor->vecSize}}).empty())
        return;

    auto texture = pointers->getCurrentCursorTexture();
    if (!texture)
        return;

    box.scale(pMonitor->scale);
    box.w *= zoom;
    box.h *= zoom;

    // we rotate the cursor by our calculated amount
    box.rot = resultShown.rotation;

    // now pass the hotspot to rotate around
    renderCursorTextureInternalWithDamage(texture, &box, &damage, 1.F, pointers->currentCursorImage.hotspot * state->monitor->scale * zoom, zoom > 1 && **PNEAREST, resultShown.stretch.angle, resultShown.stretch.magnitude);

    if (pointers->currentCursorImage.surface)
            pointers->currentCursorImage.surface->resource()->frame(now);
}

/*
This function implements damaging the screen such that the software cursor is drawn.
It is largely identical to hyprlands implementation, but expands the damage reagion, to accomodate various rotations.
*/
void CDynamicCursors::damageSoftware(CPointerManager* pointers) {

    // we damage a padding of the diagonal around the hotspot, to accomodate for all possible hotspots and rotations
    auto zoom = resultShown.scale;
    Vector2D size = pointers->currentCursorImage.size / pointers->currentCursorImage.scale * zoom;
    int diagonal = size.size();
    Vector2D padding = {diagonal, diagonal};

    CBox b = CBox{pointers->pointerPos, size + (padding * 2)}.translate(-(pointers->currentCursorImage.hotspot * zoom + padding));

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

/*
This function reimplements the hardware cursor buffer drawing.
It is largely copied from hyprland, but adjusted to allow the cursor to be rotated.
*/
wlr_buffer* CDynamicCursors::renderHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, SP<CTexture> texture) {
    static auto* const* PHW_DEBUG= (Hyprlang::INT* const*) getConfig(CONFIG_HW_DEBUG);
    static auto* const* PNEAREST = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE_NEAREST);

    auto output = state->monitor->output;
    auto zoom = resultShown.scale;

    auto size = pointers->currentCursorImage.size * zoom;
    int diagonal = size.size();
    Vector2D padding = {diagonal, diagonal};

    // we try to allocate a buffer with padding, see software damage
    auto target = size + padding * 2;

    if (output->impl->get_cursor_size) {
        int w, h;
        output->impl->get_cursor_size(output, &w, &h);

        if (w < target.x || h < target.y) {
            Debug::log(TRACE, "hardware cursor too big! {} > {}x{}", pointers->currentCursorImage.size, w, h);
            return nullptr;
        }

        target.x = w;
        target.y = h;
    }

    if (target.x <= 0 || target.y <= 0) {
        Debug::log(TRACE, "hw cursor for output {} failed the size checks ({}x{} is invalid)", state->monitor->szName, target.x, target.y);
        return nullptr;
    }

    if (!output->cursor_swapchain || target != Vector2D{output->cursor_swapchain->width, output->cursor_swapchain->height}) {
        wlr_drm_format fmt = {0};
        if (!output_pick_cursor_format(output, &fmt)) {
            Debug::log(TRACE, "Failed to pick cursor format");
            return nullptr;
        }

        wlr_swapchain_destroy(output->cursor_swapchain);
        output->cursor_swapchain = wlr_swapchain_create(output->allocator, target.x, target.y, &fmt);
        wlr_drm_format_finish(&fmt);

        if (!output->cursor_swapchain) {
            Debug::log(TRACE, "Failed to create cursor swapchain");
            return nullptr;
        }
    }

    wlr_buffer* buf = wlr_swapchain_acquire(output->cursor_swapchain, nullptr);
    if (!buf) {
        Debug::log(TRACE, "Failed to acquire a buffer from the cursor swapchain");
        return nullptr;
    }

    CRegion damage = {0, 0, INT16_MAX, INT16_MAX};

    g_pHyprRenderer->makeEGLCurrent();
    g_pHyprOpenGL->m_RenderData.pMonitor = state->monitor.get(); // has to be set cuz allocs

    const auto RBO = g_pHyprRenderer->getOrCreateRenderbuffer(buf, DRM_FORMAT_ARGB8888);
    RBO->bind();

    g_pHyprOpenGL->beginSimple(state->monitor.get(), damage, RBO);

    if (**PHW_DEBUG)
        g_pHyprOpenGL->clear(CColor{rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX), 1.F});
    else
        g_pHyprOpenGL->clear(CColor{0.F, 0.F, 0.F, 0.F});

    // the box should start in the middle portion, rotate by our calculated amount
    CBox xbox = {padding, Vector2D{pointers->currentCursorImage.size / pointers->currentCursorImage.scale * state->monitor->scale * zoom}.round()};
    xbox.rot = resultShown.rotation;

    //  use our custom draw function
    renderCursorTextureInternalWithDamage(texture, &xbox, &damage, 1.F, pointers->currentCursorImage.hotspot * state->monitor->scale * zoom, zoom > 1 && **PNEAREST, resultShown.stretch.angle, resultShown.stretch.magnitude);

    g_pHyprOpenGL->end();
    glFlush();
    g_pHyprOpenGL->m_RenderData.pMonitor = nullptr;

    wlr_buffer_unlock(buf);

    return buf;
}

/*
Implements the hardware cursor setting.
It is also mostly the same as stock hyprland, but with the hotspot translated more into the middle.
*/
bool CDynamicCursors::setHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, wlr_buffer* buf) {
    if (!state->monitor->output->impl->set_cursor)
        return false;

    auto P_MONITOR = state->monitor.lock();
    if (!P_MONITOR->output->cursor_swapchain) return false;

    // we need to transform the hotspot manually as we need to indent it by the padding
    int diagonal = pointers->currentCursorImage.size.size();
    Vector2D padding = {diagonal, diagonal};

    const auto HOTSPOT = CBox{((pointers->currentCursorImage.hotspot * P_MONITOR->scale) + padding) * resultShown.scale, {0, 0}}
        .transform(wlTransformToHyprutils(wlr_output_transform_invert(P_MONITOR->transform)), P_MONITOR->output->cursor_swapchain->width, P_MONITOR->output->cursor_swapchain->height)
        .pos();

    Debug::log(TRACE, "[pointer] hw transformed hotspot for {}: {}", state->monitor->szName, HOTSPOT);

    if (!state->monitor->output->impl->set_cursor(state->monitor->output, buf, HOTSPOT.x, HOTSPOT.y))
        return false;

    wlr_buffer_unlock(state->cursorFrontBuffer);
    state->cursorFrontBuffer = buf;

    g_pCompositor->scheduleFrameForMonitor(state->monitor.get());

    if (buf)
        wlr_buffer_lock(buf);

    return true;
}

/*
Handles cursor move events.
*/
void CDynamicCursors::onCursorMoved(CPointerManager* pointers) {
    if (!pointers->hasCursor())
        return;

    for (auto& m : g_pCompositor->m_vMonitors) {
        auto state = pointers->stateFor(m);

        state->box = pointers->getCursorBoxLogicalForMonitor(state->monitor.lock());

        if (state->hardwareFailed || !state->entered)
            continue;

        const auto CURSORPOS = pointers->getCursorPosForMonitor(m);
        m->output->impl->move_cursor(m->output, CURSORPOS.x, CURSORPOS.y);
    }

    calculate(MOVE);
}

void CDynamicCursors::setShape(const std::string& shape) {
    g_pShapeRuleHandler->activate(shape);
}

void CDynamicCursors::unsetShape() {
    g_pShapeRuleHandler->activate("clientside");
}

/*
Handle cursor tick events.
*/
void CDynamicCursors::onTick(CPointerManager* pointers) {
    calculate(TICK);
}

IMode* CDynamicCursors::currentMode() {
    static auto const* PMODE = (Hyprlang::STRING const*) getConfig(CONFIG_MODE);
    auto mode = g_pShapeRuleHandler->getModeOr(*PMODE);

    if (mode == "rotate") return &rotate;
    else if (mode == "tilt") return &tilt;
    else if (mode == "stretch") return &stretch;
    else return nullptr;
}

void CDynamicCursors::calculate(EModeUpdate type) {
    static auto* const* PTHRESHOLD = (Hyprlang::INT* const*) getConfig(CONFIG_THRESHOLD);
    static auto* const* PSHAKE = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE);
    static auto* const* PSHAKE_EFFECTS = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE_EFFECTS);

    IMode* mode = currentMode();

    // calculate angle and zoom
    if (mode) {
        if (mode->strategy() == type) resultMode = mode->update(g_pPointerManager->pointerPos);
    } else resultMode = SModeResult();

    if (**PSHAKE) {
        if (type == TICK) resultShake = shake.update(g_pPointerManager->pointerPos);

        // reset mode results if shaking
        if (resultShake > 1 && !**PSHAKE_EFFECTS) resultMode = SModeResult();
    } else resultShake = 1;

    auto result = resultMode;
    result.scale *= resultShake;

    if (resultShown.hasDifference(&result, **PTHRESHOLD * (PI / 180.0), 0.01, 0.01)) {
        resultShown = result;
        resultShown.clamp(**PTHRESHOLD * (PI / 180.0), 0.01, 0.01); // clamp low values so it is rendered pixel-perfectly when no effect

        // lock software cursors if zooming
        if (resultShown.scale > 1) {
            if (!zoomSoftware) {
                g_pPointerManager->lockSoftwareAll();
                zoomSoftware = true;
            }
        } else {
            if (zoomSoftware) {
                // damage so it is cleared properly
                g_pPointerManager->damageIfSoftware();

                g_pPointerManager->unlockSoftwareAll();
                zoomSoftware = false;
            }
        }

        // damage software and change hardware cursor shape
        g_pPointerManager->damageIfSoftware();

        bool entered = false;

        for (auto& m : g_pCompositor->m_vMonitors) {
            auto state = g_pPointerManager->stateFor(m);

            if (state->entered) entered = true;
            if (state->hardwareFailed || !state->entered)
                continue;

            g_pPointerManager->attemptHardwareCursor(state);
        }

        // there should always be one monitor entered
        // this fixes an issue wheter the cursor shape would not properly update after change
        if (!entered) {
            Debug::log(LOG, "[dynamic-cursors] updating because none entered");
            g_pPointerManager->recheckEnteredOutputs();
            g_pPointerManager->updateCursorBackend();
        }
    }
}
