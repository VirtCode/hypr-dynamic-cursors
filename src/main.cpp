#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <unistd.h>

#include "globals.hpp"
#include "render.hpp"

typedef void (*origRenderSofwareCursorsFor)(void*, SP<CMonitor>, timespec*, CRegion&, std::optional<Vector2D>);
inline CFunctionHook* g_pRenderSoftwareCursorsForHook = nullptr;

void hkRenderSoftwareCursorsFor(void* thisptr, SP<CMonitor> pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos) {
    g_pDynamicCursors->render((CPointerManager*) thisptr, pMonitor, now, damage, overridePos);
    //(*(origRenderSofwareCursorsFor)g_pRenderSoftwareCursorsForHook->m_pOriginal)(thisptr, pMonitor, now, damage, overridePos);
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    // check that header version aligns with running version
    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[dynamic-cursors] Mismatched headers! Can't proceed.", CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[dynamic-cursors] Version mismatch");
    }

    static const auto METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "renderSoftwareCursorsFor");
    g_pRenderSoftwareCursorsForHook = HyprlandAPI::createFunctionHook(PHANDLE, METHODS[0].address, (void*) &hkRenderSoftwareCursorsFor);
    g_pRenderSoftwareCursorsForHook->hook();

    g_pDynamicCursors = std::make_unique<CDynamicCursors>();

    return {"dynamic-cursors", "The most stupid cursor plugin.", "Virt", "1.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    // ...
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
