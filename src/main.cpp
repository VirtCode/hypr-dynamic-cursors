#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprlang.hpp>
#include <unistd.h>

#include "globals.hpp"
#include "cursor.hpp"
#include "invert/shader.hpp"
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

int onTick(void* data) {
    static auto* const* PENABLED = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_ENABLED)->getDataStaticPtr();

    if (**PENABLED) g_pDynamicCursors->onTick(g_pPointerManager.get());

    const int TIMEOUT = g_pHyprRenderer->m_pMostHzMonitor ? 1000.0 / g_pHyprRenderer->m_pMostHzMonitor->refreshRate : 16;
    wl_event_source_timer_update(tick, TIMEOUT);

    return 0;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    // check that header version aligns with running version
    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[dynamic-cursors] Mismatched headers! Can't proceed.", CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[dynamic-cursors] Version mismatch");
    }

    g_pDynamicCursors = std::make_unique<CDynamicCursors>();
    g_pShaders = std::make_unique<CShaders>();

    static const auto RENDER_SOFTWARE_CURSORS_FOR_METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "renderSoftwareCursorsFor");
    g_pRenderSoftwareCursorsForHook = HyprlandAPI::createFunctionHook(PHANDLE, RENDER_SOFTWARE_CURSORS_FOR_METHODS[0].address, (void*) &hkRenderSoftwareCursorsFor);
    g_pRenderSoftwareCursorsForHook->hook();

    static const auto DAMAGE_IF_SOFTWARE_METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "damageIfSoftware");
    g_pDamageIfSoftwareHook = HyprlandAPI::createFunctionHook(PHANDLE, DAMAGE_IF_SOFTWARE_METHODS[0].address, (void*) &hkDamageIfSoftware);
    g_pDamageIfSoftwareHook->hook();

    static const auto RENDER_HW_CURSOR_BUFFER_METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "renderHWCursorBuffer");
    g_pRenderHWCursorBufferHook = HyprlandAPI::createFunctionHook(PHANDLE, RENDER_HW_CURSOR_BUFFER_METHODS[0].address, (void*) &hkRenderHWCursorBuffer);
    g_pRenderHWCursorBufferHook->hook();

    static const auto SET_HW_CURSOR_BUFFER_METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "setHWCursorBuffer");
    g_pSetHWCursorBufferHook = HyprlandAPI::createFunctionHook(PHANDLE, SET_HW_CURSOR_BUFFER_METHODS[0].address, (void*) &hkSetHWCursorBuffer);
    g_pSetHWCursorBufferHook->hook();

    static const auto ON_CURSOR_MOVED_METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "onCursorMoved");
    g_pOnCursorMovedHook = HyprlandAPI::createFunctionHook(PHANDLE, ON_CURSOR_MOVED_METHODS[0].address, (void*) &hkOnCursorMoved);
    g_pOnCursorMovedHook->hook();

    // for some damn reason this tick handler does not work
    // static auto P_TICK = HyprlandAPI::registerCallbackDynamic(PHANDLE, "tick", [](void* self, SCallbackInfo& info, std::any data) { std::cout << "ticking?" << "\n"; });
    // so we have to use this (stolen from hyprtrails)
    tick = wl_event_loop_add_timer(g_pCompositor->m_sWLEventLoop, &onTick, nullptr);
    wl_event_source_timer_update(tick, 1);

    static auto P_PRE_RENDER = HyprlandAPI::registerCallbackDynamic(PHANDLE, "preRender", [](void* self, SCallbackInfo& info, std::any data) {
        static auto* const* PENABLED = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, CONFIG_ENABLED)->getDataStaticPtr();
        if (**PENABLED) g_pDynamicCursors->beforeRender(g_pPointerManager.get());
    });

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_ENABLED, Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_MODE, Hyprlang::STRING{"tilt"});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_THRESHOLD, Hyprlang::INT{2});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_OPACITY, Hyprlang::FLOAT{1});

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE, Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_NEAREST, Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_EFFECTS, Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_IPC, Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_THRESHOLD, Hyprlang::FLOAT{4});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_SHAKE_FACTOR, Hyprlang::FLOAT{1.5});

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_INVERT, Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_INVERT_SHADER, Hyprlang::STRING{"invert"});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_INVERT_CHROMA, Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_INVERT_CHROMA_COLOR, Hyprlang::INT{0xFF000000}); // opaque black

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_FUNCTION, Hyprlang::STRING{"negative_quadratic"});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_MASS, Hyprlang::INT{5000});

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_LENGTH, Hyprlang::INT{20});
    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_ROTATE_OFFSET, Hyprlang::FLOAT{0});

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_HW_DEBUG, Hyprlang::INT{0});

    HyprlandAPI::reloadConfig();

    return {"dynamic-cursors", "make your cursor more realistic", "Virt", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    // ...
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
