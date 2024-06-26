#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

#define CONFIG_ENABLED   "plugin:dynamic-cursors:enabled"
#define CONFIG_MODE      "plugin:dynamic-cursors:mode"
#define CONFIG_THRESHOLD "plugin:dynamic-cursors:threshold"
#define CONFIG_LENGTH    "plugin:dynamic-cursors:rotate:length"
#define CONFIG_MASS      "plugin:dynamic-cursors:tilt:limit"
#define CONFIG_FUNCTION  "plugin:dynamic-cursors:tilt:function"

#define CONFIG_HW_DEBUG "plugin:dynamic-cursors:hw_debug"

inline HANDLE PHANDLE = nullptr;

inline wl_event_source* tick = nullptr;
