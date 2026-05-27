#include <hyprland/src/config/lua/bindings/LuaBindingsInternal.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprlang.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include "../globals.hpp"
#include "../cursor.hpp"
#include "ShapeRule.hpp"
#include "ConfigManager.hpp"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

CConfigHandler::CConfigHandler() {
    m_shapeRules = makeUnique<CShapeRuleHandler>();

    // add config variables
    // clang-format off
    c_enabled         = conf(NS("enabled"),                true,                   "global toggle for the plugin");
    c_mode            = prop(NS("mode"),                   "tilt",                 "sets the cursor behaviour (tilt, rotate, stretch, none)");
    c_threshold       = conf(NS("threshold"),              2,                      "minimum angle difference in degrees after which the shape is changed");

    c_shakeEnabled    = conf(NS("shake:enabled"),          true,                   "enables shake to find");
    c_shakeEffects    = conf(NS("shake:effects"),          false,                  "show cursor behaviour while shaking");
    c_shakeIPC        = conf(NS("shake:ipc"),              false,                  "enable ipc events for shake");
    c_shakeThreshold  = conf(NS("shake:threshold"),        6.0f,                   "controls how soon a shake is detected");
    c_shakeBase       = conf(NS("shake:base"),             4.0f,                   "magnification level immediately after shake start");
    c_shakeSpeed      = conf(NS("shake:speed"),            4.0f,                   "magnification increase per second when continuing to shake");
    c_shakeInfluence  = conf(NS("shake:influence"),        0.0f,                   "how much the speed is influenced by the current shake intensity");
    c_shakeLimit      = conf(NS("shake:limit"),            0.0f,                   "maximal magnification the cursor can reach");
    c_shakeTimeout    = conf(NS("shake:timeout"),          2000,                   "time in milliseconds the cursor will stay magnified after a shake has ended");

    c_highresEnabled  = conf(NS("hyprcursor:enabled"),     true,                   "enable dedicated hyprcursor support");
    c_highresNearest  = conf(NS("hyprcursor:nearest"),     1,                      "use nearest-neighbour scaling when magnifying beyond texture size");
    c_highresFallback = conf(NS("hyprcursor:fallback"),    "clientside",           "shape to use when clientside cursors are being magnified");
    c_highresSize     = conf(NS("hyprcursor:resolution"),  -1,                     "resolution in pixels to load the magnified shapes at");

    c_tiltFunction    = prop(NS("tilt:activation"),        "negative_quadratic",   "relationship between speed and tilt (linear, quadratic, negative_quadratic)");
    c_tiltLimit       = prop(NS("tilt:limit"),             5000,                   "controls at which speed the full tilt is reached");
    c_tiltWindow      = prop(NS("tilt:window"),            100,                    "time window over which the speed is calculated");
    c_tiltFull        = prop(NS("tilt:full"),              60,                     "full tilt for each side in degrees");

    c_stretchFunction = prop(NS("stretch:activation"),     "negative_quadratic",   "relationship between speed and stretch amount (linear, quadratic, negative_quadratic)");
    c_stretchLimit    = prop(NS("stretch:limit"),          3000,                   "controls at which speed the full stretch is reached");
    c_stretchWindow   = prop(NS("stretch:window"),         100,                    "time window over which the speed is calculated");

    c_rotateLength    = prop(NS("rotate:length"),          20,                     "length in px of the simulated stick used to rotate the cursor");
    c_rotateOffset    = prop(NS("rotate:offset"),          0.0f,                   "clockwise offset applied to the angle in degrees");

    c_hwDebug         = conf(NS("hw_debug"),               false,                  "enable hardware debug mode");
    c_ignoreWarps     = conf(NS("ignore_warps"),           true,                   "ignore cursor warps");
    // clang-format on

    // add shape rule handlers
    HyprlandAPI::addConfigKeyword(PHANDLE, "shaperule", onShapeRuleKeyword, Hyprlang::SHandlerOptions{});
    HyprlandAPI::addLuaFunction(PHANDLE, "dynamic_cursors", "shape_rule", ::luaShapeRule);

    // clear shape rules on reload
    static const auto LISTENER = Event::bus()->m_events.config.preReload.listen([&]() -> void { m_shapeRules->clear(); });

    // add magnify dispatcher
    HyprlandAPI::addDispatcherV2(PHANDLE, NS("magnify"), ::dispatchMagnify);
    HyprlandAPI::addLuaFunction(PHANDLE, "dynamic_cursors", "dsp_magnify", ::luaMagnifyDispatcher);
}

bool CConfigHandler::isEnabled() {
    // make sure the compositor is properly initialized
    return c_enabled->value() && g_pHyprRenderer->m_mostHzMonitor && g_pDynamicCursors;
}

