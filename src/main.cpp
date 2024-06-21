#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprlang.hpp>
#include <unistd.h>

#include "globals.hpp"
#include "cursor.hpp"

typedef void (*origRenderSofwareCursorsFor)(void*, SP<CMonitor>, timespec*, CRegion&, std::optional<Vector2D>);
inline CFunctionHook* g_pRenderSoftwareCursorsForHook = nullptr;
void hkRenderSoftwareCursorsFor(void* thisptr, SP<CMonitor> pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos) {
    g_pDynamicCursors->renderSoftware((CPointerManager*) thisptr, pMonitor, now, damage, overridePos);
}

typedef void (*origDamageIfSoftware)(void*);
inline CFunctionHook* g_pDamageIfSoftwareHook = nullptr;
void hkDamageIfSoftware(void* thisptr) {
    g_pDynamicCursors->damageSoftware((CPointerManager*) thisptr);
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

    static const auto RENDER_SOFTWARE_CURSORS_FOR_METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "renderSoftwareCursorsFor");
    g_pRenderSoftwareCursorsForHook = HyprlandAPI::createFunctionHook(PHANDLE, RENDER_SOFTWARE_CURSORS_FOR_METHODS[0].address, (void*) &hkRenderSoftwareCursorsFor);
    g_pRenderSoftwareCursorsForHook->hook();

    static const auto DAMAGE_IF_SOFTWARE_METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "damageIfSoftware");
    g_pDamageIfSoftwareHook = HyprlandAPI::createFunctionHook(PHANDLE, DAMAGE_IF_SOFTWARE_METHODS[0].address, (void*) &hkDamageIfSoftware);
    g_pDamageIfSoftwareHook->hook();

    HyprlandAPI::addConfigValue(PHANDLE, CONFIG_LENGTH, Hyprlang::INT{20});

    return {"dynamic-cursors", "The most stupid cursor plugin.", "Virt", "1.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    // ...
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
