#include "../globals.hpp"
#include <sstream> // required so we don't "unprivate" sstream before including cursor.hpp
#include <hyprland/src/managers/eventLoop/EventLoopTimer.hpp> // required so we don't "unprivate" chrono before including cursor.hpp
#include "../cursor.hpp"
#include "config.hpp"

#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprlang.hpp>

void addDispatcher(std::string name, std::function<std::optional<std::string>(Hyprutils::String::CVarList)> handler) {
    HyprlandAPI::addDispatcherV2(PHANDLE, NS("") + name, [=](std::string in) {
        auto error = handler(Hyprutils::String::CVarList(in));

        SDispatchResult result;
        if (error.has_value()) {
            Log::logger->log(Log::ERR, "[dynamic-cursors] dispatcher {} received invalid args: {}", name, error.value());
            result.error = error.value();
        }

        return result;
    });
}

CConfigHandler::CConfigHandler() {
    // add config variables
    c_enabled         = add(false, NS("enabled"),                true,                   "global toggle for the plugin");
    c_mode            = add(false, NS("mode"),                   "tilt",                 "sets the cursor behaviour (tilt, rotate, stretch, none)");
    c_threshold       = add(false, NS("threshold"),              2,                      "minimum angle difference in degrees after which the shape is changed");

    c_shakeEnabled    = add(false, NS("shake:enabled"),          true,                   "enables shake to find");
    c_shakeEffects    = add(false, NS("shake:effects"),          false,                  "show cursor behaviour while shaking");
    c_shakeIPC        = add(false, NS("shake:ipc"),              false,                  "enable ipc events for shake");
    c_shakeThreshold  = add(false, NS("shake:threshold"),        6.0f,                   "controls how soon a shake is detected");
    c_shakeBase       = add(false, NS("shake:base"),             4.0f,                   "magnification level immediately after shake start");
    c_shakeSpeed      = add(false, NS("shake:speed"),            4.0f,                   "magnification increase per second when continuing to shake");
    c_shakeInfluence  = add(false, NS("shake:influence"),        0.0f,                   "how much the speed is influenced by the current shake intensity");
    c_shakeLimit      = add(false, NS("shake:limit"),            0.0f,                   "maximal magnification the cursor can reach");
    c_shakeTimeout    = add(false, NS("shake:timeout"),          2000,                   "time in milliseconds the cursor will stay magnified after a shake has ended");

    c_highresEnabled  = add(false, NS("hyprcursor:enabled"),     true,                   "enable dedicated hyprcursor support");
    c_highresNearest  = add(false, NS("hyprcursor:nearest"),     1,                      "use nearest-neighbour scaling when magnifying beyond texture size");
    c_highresFallback = add(false, NS("hyprcursor:fallback"),    "clientside",           "shape to use when clientside cursors are being magnified");
    c_highresSize     = add(false, NS("hyprcursor:resolution"),  -1,                     "resolution in pixels to load the magnified shapes at");

    c_tiltFunction    = add(true,  NS("tilt:function"),          "negative_quadratic",   "relationship between speed and tilt (linear, quadratic, negative_quadratic)");
    c_tiltLimit       = add(true,  NS("tilt:limit"),             5000,                   "controls at which speed the full tilt is reached");
    c_tiltWindow      = add(true,  NS("tilt:window"),            100,                    "time window over which the speed is calculated");
    c_tiltFull        = add(true,  NS("tilt:full_tilt"),         60,                     "full tilt for each side in degrees");

    c_stretchFunction = add(true,  NS("stretch:function"),       "negative_quadratic",   "relationship between speed and stretch amount");
    c_stretchLimit    = add(true,  NS("stretch:limit"),          3000,                   "controls at which speed the full stretch is reached");
    c_stretchWindow   = add(true,  NS("stretch:window"),         100,                    "time window over which the speed is calculated");

    c_rotateLength    = add(true,  NS("rotate:length"),          20,                     "length in px of the simulated stick used to rotate the cursor");
    c_rotateOffset    = add(true,  NS("rotate:offset"),          0.0f,                   "clockwise offset applied to the angle in degrees");

    c_hwDebug         = add(false, NS("hw_debug"),               false,                  "enable hardware debug mode");
    c_ignoreWarps     = add(false, NS("ignore_warps"),           true,                   "ignore cursor warps");

    // add legacy shape rule keyword
    HyprlandAPI::addConfigKeyword(PHANDLE, "shaperule", onShapeRuleKeyword, Hyprlang::SHandlerOptions {});
    static const auto LISTENER = Event::bus()->m_events.config.preReload.listen([&]() -> void {
        m_shapeRules.clear();
    });
}

bool CConfigHandler::isEnabled() {
    // make sure the compositor is properly initialized
    return c_enabled->value() && g_pHyprRenderer->m_mostHzMonitor && g_pDynamicCursors;
}

SP<CBoolValue> CConfigHandler::add(bool rule, const char* name, bool def, const char* desc) {
    auto val = makeShared<CBoolValue>(name, desc, def);
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    m_shapeRules.addProperty(name + NS_LEN, EShapeRuleType::INT); // TODO: support bools eventually

    return val;
}

SP<CIntValue> CConfigHandler::add(bool rule, const char* name, int def, const char* desc) {
    auto val = makeShared<CIntValue>(name, desc, def);
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    m_shapeRules.addProperty(name + NS_LEN, EShapeRuleType::INT);

    return val;
}

SP<CStringValue> CConfigHandler::add(bool rule, const char* name, const char* def, const char* desc) {
    auto val = makeShared<CStringValue>(name, desc, std::string {def});
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    m_shapeRules.addProperty(name + NS_LEN, EShapeRuleType::STRING);

    return val;
}

SP<CFloatValue> CConfigHandler::add(bool rule, const char* name, float def, const char* desc) {
    auto val = makeShared<CFloatValue>(name, desc, def);
    HyprlandAPI::addConfigValueV2(PHANDLE, val);

    m_shapeRules.addProperty(name + NS_LEN, EShapeRuleType::FLOAT);

    return val;
}
