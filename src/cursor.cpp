#include "config/config.hpp"
#include "mode/Mode.hpp"
#include "src/debug/Log.hpp"
#include "src/helpers/math/Math.hpp"
#include "src/managers/eventLoop/EventLoopManager.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <hyprlang.hpp>
#include <gbm.h>

#define private public
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/Compositor.hpp>
#undef private

#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/protocols/core/Compositor.hpp>

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

    Debug::log(LOG, "[dynamic-cursors] rendering software cursors");
    // now pass the hotspot to rotate around
    renderCursorTextureInternalWithDamage(texture, &box, &damage, 1.F, nullptr, 0, pointers->currentCursorImage.hotspot * state->monitor->scale * zoom, zoom > 1 && **PNEAREST, resultShown.stretch.angle, resultShown.stretch.magnitude);

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
SP<Aquamarine::IBuffer> CDynamicCursors::renderHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, SP<CTexture> texture) {
    static auto* const* PHW_DEBUG = (Hyprlang::INT* const*) getConfig(CONFIG_HW_DEBUG);
    static auto* const* PNEAREST = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE_NEAREST);

    auto output = state->monitor->output;

    auto maxSize    = output->cursorPlaneSize();
    auto zoom = resultShown.scale;

    auto cursorSize = pointers->currentCursorImage.size * zoom;
    int cursorDiagonal = cursorSize.size();
    auto cursorPadding = Vector2D{cursorDiagonal, cursorDiagonal};
    auto targetSize = cursorSize + cursorPadding * 2;

    if (maxSize == Vector2D{})
        return nullptr;

    if (maxSize != Vector2D{-1, -1}) {
        if (targetSize.x > maxSize.x || targetSize.y > maxSize.y) {
            Debug::log(TRACE, "hardware cursor too big! {} > {}", pointers->currentCursorImage.size, maxSize);
            return nullptr;
        }
    } else
        maxSize = targetSize;

    if (!state->monitor->cursorSwapchain || maxSize != state->monitor->cursorSwapchain->currentOptions().size) {

        if (!state->monitor->cursorSwapchain)
            state->monitor->cursorSwapchain = Aquamarine::CSwapchain::create(state->monitor->output->getBackend()->preferredAllocator(), state->monitor->output->getBackend());

        auto options     = state->monitor->cursorSwapchain->currentOptions();
        options.size     = maxSize;
        options.length   = 2;
        options.scanout  = true;
        options.cursor   = true;
        options.multigpu = state->monitor->output->getBackend()->preferredAllocator()->drmFD() != g_pCompositor->m_iDRMFD;
        // We do not set the format. If it's unset (DRM_FORMAT_INVALID) then the swapchain will pick for us,
        // but if it's set, we don't wanna change it.

        if (!state->monitor->cursorSwapchain->reconfigure(options)) {
            Debug::log(TRACE, "Failed to reconfigure cursor swapchain");
            return nullptr;
        }
    }

    auto buf = state->monitor->cursorSwapchain->next(nullptr);
    if (!buf) {
        Debug::log(TRACE, "Failed to acquire a buffer from the cursor swapchain");
        return nullptr;
    }

    CRegion damage = {0, 0, INT16_MAX, INT16_MAX};

    g_pHyprRenderer->makeEGLCurrent();
    g_pHyprOpenGL->m_RenderData.pMonitor = state->monitor.get();

    auto RBO = g_pHyprRenderer->getOrCreateRenderbuffer(buf, state->monitor->cursorSwapchain->currentOptions().format);
    if (!RBO) {

        // dumb copy won't work with this plugin, as we just copy the buffer, and can't apply transformations to it
        Debug::log(TRACE, "Failed to create cursor RB with format {}, mod {}", buf->dmabuf().format, buf->dmabuf().modifier);
        static auto PDUMB = CConfigValue<Hyprlang::INT>("cursor:allow_dumb_copy");
        if (!*PDUMB)
            return nullptr;

        auto bufData = buf->beginDataPtr(0);
        auto bufPtr  = std::get<0>(bufData);

        // clear buffer
        memset(bufPtr, 0, std::get<2>(bufData));

        auto texBuffer = pointers->currentCursorImage.pBuffer ? pointers->currentCursorImage.pBuffer : pointers->currentCursorImage.surface->resource()->current.buffer;

        if (texBuffer) {
            auto textAttrs = texBuffer->shm();
            auto texData   = texBuffer->beginDataPtr(GBM_BO_TRANSFER_WRITE);
            auto texPtr    = std::get<0>(texData);
            Debug::log(TRACE, "cursor texture {}x{} {} {} {}", textAttrs.size.x, textAttrs.size.y, (void*)texPtr, textAttrs.format, textAttrs.stride);
            // copy cursor texture
            for (int i = 0; i < texBuffer->shm().size.y; i++)
                memcpy(bufPtr + i * buf->dmabuf().strides[0], texPtr + i * textAttrs.stride, textAttrs.stride);
        }

        buf->endDataPtr();

        return buf;
    }

    RBO->bind();

    Debug::log(LOG, "[dynamic-cursors] rendering hardware cursors");
    g_pHyprOpenGL->beginSimple(state->monitor.get(), damage, RBO);

    if (**PHW_DEBUG)
        g_pHyprOpenGL->clear(CColor{rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX), 1.F});
    else
        g_pHyprOpenGL->clear(CColor{0.F, 0.F, 0.F, 0.F});


    // the box should start in the middle portion, rotate by our calculated amount
    CBox xbox = {cursorPadding, Vector2D{pointers->currentCursorImage.size / pointers->currentCursorImage.scale * state->monitor->scale * zoom}.round()};
    xbox.rot = resultShown.rotation;

    //  use our custom draw function
    renderCursorTextureInternalWithDamage(texture, &xbox, &damage, 1.F, pointers->currentCursorImage.waitTimeline, pointers->currentCursorImage.waitPoint, pointers->currentCursorImage.hotspot * state->monitor->scale * zoom, zoom > 1 && **PNEAREST, resultShown.stretch.angle, resultShown.stretch.magnitude);

    g_pHyprOpenGL->end();
    glFlush();
    g_pHyprOpenGL->m_RenderData.pMonitor = nullptr;

    g_pHyprRenderer->onRenderbufferDestroy(RBO.get());

    return buf;
}

