#include "globals.hpp"
#include "src/debug/Log.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>

#define private public
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/Compositor.hpp>
#undef private

#include <hyprland/src/config/ConfigValue.hpp>
#include "hyprland/cursor.hpp"

#include <hyprland/wlr/interfaces/wlr_output.h>
#include <hyprland/wlr/render/interface.h>
#include <hyprland/wlr/render/wlr_renderer.h>

#include "cursor.hpp"
#include "renderer.hpp"

/*
Reimplements rendering of the software cursor.
Is also largely identical to hyprlands impl, but uses our custom rendering to rotate the cursor.
*/
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
    box.rot = this->angle;

    // now pass the hotspot to rotate around
    renderCursorTextureInternalWithDamage(texture, &box, &damage, 1.F, pointers->currentCursorImage.hotspot);
}

/*
This function implements damaging the screen such that the software cursor is drawn.
It is largely identical to hyprlands implementation, but expands the damage reagion, to accomodate various rotations.
*/
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

/*
This function reimplements the hardware cursor buffer drawing.
It is largely copied from hyprland, but adjusted to allow the cursor to be rotated.
*/
wlr_buffer* CDynamicCursors::renderHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, SP<CTexture> texture) {
    static auto* const* PHW_DEBUG= (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_HW_DEBUG)->getDataStaticPtr();

    auto output = state->monitor->output;

    auto size = pointers->currentCursorImage.size;
    // we try to allocate a buffer that is thrice as big, see software rendering
    auto target = size * 3;

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
    CBox xbox = {size, Vector2D{pointers->currentCursorImage.size / pointers->currentCursorImage.scale * state->monitor->scale}.round()};
    xbox.rot = this->angle;

    //  use our custom draw function
    renderCursorTextureInternalWithDamage(texture, &xbox, &damage, 1.F, pointers->currentCursorImage.hotspot);

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

    // we need to transform the hotspot manually as we need to indent it by the size
    const auto HOTSPOT = CBox{(pointers->currentCursorImage.hotspot + pointers->currentCursorImage.size) * P_MONITOR->scale, {0, 0}}
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

    static auto const* PMODE = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_MODE)->getDataStaticPtr();
    if (!strcmp(*PMODE, "rotate")) calculate();
}

/*
Handle cursor tick events.
*/
void CDynamicCursors::onTick(CPointerManager* pointers) {
    static auto const* PMODE = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_MODE)->getDataStaticPtr();

    if (!strcmp(*PMODE, "tilt")) calculate();
}

void CDynamicCursors::calculate() {
    static auto const* PMODE = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_MODE)->getDataStaticPtr();
    static auto* const* PTHRESHOLD = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_THRESHOLD)->getDataStaticPtr();

    double angle = 0;
    if (!strcmp(*PMODE, "rotate"))
        angle = calculateStick();
    else if (!strcmp(*PMODE, "tilt"))
        angle = calculateAir();
    else
        Debug::log(WARN, "[dynamic-cursors] unknown mode specified");

    // we only consider the angle changed if it is larger than 1 degree
    if (abs(this->angle - angle) > ((PI / 180) * **PTHRESHOLD)) {
        this->angle = angle;

        // damage software and change hardware cursor shape
        g_pPointerManager->damageIfSoftware();

        for (auto& m : g_pCompositor->m_vMonitors) {
            auto state = g_pPointerManager->stateFor(m);
            if (state->hardwareFailed || !state->entered)
                continue;

            g_pPointerManager->attemptHardwareCursor(state);
        }
    }
}

double airFunction(double speed) {
    static auto const* PFUNCTION = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_FUNCTION)->getDataStaticPtr();
    static auto* const* PMASS = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_MASS)->getDataStaticPtr();
    double mass = **PMASS;

    double result = 0;
    if (!strcmp(*PFUNCTION, "linear")) {

        result = speed / **PMASS;

    } else if (!strcmp(*PFUNCTION, "quadratic")) {


        // (1 / m²) * x², is a quadratic function which will reach 1 at m
        result = (1.0 / (mass * mass)) * (speed * speed);
        result *= (speed > 0 ? 1 : -1);

    } else if (!strcmp(*PFUNCTION, "negative_quadratic")) {

        float x = std::abs(speed);
        // (-1 / m²) * (x - m)² + 1, is a quadratic function with the inverse curvature which will reach 1 at m
        result = (-1.0 / (mass * mass)) * ((x - mass) * (x - mass)) + 1;
        if (x > mass) result = 1; // need to clamp manually, as the function would decrease again

        result *= (speed > 0 ? 1 : -1);
    } else
        Debug::log(WARN, "[dynamic-cursors] unknown air function specified");

    return std::clamp(result, -1.0, 1.0);
}

double CDynamicCursors::calculateAir() {
    // create samples array
    int max = g_pHyprRenderer->m_pMostHzMonitor->refreshRate / 10; // 100ms worth of history
    samples.resize(max);

    // capture current sample
    samples[samples_index] = Vector2D{g_pPointerManager->pointerPos};
    int current = samples_index;
    samples_index = (samples_index + 1) % max; // increase for next sample
    int first = samples_index;

    /* turns out this is not relevant on my systems (should've checked before implementing lol):
    // motion smooting
    // fills samples in between with linear approximations
    // accomodates for mice with low polling rates and monitors with high fps
    int previous = current == 0 ? max - 1 : current - 1;
    if (samples[previous] != samples[current]) {
        int steps = std::abs(samples_last_change - previous);
        Vector2D amount = (samples[current] - samples[previous]) / steps;

        int factor = 1;
        for (int i = (samples_last_change + 1) % max; i != current; i = (i + 1) % max) {
            samples[i] += amount * factor++;
        }

        samples_last_change = current;
    } else if (samples_last_change == current) {
        samples_last_change = first; // next is the last then
    }
    */

    // calculate speed and tilt
    double speed = (samples[current].x - samples[first].x) / 0.1;

    return airFunction(speed) * (PI / 3); // 120° in both directions
}

double CDynamicCursors::calculateStick() {
    static auto* const* PLENGTH = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_LENGTH)->getDataStaticPtr();

    auto pos = g_pPointerManager->pointerPos;

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
    double angle = -atan(end.x / end.y);
    if (end.y > 0) angle += PI;
    angle += PI;

    // translate back
    end.x += pos.x;
    end.y += pos.y;

    return angle;
}
