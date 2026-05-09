#pragma once

#include "ShapeRule.hpp"

#include <functional>
#include <hyprlang.hpp>
#include <hyprutils/string/VarList.hpp>

#include <hyprland/src/config/values/types/BoolValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>

#define NS(a) "plugin:dynamic-cursors:" a
#define NS_LEN 23

#define CONFIG(a) g_pConfigHandler->c_ ## a->value()

using namespace Config::Values;

/* adds a dispatcher */
void addDispatcher(std::string name, std::function<std::optional<std::string>(Hyprutils::String::CVarList)> handler);

class CConfigHandler {
public:
    SP<CBoolValue>      c_enabled;
    SP<CStringValue>    c_mode;
    SP<CIntValue>       c_threshold;

    SP<CBoolValue>      c_shakeEnabled;
    SP<CBoolValue>      c_shakeEffects;
    SP<CBoolValue>      c_shakeIPC;
    SP<CFloatValue>     c_shakeThreshold;
    SP<CFloatValue>     c_shakeBase;
    SP<CFloatValue>     c_shakeSpeed;
    SP<CFloatValue>     c_shakeInfluence;
    SP<CFloatValue>     c_shakeLimit;
    SP<CIntValue>       c_shakeTimeout;

    SP<CBoolValue>      c_highresEnabled;
    SP<CIntValue>       c_highresNearest;
    SP<CStringValue>    c_highresFallback;
    SP<CIntValue>       c_highresSize;

    SP<CStringValue>    c_tiltFunction;
    SP<CIntValue>       c_tiltLimit;
    SP<CIntValue>       c_tiltWindow;
    SP<CIntValue>       c_tiltFull;

    SP<CStringValue>    c_stretchFunction;
    SP<CIntValue>       c_stretchLimit;
    SP<CIntValue>       c_stretchWindow;

    SP<CIntValue>       c_rotateLength;
    SP<CFloatValue>     c_rotateOffset;

    SP<CBoolValue>      c_hwDebug;
    SP<CBoolValue>      c_ignoreWarps;

    CShapeRuleHandler   m_shapeRules;

    /* create and initialize the config handler */
    CConfigHandler();

    /* whether the plugin is enabled */
    bool isEnabled();

private:
    SP<CBoolValue>   add(bool rule, const char* name, bool def, const char* desc);
    SP<CIntValue>    add(bool rule, const char* name, int def, const char* desc);
    SP<CStringValue> add(bool rule, const char* name, const char* def, const char* desc);
    SP<CFloatValue>  add(bool rule, const char* name, float def, const char* desc);
};

inline UP<CConfigHandler> g_pConfigHandler;