/*
Implements the hardware cursor setting.
It is also mostly the same as stock hyprland, but with the hotspot translated more into the middle.
*/
bool CDynamicCursors::setHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, SP<Aquamarine::IBuffer> buf) {
    if (!(state->monitor->output->getBackend()->capabilities() & Aquamarine::IBackendImplementation::eBackendCapabilities::AQ_BACKEND_CAPABILITY_POINTER))
        return false;

    auto PMONITOR = state->monitor.lock();
    if (!PMONITOR->cursorSwapchain)
        return false;

    // we need to transform the hotspot manually as we need to indent it by the padding
    int diagonal = pointers->currentCursorImage.size.size();
    Vector2D padding = {diagonal, diagonal};

    const auto HOTSPOT = CBox{((pointers->currentCursorImage.hotspot * PMONITOR->scale) + padding) * resultShown.scale, {0, 0}}
        .transform(wlTransformToHyprutils(invertTransform(PMONITOR->transform)), PMONITOR->cursorSwapchain->currentOptions().size.x, PMONITOR->cursorSwapchain->currentOptions().size.y)
        .pos();

    Debug::log(TRACE, "[pointer] hw transformed hotspot for {}: {}", state->monitor->szName, HOTSPOT);

    if (!state->monitor->output->setCursor(buf, HOTSPOT))
        return false;

    state->cursorFrontBuffer = buf;

    g_pCompositor->scheduleFrameForMonitor(state->monitor.get(), Aquamarine::IOutput::AQ_SCHEDULE_CURSOR_SHAPE);

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
        m->output->moveCursor(CURSORPOS);
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

        Debug::log(LOG, "[dynamic-cursors] updating with rotation: {}, scale: {}, stretch: {},{}", resultShown.rotation, resultShown.scale, resultShown.stretch.magnitude.x, resultShown.stretch.magnitude.y);

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

            if (g_pPointerManager->softwareLockedFor(m))
                Debug::log(LOG, "[dynamic-cursors] monitor {} has software lock", m->szName);

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
