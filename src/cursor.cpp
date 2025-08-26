#include <cmath>
#include <cstdlib>
#include <cstring>
#include <hyprcursor/hyprcursor.hpp>
#include <hyprlang.hpp>
#include <gbm.h>

#include <hyprland/src/managers/eventLoop/EventLoopTimer.hpp> // required so we don't "unprivate" chrono
#include <hyprutils/utils/ScopeGuard.hpp>

#define private public
#include <hyprland/src/managers/CursorManager.hpp>
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/Compositor.hpp>
#undef private

#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/protocols/core/Compositor.hpp>
#include <hyprland/src/protocols/core/Seat.hpp>
#include <hyprland/src/debug/Log.hpp>
#include <hyprland/src/helpers/math/Math.hpp>

#include "cursor.hpp"
#include "render/renderer.hpp"
#include "config/config.hpp"
#include "mode/Mode.hpp"
#include "render/CursorPassElement.hpp"
#include "render/Renderer.hpp"

void tickRaw(SP<CEventLoopTimer> self, void* data) {
    if (isEnabled())
        g_pDynamicCursors->onTick(g_pPointerManager.get());

    const int TIMEOUT = g_pHyprRenderer->m_mostHzMonitor && g_pHyprRenderer->m_mostHzMonitor->m_refreshRate > 0
        ? 1000.0 / g_pHyprRenderer->m_mostHzMonitor->m_refreshRate
        : 16;
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
void CDynamicCursors::renderSoftware(CPointerManager* pointers, SP<CMonitor> pMonitor, const Time::steady_tp& now, CRegion& damage, std::optional<Vector2D> overridePos, bool forceRender) {
    static auto* const* PNEAREST = (Hyprlang::INT* const*) getConfig(CONFIG_HIGHRES_NEAREST);

    if (!pointers->hasCursor())
        return;

    auto state = pointers->stateFor(pMonitor);
    auto zoom = resultShown.scale;

    if (!state->hardwareFailed && state->softwareLocks <= 0 && !forceRender) {
        if (pointers->m_currentCursorImage.surface)
                pointers->m_currentCursorImage.surface->resource()->frame(now);

        return;
    }

    // don't render cursor if forced but we are already using sw cursors for the monitor
    // otherwise we draw the cursor again for screencopy when using sw cursors
    if (forceRender && (state->hardwareFailed || state->softwareLocks != 0))
        return;

    auto box = state->box.copy();
    if (overridePos.has_value()) {
        box.x = overridePos->x;
        box.y = overridePos->y;

        box.translate(-pointers->m_currentCursorImage.hotspot);
    }

    auto texture = pointers->getCurrentCursorTexture();
    bool nearest = false;

    if (zoom > 1) {
        // this first has to undo the hotspot transform from getCursorBoxGlobal
        box.x += pointers->m_currentCursorImage.hotspot.x;
        box.y += pointers->m_currentCursorImage.hotspot.y;

        auto high = highres.getTexture();

        if (high) {
            texture = high;
            auto buf = highres.getBuffer();

            // we calculate a more accurate hotspot location if we have bigger shapes
            box.x -= (buf->m_hotspot.x / buf->size.x) * pointers->m_currentCursorImage.size.x * zoom;
            box.y -= (buf->m_hotspot.y / buf->size.y) * pointers->m_currentCursorImage.size.y * zoom;

            // only use nearest-neighbour if magnifying over size
            nearest = **PNEAREST == 2 && pointers->m_currentCursorImage.size.x * zoom > buf->size.x;

        } else {
            box.x -= pointers->m_currentCursorImage.hotspot.x * zoom;
            box.y -= pointers->m_currentCursorImage.hotspot.y * zoom;

            nearest = **PNEAREST;
        }
    }

    if (!texture)
        return;

    box.w *= zoom;
    box.h *= zoom;

    if (box.intersection(CBox{{}, {pMonitor->m_size}}).empty())
        return;

    box.scale(pMonitor->m_scale);
    box.x = std::round(box.x);
    box.y = std::round(box.y);

    // we rotate the cursor by our calculated amount
    box.rot = resultShown.rotation;

    CCursorPassElement::SRenderData data;
    data.tex = texture;
    data.box = box;

    data.hotspot = pointers->m_currentCursorImage.hotspot * state->monitor->m_scale * zoom;
    data.nearest = nearest;
    data.stretchAngle = resultShown.stretch.angle;
    data.stretchMagnitude = resultShown.stretch.magnitude;

    g_pHyprRenderer->m_renderPass.add(makeUnique<CCursorPassElement>(data));

    if (pointers->m_currentCursorImage.surface)
            pointers->m_currentCursorImage.surface->resource()->frame(now);
}

/*
This function implements damaging the screen such that the software cursor is drawn.
It is largely identical to hyprlands implementation, but expands the damage reagion, to accomodate various rotations.
*/
void CDynamicCursors::damageSoftware(CPointerManager* pointers) {

    // we damage a padding of the diagonal around the hotspot, to accomodate for all possible hotspots and rotations
    auto zoom = resultShown.scale;
    Vector2D size = pointers->m_currentCursorImage.size / pointers->m_currentCursorImage.scale * zoom;
    int diagonal = size.size();
    Vector2D padding = {diagonal, diagonal};

    CBox b = CBox{pointers->m_pointerPos, size + (padding * 2)}.translate(-(pointers->m_currentCursorImage.hotspot * zoom + padding));

    static auto PNOHW = CConfigValue<Hyprlang::INT>("cursor:no_hardware_cursors");

    for (auto& mw : pointers->m_monitorStates) {
        if (mw->monitor.expired())
            continue;

        if ((mw->softwareLocks > 0 || mw->hardwareFailed || *PNOHW) && b.overlaps({mw->monitor->m_position, mw->monitor->m_size})) {
            g_pHyprRenderer->damageBox(b, mw->monitor->shouldSkipScheduleFrameOnMouseEvent());
            break;
        }
    }
}

/*
This function reimplements the hardware cursor buffer drawing.
It is largely copied from hyprland, but adjusted to allow the cursor to be rotated.
*/
SP<Aquamarine::IBuffer> CDynamicCursors::renderHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, SP<CTexture> texture) {
    static auto* const* PHW_DEBUG = (Hyprlang::INT* const*) getConfig(CONFIG_HW_DEBUG);
    static auto* const* PNEAREST = (Hyprlang::INT* const*) getConfig(CONFIG_HIGHRES_NEAREST);

    auto output = state->monitor->m_output;

    auto maxSize    = output->cursorPlaneSize();
    auto zoom = resultShown.scale;

    auto cursorSize = pointers->m_currentCursorImage.size * zoom;
    int cursorDiagonal = cursorSize.size();
    auto cursorPadding = Vector2D{cursorDiagonal, cursorDiagonal};
    auto targetSize = cursorSize + cursorPadding * 2;

    if (maxSize == Vector2D{})
        return nullptr;

    if (maxSize != Vector2D{-1, -1}) {
        if (targetSize.x > maxSize.x || targetSize.y > maxSize.y) {
            Debug::log(TRACE, "hardware cursor too big! {} > {}", pointers->m_currentCursorImage.size, maxSize);
            return nullptr;
        }
    } else {
        maxSize = targetSize;
        if (maxSize.x < 16 || maxSize.y < 16) maxSize = {16, 16}; // fix some annoying crashes in nest
    }

    if (!state->monitor->m_cursorSwapchain || maxSize != state->monitor->m_cursorSwapchain->currentOptions().size || state->monitor->m_cursorSwapchain->currentOptions().length != 3) {

        if (!state->monitor->m_cursorSwapchain) {
            auto backend                    = state->monitor->m_output->getBackend();
            auto primary                    = backend->getPrimary();

            state->monitor->m_cursorSwapchain = Aquamarine::CSwapchain::create(state->monitor->m_output->getBackend()->preferredAllocator(), primary ? primary.lock() : backend);
        }

        auto options     = state->monitor->m_cursorSwapchain->currentOptions();
        options.size     = maxSize;
        // we still have to create a triple buffering swapchain, as we seem to be running into some sort of race condition
        // or something. I'll continue debugging this when I find some energy again, I've spent too much time here already.
        options.length   = 3;
        options.scanout  = true;
        options.cursor   = true;
        options.multigpu = state->monitor->m_output->getBackend()->preferredAllocator()->drmFD() != g_pCompositor->m_drm.fd;
        // We do not set the format. If it's unset (DRM_FORMAT_INVALID) then the swapchain will pick for us,
        // but if it's set, we don't wanna change it.

        if (!state->monitor->m_cursorSwapchain->reconfigure(options)) {
            Debug::log(TRACE, "Failed to reconfigure cursor swapchain");
            return nullptr;
        }
    }

    // if we already rendered the cursor, revert the swapchain to avoid rendering the cursor over
    // the current front buffer
    // this flag will be reset in the preRender hook, so when we commit this buffer to KMS
    // see https://github.com/hyprwm/Hyprland/commit/4c3b03516209a49244a8f044143c1162752b8a7a
    // this is however still not enough, see above
    if (state->cursorRendered)
        state->monitor->m_cursorSwapchain->rollback();

    state->cursorRendered = true;

    auto buf = state->monitor->m_cursorSwapchain->next(nullptr);
    if (!buf) {
        Debug::log(TRACE, "Failed to acquire a buffer from the cursor swapchain");
        return nullptr;
    }

    CRegion damage = {0, 0, INT16_MAX, INT16_MAX};

    g_pHyprRenderer->makeEGLCurrent();
    g_pHyprOpenGL->m_renderData.pMonitor = state->monitor;

    auto RBO = g_pHyprRenderer->getOrCreateRenderbuffer(buf, state->monitor->m_cursorSwapchain->currentOptions().format);

    // we just fail if we cannot create a render buffer, this will force hl to render sofware cursors, which we support
    if (!RBO)
        return nullptr;

    RBO->bind();

    g_pHyprOpenGL->beginSimple(state->monitor.lock(), damage, RBO);

    if (**PHW_DEBUG)
        g_pHyprOpenGL->clear(CHyprColor{rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX), 1.F});
    else
        g_pHyprOpenGL->clear(CHyprColor{0.F, 0.F, 0.F, 0.F});


    // the box should start in the middle portion, rotate by our calculated amount
    CBox xbox = {cursorPadding, Vector2D{pointers->m_currentCursorImage.size / pointers->m_currentCursorImage.scale * state->monitor->m_scale * zoom}.round()};
    xbox.rot = resultShown.rotation;

    //  use our custom draw function
    renderCursorTextureInternalWithDamage(texture, &xbox, damage, 1.F, pointers->m_currentCursorImage.hotspot * state->monitor->m_scale * zoom, zoom > 1 && **PNEAREST, resultShown.stretch.angle, resultShown.stretch.magnitude);

    g_pHyprOpenGL->end();
    glFlush();
    g_pHyprOpenGL->m_renderData.pMonitor.reset();

    g_pHyprRenderer->onRenderbufferDestroy(RBO.get());

    return buf;
}

