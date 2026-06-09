#include <hyprgraphics/color/Color.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/managers/CursorManager.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/helpers/time/Time.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprlang.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <hyprutils/string/String.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <dlfcn.h>

#include "globals.hpp"
#include "cursor.hpp"
#include "config/ConfigManager.hpp"

typedef void (*origRenderSoftwareCursorsFor)(void*, SP<CMonitor>, const Time::steady_tp&, CRegion&, std::optional<Vector2D>, bool);
inline CFunctionHook* g_pRenderSoftwareCursorsForHook = nullptr;
void hkRenderSoftwareCursorsFor(void* thisptr, SP<CMonitor> pMonitor, const Time::steady_tp& now, CRegion& damage, std::optional<Vector2D> overridePos, bool forceRender) {
    if (g_pConfigHandler->isEnabled())
        g_pDynamicCursors->renderSoftware((CPointerManager*)thisptr, pMonitor, now, damage, overridePos, forceRender);
    else
        (*(origRenderSoftwareCursorsFor)g_pRenderSoftwareCursorsForHook->m_original)(thisptr, pMonitor, now, damage, overridePos, forceRender);
}

typedef void (*origDamageIfSoftware)(void*);
inline CFunctionHook* g_pDamageIfSoftwareHook = nullptr;
void                  hkDamageIfSoftware(void* thisptr) {
    if (g_pConfigHandler->isEnabled())
        g_pDynamicCursors->damageSoftware((CPointerManager*)thisptr);
    else
        (*(origDamageIfSoftware)g_pDamageIfSoftwareHook->m_original)(thisptr);
}

typedef SP<Aquamarine::IBuffer> (*origRenderHWCursorBuffer)(void*, SP<CPointerManager::SMonitorPointerState>, SP<Render::ITexture>);
inline CFunctionHook*   g_pRenderHWCursorBufferHook = nullptr;
SP<Aquamarine::IBuffer> hkRenderHWCursorBuffer(void* thisptr, SP<CPointerManager::SMonitorPointerState> state, SP<Render::ITexture> texture) {
    if (g_pConfigHandler->isEnabled())
        return g_pDynamicCursors->renderHardware((CPointerManager*)thisptr, state, texture);
    else
        return (*(origRenderHWCursorBuffer)g_pRenderHWCursorBufferHook->m_original)(thisptr, state, texture);
}

typedef bool (*origSetHWCursorBuffer)(void*, SP<CPointerManager::SMonitorPointerState>, SP<Aquamarine::IBuffer>);
inline CFunctionHook* g_pSetHWCursorBufferHook = nullptr;
bool                  hkSetHWCursorBuffer(void* thisptr, SP<CPointerManager::SMonitorPointerState> state, SP<Aquamarine::IBuffer> buffer) {
    if (g_pConfigHandler->isEnabled())
        return g_pDynamicCursors->setHardware((CPointerManager*)thisptr, state, buffer);
    else
        return (*(origSetHWCursorBuffer)g_pSetHWCursorBufferHook->m_original)(thisptr, state, buffer);
}

typedef void (*origOnCursorMoved)(void*);
inline CFunctionHook* g_pOnCursorMovedHook = nullptr;
void                  hkOnCursorMoved(void* thisptr) {
    if (g_pConfigHandler->isEnabled())
        return g_pDynamicCursors->onCursorMoved((CPointerManager*)thisptr);
    else
        return (*(origOnCursorMoved)g_pOnCursorMovedHook->m_original)(thisptr);
}

typedef void (*origSetCursorFromName)(void*, const std::string& name);
inline CFunctionHook* g_pSetCursorFromNameHook = nullptr;
void                  hkSetCursorFromName(void* thisptr, const std::string& name) {
    if (g_pConfigHandler->isEnabled())
        g_pDynamicCursors->setShape(name);
    (*(origSetCursorFromName)g_pSetCursorFromNameHook->m_original)(thisptr, name);
}

typedef void (*origSetCursorSurface)(void*, SP<Desktop::View::CWLSurface>, const Vector2D&);
inline CFunctionHook* g_pSetCursorSurfaceHook = nullptr;
void                  hkSetCursorSurface(void* thisptr, SP<Desktop::View::CWLSurface> surf, const Vector2D& hotspot) {
    if (g_pConfigHandler->isEnabled())
        g_pDynamicCursors->unsetShape();
    (*(origSetCursorSurface)g_pSetCursorSurfaceHook->m_original)(thisptr, surf, hotspot);
}