SP<CBoolValue> CConfigHandler::conf(const char* name, bool def, const char* desc) {
    auto val = makeShared<CBoolValue>(name, desc, def);
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    return val;
}

SP<CIntValue> CConfigHandler::conf(const char* name, int def, const char* desc) {
    auto val = makeShared<CIntValue>(name, desc, def);
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    return val;
}

SP<CStringValue> CConfigHandler::conf(const char* name, const char* def, const char* desc) {
    auto val = makeShared<CStringValue>(name, desc, std::string{def});
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    return val;
}

SP<CFloatValue> CConfigHandler::conf(const char* name, float def, const char* desc) {
    auto val = makeShared<CFloatValue>(name, desc, def);
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    return val;
}

SP<CBoolProp> CConfigHandler::prop(const char* name, bool def, const char* desc) {
    auto val = makeShared<CBoolValue>(name, desc, def);
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    auto prop = makeShared<CBoolProp>(val);
    prop->setHandler(m_shapeRules);
    m_shapeRules->registerProp(prop);

    return prop;
}

SP<CIntProp> CConfigHandler::prop(const char* name, int def, const char* desc) {
    auto val = makeShared<CIntValue>(name, desc, def);
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    auto prop = makeShared<CIntProp>(val);
    prop->setHandler(m_shapeRules);
    m_shapeRules->registerProp(prop);

    return prop;
}

SP<CStringProp> CConfigHandler::prop(const char* name, const char* def, const char* desc) {
    auto val = makeShared<CStringValue>(name, desc, std::string{def});
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    auto prop = makeShared<CStringProp>(val);
    prop->setHandler(m_shapeRules);
    m_shapeRules->registerProp(prop);

    return prop;
}

SP<CFloatProp> CConfigHandler::prop(const char* name, float def, const char* desc) {
    auto val = makeShared<CFloatValue>(name, desc, def);
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    auto prop = makeShared<CFloatProp>(val);
    prop->setHandler(m_shapeRules);
    m_shapeRules->registerProp(prop);

    return prop;
}

SDispatchResult dispatchMagnify(std::string in) {
    Hyprutils::String::CVarList args = in;

    SDispatchResult result;

    std::optional<int>   duration;
    std::optional<float> size;

    try {
        auto it = args.begin();
        if (it != args.end() && *it != "") {
            duration = std::stoi(*it);

            it++;
            if (it != args.end())
                size = std::stof(*it);
        }
    } catch (...) {
        result.error = "invalid types for arguments";
        Log::logger->log(Log::ERR, "[dynamic-cursors] dispatcher `magnify` received invalid args: {}", in);
    }

    g_pDynamicCursors->dispatchMagnify(duration, size);

    return result;
}

int luaMagnifyDispatcher(lua_State* L) {
    if (!lua_istable(L, 1))
        return Config::Lua::Bindings::Internal::configError(L, "dsp_magnify: expected a table { duration, size }");

    std::optional<int>   duration;
    std::optional<float> size;

    {
        Hyprutils::Utils::CScopeGuard x([L] { lua_pop(L, 1); });

        lua_getfield(L, 1, "duration");

        if (!lua_isnil(L, -1)) {
            if (!lua_isinteger(L, -1))
                return Config::Lua::Bindings::Internal::configError(L, "dsp_magnify: `duration` must be an integer");

            duration = lua_tointeger(L, -1);
        }
    }

    {
        Hyprutils::Utils::CScopeGuard x([L] { lua_pop(L, 1); });

        lua_getfield(L, 1, "size");

        if (!lua_isnil(L, -1)) {
            if (!lua_isnumber(L, -1))
                return Config::Lua::Bindings::Internal::configError(L, "dsp_magnify: `size` must be a number");

            size = lua_tonumber(L, -1);
        }
    }

    auto dispatch = [](lua_State* L) -> int {
        std::optional<int>   duration;
        std::optional<float> size;

        if (!lua_isnil(L, lua_upvalueindex(1)))
            duration = lua_tointeger(L, lua_upvalueindex(1));
        if (!lua_isnil(L, lua_upvalueindex(2)))
            size = lua_tonumber(L, lua_upvalueindex(2));

        g_pDynamicCursors->dispatchMagnify(duration, size);

        return 0;
    };

    if (duration.has_value())
        lua_pushinteger(L, duration.value());
    else
        lua_pushnil(L);

    if (size.has_value())
        lua_pushnumber(L, size.value());
    else
        lua_pushnil(L);

    lua_pushcclosure(L, dispatch, 2);

    return 1;
}
