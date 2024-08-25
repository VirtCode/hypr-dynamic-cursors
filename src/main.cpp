#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprlang.hpp>
#include <unistd.h>

#include "globals.hpp"
#include "cursor.hpp"
#include "config/config.hpp"
#include "src/debug/Log.hpp"
#include "src/managers/PointerManager.hpp"
#include "src/version.h"

bool isEnabled() {
    static auto* const* PENABLED = (Hyprlang::INT* const*) getConfig(CONFIG_ENABLED);
    return **PENABLED;
}

typedef void (*origRenderSofwareCursorsFor)(void*, SP<CMonitor>, timespec*, CRegion&, std::optional<Vector2D>);
inline CFunctionHook* g_pRenderSoftwareCursorsForHook = nullptr;
void hkRenderSoftwareCursorsFor(void* thisptr, SP<CMonitor> pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos) {
    if (isEnabled()) g_pDynamicCursors->renderSoftware((CPointerManager*) thisptr, pMonitor, now, damage, overridePos);
    else (*(origRenderSofwareCursorsFor)g_pRenderSoftwareCursorsForHook->m_pOriginal)(thisptr, pMonitor, now, damage, overridePos);
}

typedef void (*origDamageIfSoftware)(void*);
inline CFunctionHook* g_pDamageIfSoftwareHook = nullptr;
void hkDamageIfSoftware(void* thisptr) {
    if (isEnabled()) g_pDynamicCursors->damageSoftware((CPointerManager*) thisptr);
    else (*(origDamageIfSoftware)g_pDamageIfSoftwareHook->m_pOriginal)(thisptr);
}

typedef SP<Aquamarine::IBuffer> (*origRenderHWCursorBuffer)(void*, SP<CPointerManager::SMonitorPointerState>, SP<CTexture>);
inline CFunctionHook* g_pRenderHWCursorBufferHook = nullptr;
SP<Aquamarine::IBuffer> hkRenderHWCursorBuffer(void* thisptr, SP<CPointerManager::SMonitorPointerState> state, SP<CTexture> texture) {
    if (isEnabled()) return g_pDynamicCursors->renderHardware((CPointerManager*) thisptr, state, texture);
    else return (*(origRenderHWCursorBuffer)g_pRenderHWCursorBufferHook->m_pOriginal)(thisptr, state, texture);
}

typedef bool (*origSetHWCursorBuffer)(void*, SP<CPointerManager::SMonitorPointerState>, SP<Aquamarine::IBuffer>);
inline CFunctionHook* g_pSetHWCursorBufferHook = nullptr;
bool hkSetHWCursorBuffer(void* thisptr, SP<CPointerManager::SMonitorPointerState> state, SP<Aquamarine::IBuffer> buffer) {
    if (isEnabled()) return g_pDynamicCursors->setHardware((CPointerManager*) thisptr, state, buffer);
    else return (*(origSetHWCursorBuffer)g_pSetHWCursorBufferHook->m_pOriginal)(thisptr, state, buffer);
}

typedef void (*origOnCursorMoved)(void*);
inline CFunctionHook* g_pOnCursorMovedHook = nullptr;
void hkOnCursorMoved(void* thisptr) {
    if (isEnabled()) return g_pDynamicCursors->onCursorMoved((CPointerManager*) thisptr);
    else return (*(origOnCursorMoved)g_pOnCursorMovedHook->m_pOriginal)(thisptr);
}

typedef void (*origSetCusorFromName)(void*, const std::string& name);
inline CFunctionHook* g_pSetCursorFromNameHook = nullptr;
void hkSetCursorFromName(void* thisptr, const std::string& name) {
    if (isEnabled()) g_pDynamicCursors->setShape(name);
    (*(origSetCusorFromName)g_pSetCursorFromNameHook->m_pOriginal)(thisptr, name);
}

typedef void (*origSetCursorSurface)(void*, SP<CWLSurface>, const Vector2D&);
inline CFunctionHook* g_pSetCursorSurfaceHook = nullptr;
void hkSetCursorSurface(void* thisptr, SP<CWLSurface> surf, const Vector2D& hotspot) {
    if (isEnabled()) g_pDynamicCursors->unsetShape();
    (*(origSetCursorSurface)g_pSetCursorSurfaceHook->m_pOriginal)(thisptr, surf, hotspot);
}