/*
Implements the hardware cursor setting.
It is also mostly the same as stock hyprland, but with the hotspot translated more into the middle.
*/
bool CDynamicCursors::setHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, SP<Aquamarine::IBuffer> buf) {
    if (!(state->monitor->m_output->getBackend()->capabilities() & Aquamarine::IBackendImplementation::eBackendCapabilities::AQ_BACKEND_CAPABILITY_POINTER))
        return false;

    auto PMONITOR = state->monitor.lock();
    if (!PMONITOR->m_cursorSwapchain)
        return false;

    // we need to transform the hotspot manually as we need to indent it by the padding
    int diagonal = pointers->m_currentCursorImage.size.size();
    Vector2D padding = {diagonal, diagonal};

    const auto HOTSPOT = CBox{((pointers->m_currentCursorImage.hotspot * PMONITOR->m_scale) + padding) * resultShown.scale, {0, 0}}
        .transform(wlTransformToHyprutils(invertTransform(PMONITOR->m_transform)), PMONITOR->m_cursorSwapchain->currentOptions().size.x, PMONITOR->m_cursorSwapchain->currentOptions().size.y)
        .pos();

    Debug::log(TRACE, "[pointer] hw transformed hotspot for {}: {}", state->monitor->m_name, HOTSPOT);

    if (!state->monitor->m_output->setCursor(buf, HOTSPOT))
        return false;

    state->cursorFrontBuffer = buf;

    if (!state->monitor->shouldSkipScheduleFrameOnMouseEvent())
        g_pCompositor->scheduleFrameForMonitor(state->monitor.lock(), Aquamarine::IOutput::AQ_SCHEDULE_CURSOR_SHAPE);

    state->monitor->m_scanoutNeedsCursorUpdate = true;

    return true;
}