typedef void (*origMove)(void*, const Vector2D&);
inline CFunctionHook* g_pMoveHook = nullptr;
void                  hkMove(void* thisptr, const Vector2D& deltaLogical) {
    if (g_pConfigHandler->isEnabled())
        g_pDynamicCursors->setMove();
    (*(origMove)g_pMoveHook->m_original)(thisptr, deltaLogical);
}

typedef void (*origUpdateTheme)(void*);
inline CFunctionHook* g_pUpdateThemeHook = nullptr;
void                  hkUpdateTheme(void* thisptr) {
    (*(origUpdateTheme)g_pUpdateThemeHook->m_original)(thisptr);
    if (g_pConfigHandler->isEnabled())
        g_pDynamicCursors->updateTheme();
}

// takes a member function pointer and returns the function address
// will throw an exception if the supplied function is virtual
// see https://itanium-cxx-abi.github.io/cxx-abi/abi.html#member-function-pointers
template <typename T>
void* pmf_address(T pmf) {
    struct PMF {
        uintptr_t ptr;
        ptrdiff_t adj;
    };

    static_assert(std::is_member_function_pointer_v<T>, "must be member function pointer");
    static_assert(sizeof(T) == sizeof(PMF), "sanity check: not itanium pmf representation?");

    auto representation = std::bit_cast<PMF>(pmf);

    // if it is a virtual function, it would be a vtable offset + 1, so this would catch it
    if (representation.ptr & 0x01) {
        Log::logger->log(Log::ERR, "[dynamic-cursors] aborting hook on virtual function pointer");
        throw std::runtime_error("unexpected virtual function, are you up-to-date?");
    }

    return (void*)representation.ptr;
}

/*
 * hooks a function hook, given a function address `target` and its symbol `signature` when compiled against libstdc++
 * the symbol is only used when compiled against libstdc++ to check that the function signature has not changed, on other libc++ impls it is ignored
 *
 * the target address is intended to stem from a function pointer, the benefit of which is that it doesn't compile if the function isn't found
 * (signature validation which is equally as important is still only done at load however)
 * to hook member functions, refer to `pmf_address` above
 */