/* hooks a function hook */
CFunctionHook* hook(std::string name, std::string object, void* function) {
    auto names = HyprlandAPI::findFunctionsByName(PHANDLE, name);

    // we hook on member functions, so search for them
    for (auto match : names) {
        if (!match.demangled.starts_with(object)) continue;

        Debug::log(LOG, "[dynamic-cursors] hooking on {} for {}::{}", match.demangled, object, name);

        auto hook = HyprlandAPI::createFunctionHook(PHANDLE, match.address, function);
        hook->hook();

        return hook;
    }

    Debug::log(ERR, "Could not find hooking candidate for {}::{}", object, name);
    throw std::runtime_error("no hook candidate found");
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    // check that header version aligns with running version
    const std::string HASH = __hyprland_api_get_hash();
    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[dynamic-cursors] Failed to load, mismatched headers!", CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        HyprlandAPI::addNotification(PHANDLE, std::format("[dynamic-cursors] Built with: {}, running: {}", GIT_COMMIT_HASH, HASH), CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("version mismatch");
    }

    // setup config
    startConfig();

    addConfig(CONFIG_ENABLED, true);
    addConfig(CONFIG_MODE, "tilt");
    addConfig(CONFIG_THRESHOLD, 2);

    addConfig(CONFIG_SHAKE, true);
    addConfig(CONFIG_SHAKE_NEAREST, true);
    addConfig(CONFIG_SHAKE_EFFECTS, false);
    addConfig(CONFIG_SHAKE_IPC, false);
    addConfig(CONFIG_SHAKE_THRESHOLD, 6.0f);
    addConfig(CONFIG_SHAKE_BASE, 4.0F);
    addConfig(CONFIG_SHAKE_SPEED, 4.0F);
    addConfig(CONFIG_SHAKE_INFLUENCE, 0.0F);
    addConfig(CONFIG_SHAKE_LIMIT, 0.0F);
    addConfig(CONFIG_SHAKE_TIMEOUT, 2000);

    addShapeConfig(CONFIG_TILT_FUNCTION, "negative_quadratic");
    addShapeConfig(CONFIG_TILT_LIMIT, 5000);

    addShapeConfig(CONFIG_STRETCH_FUNCTION, "negative_quadratic");
    addShapeConfig(CONFIG_STRETCH_LIMIT, 3000);

    addShapeConfig(CONFIG_ROTATE_LENGTH, 20);
    addShapeConfig(CONFIG_ROTATE_OFFSET, 0.0f);

    addConfig(CONFIG_HW_DEBUG, false);

    addRulesConfig();
    finishConfig();

    // init things
    g_pDynamicCursors = std::make_unique<CDynamicCursors>();

    // try hooking
    try {
        g_pRenderSoftwareCursorsForHook = hook("renderSoftwareCursorsFor", "CPointerManager", (void*) &hkRenderSoftwareCursorsFor);
        g_pDamageIfSoftwareHook = hook("damageIfSoftware", "CPointerManager", (void*) &hkDamageIfSoftware);
        g_pRenderHWCursorBufferHook = hook("renderHWCursorBuffer", "CPointerManager", (void*) &hkRenderHWCursorBuffer);
        g_pSetHWCursorBufferHook = hook("setHWCursorBuffer", "CPointerManager", (void*) &hkSetHWCursorBuffer);
        g_pOnCursorMovedHook = hook("onCursorMoved", "CPointerManager", (void*) &hkOnCursorMoved);

        g_pSetCursorFromNameHook = hook("setCursorFromName", "CCursorManager", (void*) &hkSetCursorFromName);
        g_pSetCursorSurfaceHook = hook("setCursorSurface", "CCursorManager", (void*) &hkSetCursorSurface);
    } catch (...) {
        HyprlandAPI::addNotification(PHANDLE, "[dynamic-cursors] Failed to load, hooks could not be made!", CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("hooks failed");
    }

    return {"dynamic-cursors", "a plugin to make your hyprland cursor more realistic, also adds shake to find", "Virt", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() { }

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
