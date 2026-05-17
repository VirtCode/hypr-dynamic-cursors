#pragma once

#include "ShapeRule.hpp"
#include "SharedDefs.hpp"
#include "rule/BoolProp.hpp"
#include "rule/FloatProp.hpp"
#include "rule/IntProp.hpp"
#include "rule/StringProp.hpp"

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

class CConfigHandler {
public:
    SP<CBoolValue>      c_enabled;
    SP<CStringProp>     c_mode;
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

    SP<CStringProp>     c_tiltFunction;
    SP<CIntProp>        c_tiltLimit;
    SP<CIntProp>        c_tiltWindow;
    SP<CIntProp>        c_tiltFull;

    SP<CStringProp>     c_stretchFunction;
    SP<CIntProp>        c_stretchLimit;
    SP<CIntProp>        c_stretchWindow;

    SP<CIntProp>        c_rotateLength;
    SP<CFloatProp>      c_rotateOffset;

    SP<CBoolValue>      c_hwDebug;
    SP<CBoolValue>      c_ignoreWarps;

    UP<CShapeRuleHandler> m_shapeRules;

    /* create and initialize the config handler */
    CConfigHandler();

    /* whether the plugin is enabled */
    bool isEnabled();

private:
    SP<CBoolValue>   conf(const char* name, bool def, const char* desc);
    SP<CIntValue>    conf(const char* name, int def, const char* desc);
    SP<CStringValue> conf(const char* name, const char* def, const char* desc);
    SP<CFloatValue>  conf(const char* name, float def, const char* desc);

    SP<CBoolProp>   prop(const char* name, bool def, const char* desc);
    SP<CIntProp>    prop(const char* name, int def, const char* desc);
    SP<CStringProp> prop(const char* name, const char* def, const char* desc);
    SP<CFloatProp>  prop(const char* name, float def, const char* desc);
};

inline UP<CConfigHandler> g_pConfigHandler;

/* the callback for the magnify dispatcher */
SDispatchResult dispatchMagnify(std::string args);

/* lua magnify dispatcher factory */
int luaMagnifyDispatcher(lua_State* L);