CFunctionHook* hook(void* target, std::string signature, void* handler) {
    Log::logger->log(Log::INFO, "[dynamic-cursors] starting to hook for {} at {:p}", signature, target);

    Dl_info info = {};
    if (!dladdr(target, &info)) {
        Log::logger->log(Log::ERR, "[dynamic-cursors] aborting hook without associated symbol");
        throw std::runtime_error("symbol not available, are you up-to-date?");
    }

// when using libstdc++, we check that the symbol is the one we expect
// this makes sure that the function signature has not changed without us noticing
#ifdef __GLIBCXX__
    if (signature != info.dli_sname) {
        Log::logger->log(Log::ERR, "[dynamic-cursors] aborting hook, function symbol changed to {}", info.dli_sname);
        throw std::runtime_error("unexpected function signature, are you up-to-date?");
    }
#else
    Log::logger->log(Log::INFO, "[dynamic-cursors] unchecked symbol compiled against unknown libc++ is {}", info.dli_sname);
    Log::logger->log(Log::WARN, "[dynamic-cursors] hooking on unknown libc++, signatures are not checked, this might crash");
#endif

    auto hook = HyprlandAPI::createFunctionHook(PHANDLE, target, handler);

    Log::logger->log(Log::INFO, "[dynamic-cursors] checks passed, actually hooking");
    if (!hook->hook()) {
        Log::logger->log(Log::ERR, "[dynamic-cursors] could not hook, hooking failed");
        throw std::runtime_error("hooking failed, are you on x86_64?");
    }

    return hook;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    // check that header version aligns with running version
    const std::string COMPOSITOR_HASH = __hyprland_api_get_hash();
    const std::string CLIENT_HASH     = __hyprland_api_get_client_hash();
    if (COMPOSITOR_HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[dynamic-cursors] failed to load, version mismatch!", CHyprColor{1.0, 0.2, 0.2, 1.0}, 10000);
        throw std::runtime_error(std::format("version mismatch, built against: {}, running compositor: {}", CLIENT_HASH, COMPOSITOR_HASH));
    }

    // setup config
    g_pConfigHandler = makeUnique<CConfigHandler>();

    // init things
    g_pDynamicCursors = makeUnique<CDynamicCursors>();

    // try hooking
    try {
        // clang-format off
        g_pRenderSoftwareCursorsForHook = hook(
            pmf_address(&CPointerManager::renderSoftwareCursorsFor),
            "_ZN15CPointerManager24renderSoftwareCursorsForEN9Hyprutils6Memory14CSharedPointerI8CMonitorEERKNSt6chrono10time_pointINS5_3_V212steady_clockENS5_8durationIlSt5ratioILl1ELl1000000000EEEEEERNS0_4Math7CRegionESt8optionalINSG_8Vector2DEEb",
            (void*) &hkRenderSoftwareCursorsFor
        );
        g_pDamageIfSoftwareHook = hook(
            pmf_address(&CPointerManager::damageIfSoftware),
            "_ZN15CPointerManager16damageIfSoftwareEv",
            (void*) &hkDamageIfSoftware
        );
        g_pRenderHWCursorBufferHook = hook(
            pmf_address(&CPointerManager::renderHWCursorBuffer),
            "_ZN15CPointerManager20renderHWCursorBufferEN9Hyprutils6Memory14CSharedPointerINS_20SMonitorPointerStateEEENS2_IN6Render8ITextureEEE",
            (void*) &hkRenderHWCursorBuffer
        );
        g_pSetHWCursorBufferHook = hook(
            pmf_address(&CPointerManager::setHWCursorBuffer),
            "_ZN15CPointerManager17setHWCursorBufferEN9Hyprutils6Memory14CSharedPointerINS_20SMonitorPointerStateEEENS2_IN10Aquamarine7IBufferEEE",
            (void*) &hkSetHWCursorBuffer
        );
        g_pOnCursorMovedHook = hook(
            pmf_address(&CPointerManager::onCursorMoved),
            "_ZN15CPointerManager13onCursorMovedEv",
            (void*) &hkOnCursorMoved
        );
        g_pMoveHook = hook(
            pmf_address(&CPointerManager::move),
            "_ZN15CPointerManager4moveERKN9Hyprutils4Math8Vector2DE",
            (void*) &hkMove
        );

        g_pSetCursorFromNameHook = hook(
            pmf_address(&CCursorManager::setCursorFromName),
            "_ZN14CCursorManager17setCursorFromNameERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE",
            (void*) &hkSetCursorFromName
        );
        g_pSetCursorSurfaceHook = hook(
            pmf_address(&CCursorManager::setCursorSurface),
            "_ZN14CCursorManager16setCursorSurfaceEN9Hyprutils6Memory14CSharedPointerIN7Desktop4View10CWLSurfaceEEERKNS0_4Math8Vector2DE",
            (void*) &hkSetCursorSurface
        );
        g_pUpdateThemeHook = hook(
            pmf_address(&CCursorManager::updateTheme),
            "_ZN14CCursorManager11updateThemeEv",
            (void*) &hkUpdateTheme
        );
        // clang-format on
    } catch (std::exception& e) {
        Log::logger->log(Log::ERR, "[dynamic-cursors] failed to hook, {}", e.what());
        HyprlandAPI::addNotification(PHANDLE, std::format("[dynamic-cursors] cannot load, {}", e.what()), CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw e;
    } catch (...) {
        Log::logger->log(Log::ERR, "[dynamic-cursors] failed to hook for unknown reason");
        HyprlandAPI::addNotification(PHANDLE, "[dynamic-cursors] cannot load, unknown error with hooks!", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("hooks failed for unknown reason");
    }

    return {"dynamic-cursors", "a plugin to make your hyprland cursor more realistic, also adds shake to find", "Virt", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {

    // we need to remove our pass elements because otherwise we'll have some
    // invalid passes after unload, causing a SEGV
    g_pHyprRenderer->m_renderPass.removeAllOfType("CCursorPassElement");
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
