#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprlang.hpp>
#include <unistd.h>

#include "globals.hpp"
#include "cursor.hpp"
#include "src/debug/Log.hpp"
#include "src/managers/PointerManager.hpp"

typedef void (*origRenderSofwareCursorsFor)(void*, SP<CMonitor>, timespec*, CRegion&, std::optional<Vector2D>);
inline CFunctionHook* g_pRenderSoftwareCursorsForHook = nullptr;
void hkRenderSoftwareCursorsFor(void* thisptr, SP<CMonitor> pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos) {
    static auto* const* PENABLED = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_ENABLED)->getDataStaticPtr();

    if (**PENABLED) g_pDynamicCursors->renderSoftware((CPointerManager*) thisptr, pMonitor, now, damage, overridePos);
    else (*(origRenderSofwareCursorsFor)g_pRenderSoftwareCursorsForHook->m_pOriginal)(thisptr, pMonitor, now, damage, overridePos);
}

typedef void (*origDamageIfSoftware)(void*);
inline CFunctionHook* g_pDamageIfSoftwareHook = nullptr;
void hkDamageIfSoftware(void* thisptr) {
    static auto* const* PENABLED = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_ENABLED)->getDataStaticPtr();

    if (**PENABLED) g_pDynamicCursors->damageSoftware((CPointerManager*) thisptr);
    else (*(origDamageIfSoftware)g_pDamageIfSoftwareHook->m_pOriginal)(thisptr);
}

typedef wlr_buffer* (*origRenderHWCursorBuffer)(void*, SP<CPointerManager::SMonitorPointerState>, SP<CTexture>);
inline CFunctionHook* g_pRenderHWCursorBufferHook = nullptr;
wlr_buffer* hkRenderHWCursorBuffer(void* thisptr, SP<CPointerManager::SMonitorPointerState> state, SP<CTexture> texture) {
    static auto* const* PENABLED = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_ENABLED)->getDataStaticPtr();

    if (**PENABLED) return g_pDynamicCursors->renderHardware((CPointerManager*) thisptr, state, texture);
    else return (*(origRenderHWCursorBuffer)g_pRenderHWCursorBufferHook->m_pOriginal)(thisptr, state, texture);
}

typedef bool (*origSetHWCursorBuffer)(void*, SP<CPointerManager::SMonitorPointerState>, wlr_buffer*);
inline CFunctionHook* g_pSetHWCursorBufferHook = nullptr;
bool hkSetHWCursorBuffer(void* thisptr, SP<CPointerManager::SMonitorPointerState> state, wlr_buffer* buffer) {
    static auto* const* PENABLED = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_ENABLED)->getDataStaticPtr();

    if (**PENABLED) return g_pDynamicCursors->setHardware((CPointerManager*) thisptr, state, buffer);
    else return (*(origSetHWCursorBuffer)g_pSetHWCursorBufferHook->m_pOriginal)(thisptr, state, buffer);
}

typedef void (*origOnCursorMoved)(void*);
inline CFunctionHook* g_pOnCursorMovedHook = nullptr;
void hkOnCursorMoved(void* thisptr) {
    static auto* const* PENABLED = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_ENABLED)->getDataStaticPtr();

    if (**PENABLED) return g_pDynamicCursors->onCursorMoved((CPointerManager*) thisptr);
    else return (*(origOnCursorMoved)g_pOnCursorMovedHook->m_pOriginal)(thisptr);
}

/* hooks a function hook */
CFunctionHook* hook(std::string name, void* function) {
    auto names = HyprlandAPI::findFunctionsByName(PHANDLE, name);
    auto match = names.at(0);

    Debug::log(LOG, "[dynamic-cursors] hooking on {} for {}", match.signature, name);

    auto hook = HyprlandAPI::createFunctionHook(PHANDLE, match.address, function);
    hook->hook();

    return hook;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    // check that header version aligns with running version
    const std::string HASH = __hyprland_api_get_hash();
    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[dynamic-cursors] Failed to load, mismatched headers!", CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("version mismatch");
    }

    // setup config
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_ENABLED, Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_MODE, Hyprlang::STRING{"tilt"});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_THRESHOLD, Hyprlang::INT{2});

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE, Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_NEAREST, Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_EFFECTS, Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_IPC, Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_THRESHOLD, Hyprlang::FLOAT{4});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_FACTOR, Hyprlang::FLOAT{1.5});

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_TILT_FUNCTION, Hyprlang::STRING{"negative_quadratic"});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_TILT_LIMIT, Hyprlang::INT{5000});

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_STRETCH_FUNCTION, Hyprlang::STRING{"negative_quadratic"});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_STRETCH_LIMIT, Hyprlang::INT{3000});

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_ROTATE_LENGTH, Hyprlang::INT{20});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_ROTATE_OFFSET, Hyprlang::FLOAT{0});

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_HW_DEBUG, Hyprlang::INT{0});

    HyprlandAPI::reloadConfig();

    // init things
    g_pDynamicCursors = std::make_unique<CDynamicCursors>();

    // try hooking
    try {
        g_pRenderSoftwareCursorsForHook = hook("renderSoftwareCursorsFor", (void*) &hkRenderSoftwareCursorsFor);
        g_pDamageIfSoftwareHook = hook("damageIfSoftware", (void*) &hkDamageIfSoftware);
        g_pRenderHWCursorBufferHook = hook("renderHWCursorBuffer", (void*) &hkRenderHWCursorBuffer);
        g_pSetHWCursorBufferHook = hook("setHWCursorBuffer", (void*) &hkSetHWCursorBuffer);
        g_pOnCursorMovedHook = hook("onCursorMoved", (void*) &hkOnCursorMoved);
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
