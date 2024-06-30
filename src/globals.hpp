#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

#define CONFIG_ENABLED              "plugin:dynamic-cursors:enabled"
#define CONFIG_MODE                 "plugin:dynamic-cursors:mode"
#define CONFIG_THRESHOLD            "plugin:dynamic-cursors:threshold"
#define CONFIG_HW_DEBUG             "plugin:dynamic-cursors:hw_debug"

#define CONFIG_LENGTH               "plugin:dynamic-cursors:rotate:length"
#define CONFIG_ROTATE_OFFSET        "plugin:dynamic-cursors:rotate:offset"

#define CONFIG_MASS                 "plugin:dynamic-cursors:tilt:limit"
#define CONFIG_FUNCTION             "plugin:dynamic-cursors:tilt:function"

#define CONFIG_SHAKE                "plugin:dynamic-cursors:shake"
#define CONFIG_SHAKE_NEAREST        "plugin:dynamic-cursors:shake:nearest"
#define CONFIG_SHAKE_THRESHOLD      "plugin:dynamic-cursors:shake:threshold"
#define CONFIG_SHAKE_FACTOR         "plugin:dynamic-cursors:shake:factor"
#define CONFIG_SHAKE_EFFECTS        "plugin:dynamic-cursors:shake:effects"
#define CONFIG_SHAKE_IPC            "plugin:dynamic-cursors:shake:ipc"

#define CONFIG_INVERT               "plugin:dynamic-cursors:invert"
#define CONFIG_INVERT_SHADER        "plugin:dynamic-cursors:invert:shader"
#define CONFIG_INVERT_CHROMA        "plugin:dynamic-cursors:invert:chroma"
#define CONFIG_INVERT_CHROMA_COLOR  "plugin:dynamic-cursors:invert:chroma:color"

inline HANDLE PHANDLE = nullptr;

inline wl_event_source* tick = nullptr;
