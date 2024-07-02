#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

#define CONFIG_ENABLED          "plugin:dynamic-cursors:enabled"
#define CONFIG_MODE             "plugin:dynamic-cursors:mode"
#define CONFIG_SHAKE            "plugin:dynamic-cursors:shake"
#define CONFIG_THRESHOLD        "plugin:dynamic-cursors:threshold"
#define CONFIG_SHAKE_NEAREST    "plugin:dynamic-cursors:shake:nearest"
#define CONFIG_SHAKE_THRESHOLD  "plugin:dynamic-cursors:shake:threshold"
#define CONFIG_SHAKE_FACTOR     "plugin:dynamic-cursors:shake:factor"
#define CONFIG_SHAKE_EFFECTS    "plugin:dynamic-cursors:shake:effects"
#define CONFIG_SHAKE_IPC        "plugin:dynamic-cursors:shake:ipc"
#define CONFIG_LENGTH           "plugin:dynamic-cursors:rotate:length"
#define CONFIG_ROTATE_OFFSET    "plugin:dynamic-cursors:rotate:offset"
#define CONFIG_MASS             "plugin:dynamic-cursors:tilt:limit"
#define CONFIG_FUNCTION         "plugin:dynamic-cursors:tilt:function"
#define CONFIG_STRETCH_LIMIT    "plugin:dynamic-cursors:stretch:limit"
#define CONFIG_STRETCH_FUNCTION "plugin:dynamic-cursors:stretch:function"

#define CONFIG_HW_DEBUG         "plugin:dynamic-cursors:hw_debug"

inline HANDLE PHANDLE = nullptr;
