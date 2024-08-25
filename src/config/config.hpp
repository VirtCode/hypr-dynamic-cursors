#pragma once

#include <hyprlang.hpp>
#include "ShapeRule.hpp"

#define NAMESPACE "plugin:dynamic-cursors:"

#define CONFIG_ENABLED          "enabled"
#define CONFIG_MODE             "mode"
#define CONFIG_THRESHOLD        "threshold"
#define CONFIG_HW_DEBUG         "hw_debug"
#define CONFIG_IGNORE_WARPS     "ignore_warps"

#define CONFIG_SHAKE            "shake:enabled"
#define CONFIG_SHAKE_NEAREST    "shake:nearest"
#define CONFIG_SHAKE_EFFECTS    "shake:effects"
#define CONFIG_SHAKE_IPC        "shake:ipc"
#define CONFIG_SHAKE_THRESHOLD  "shake:threshold"
#define CONFIG_SHAKE_SPEED      "shake:speed"
#define CONFIG_SHAKE_INFLUENCE  "shake:influence"
#define CONFIG_SHAKE_BASE       "shake:base"
#define CONFIG_SHAKE_LIMIT      "shake:limit"
#define CONFIG_SHAKE_TIMEOUT    "shake:timeout"

#define CONFIG_ROTATE_LENGTH    "rotate:length"
#define CONFIG_ROTATE_OFFSET    "rotate:offset"

#define CONFIG_TILT_LIMIT       "tilt:limit"
#define CONFIG_TILT_FUNCTION    "tilt:function"

#define CONFIG_STRETCH_LIMIT    "stretch:limit"
#define CONFIG_STRETCH_FUNCTION "stretch:function"

#define CONFIG_SHAPERULE         "shaperule"

/* initializes stuff so config can be set up */
void startConfig();
/* finishes config setup */
void finishConfig();

/* add shaperule config entry */
void addRulesConfig();

/* will add an ordinary config value */
void addConfig(std::string name, std::variant<std::string, float, int> value);

/* will add a config variable which is also a property for shape rules */
void addShapeConfig(std::string name, std::variant<std::string, float, int> value);

/* get static pointer to config value */
void* const* getConfig(std::string name);