/*
Handles cursor move events.
*/
void CDynamicCursors::onCursorMoved(CPointerManager* pointers) {
    static auto* const* PSHAKE = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE);
    static auto* const* PIGNORE_WARPS = (Hyprlang::INT* const*) getConfig(CONFIG_IGNORE_WARPS);

    if (!pointers->hasCursor())
        return;

    const auto CURSORBOX = pointers->getCursorBoxGlobal();
    bool       recalc    = false;

    for (auto& m : g_pCompositor->m_monitors) {
        auto state = pointers->stateFor(m);

        state->box = pointers->getCursorBoxLogicalForMonitor(state->monitor.lock());

        auto CROSSES = !m->logicalBox().intersection(CURSORBOX).empty();

        if (!CROSSES && state->cursorFrontBuffer) {
            Debug::log(TRACE, "onCursorMoved for output {}: cursor left the viewport, removing it from the backend", m->m_name);
            pointers->setHWCursorBuffer(state, nullptr);
            continue;
        } else if (CROSSES && !state->cursorFrontBuffer) {
            Debug::log(TRACE, "onCursorMoved for output {}: cursor entered the output, but no front buffer, forcing recalc", m->m_name);
            recalc = true;
        }

        if (!state->entered)
            continue;

        Hyprutils::Utils::CScopeGuard x([m] { m->onCursorMovedOnMonitor(); });

        if (state->hardwareFailed)
            continue;

        const auto CURSORPOS = pointers->getCursorPosForMonitor(m);
        m->m_output->moveCursor(CURSORPOS);

        state->monitor->m_scanoutNeedsCursorUpdate = true;
    }

    if (recalc)
        pointers->updateCursorBackend();

    // ignore warp
    if (!isMove && **PIGNORE_WARPS) {
        auto mode = this->currentMode();
        if (mode) mode->warp(lastPos, pointers->m_pointerPos);

        if (**PSHAKE) shake.warp(lastPos, pointers->m_pointerPos);
    }

    calculate(MOVE);

    isMove = false;
    lastPos = pointers->m_pointerPos;
}

void CDynamicCursors::setShape(const std::string& shape) {
    g_pShapeRuleHandler->activate(shape);
    highres.loadShape(shape);
}

void CDynamicCursors::unsetShape() {
    g_pShapeRuleHandler->activate("clientside");
    highres.loadShape("clientside");
}

void CDynamicCursors::updateTheme() {
    highres.update();
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
        // reset mode if it has changed
        if (mode != lastMode) mode->reset();

        if (mode->strategy() == type) resultMode = mode->update(g_pPointerManager->m_pointerPos);
    } else resultMode = SModeResult();

    lastMode = mode;

    if (**PSHAKE) {
        if (type == TICK) resultShake = shake.update(g_pPointerManager->m_pointerPos);

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

        for (auto& m : g_pCompositor->m_monitors) {
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

void CDynamicCursors::setMove() {
    isMove = true;
}

void CDynamicCursors::dispatchMagnify(std::optional<int> duration, std::optional<float> size) {
    static auto* const* PSHAKE = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE);
    if (!**PSHAKE) return;

    shake.force(duration, size);
}
