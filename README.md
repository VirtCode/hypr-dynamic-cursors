# hypr-dynamic-cursors
This plugin makes your cursor more realistic by simulating how it would behave if it was an actual object being dragged across your screen. This means that your cursor can change based on how it is used, e.g. tilt in the direction you are moving or straight out rotate towards it.

Why did I implement this again?

Inspired by KDE, it also supports shake to find, to enlarge the cursor when it is shaken, so it is easier to find it. It can be enabled separately or together with one simulation mode.

### simulation modes

#### `rotate`
In this mode, the cursor is simulated as a stick which is dragged across the screen on one end. This means it will rotate towards the movement direction, and feels really realistic.

https://github.com/VirtCode/hypr-dynamic-cursor/assets/41426325/ccd6d742-8e2b-4073-a35e-318c7e19705c

#### `tilt`
In this mode, the cursor is tilted based on the `x` direction and speed it is moving at. It was intended to simulate how an object would be affected by air drag, but implemented is only a rough approximation. This mode can also be customized extensively with different activation functions, and is enabled by default.

https://github.com/VirtCode/hypr-dynamic-cursors/assets/41426325/ae25415c-e77f-4c85-864c-2eedbfe432e3

#### `stretch`
This mode tries recreating the stretching and squishing that is done to moving object in comic animations. It stretches your cursor in the direction you are moving based on the speed. Yes, this is not at all realistic.

https://github.com/VirtCode/hypr-dynamic-cursors/assets/41426325/7b8289e7-9dd2-4b57-b406-4fa28779a260

### shake to find
The plugin supports shake to find, akin to how KDE Plasma, MacOS, etc. do it. It can also be extensively configured and is enabled by default. It also supports using [hyprcursor](https://github.com/hyprwm/hyprcursor) for high resolution cursor images. The magnification can also be triggered as a dispatcher instead of on shake. If you only want shake to find, and no weird cursor behaviour, you can disable the above modes with the mode `none`.

https://github.com/user-attachments/assets/1346101e-4e62-4ba2-a1df-5940e0706514

## state
This plugin is still very early in its development. There are also multiple things which may or may not be implemented in the future:

- [X] software cursor rendering
- [X] hardware cursor rendering
- [X] stick simulation
- [X] air drag simulation
- [ ] pendulum simulation
- [X] per-shape length and starting angle (if possible)
- [X] cursor shake to find
- [X] overdue refactoring (wait for aquamarine merge)
- [X] hyprcursor magnified shape

If anything here sounds interesting to you, don't hesitate to contribute.

Please note that this plugin was created more or less as a joke. I mainly wanted to see how using a rotating or tilted cursor was like. So I will not guarantee any future updates and bugfixes. The only useful feature, shake to find, was implemented more or less as an afterthought.

## installation

### hyprpm

Installation is supported via `hyprpm`. Supported hyprland versions are `v0.41.2` and upwards. The main branch generally tries to target `-git`.

```sh
hyprpm add https://github.com/virtcode/hypr-dynamic-cursors
hyprpm enable dynamic-cursors
```

### NixOS

Add this flake to your inputs, note that using a mismatched/unsupported Hyprland release will fail to build or load the plugin:

```nix
inputs = {
    hyprland.url = "github:hyprwm/Hyprland"; # follows development branch of hyprland
    hypr-dynamic-cursors = {
        url = "github:VirtCode/hypr-dynamic-cursors";
        inputs.hyprland.follows = "hyprland"; # to make sure that the plugin is built for the correct version of hyprland
    };
};
```

Then, in your home-manager config, add the plugin:

```nix
wayland.windowManager.hyprland = {
    enable = true;
    plugins = [ inputs.hypr-dynamic-cursors.packages.${pkgs.system}.hypr-dynamic-cursors ];
};
```

or add it like this:

```nix
wayland.windowManager.hyprland = {
    enable = true;
    extraConfig = ''
        plugin = ${inputs.hypr-dynamic-cursors.packages.${pkgs.system}.hypr-dynamic-cursors}/lib/libhypr-dynamic-cursors.so
    '';
};
```

## configuration
This plugin can be configured in its dedicated configuration section (`plugin:dynamic-cursors`). The default values are shown below.
```ini
plugin:dynamic-cursors {

    # enables the plugin
    enabled = true

    # sets the cursor behaviour, supports these values:
    # tilt    - tilt the cursor based on x-velocity
    # rotate  - rotate the cursor based on movement direction
    # stretch - stretch the cursor shape based on direction and velocity
    # none    - do not change the cursors behaviour
    mode = tilt

    # minimum angle difference in degrees after which the shape is changed
    # smaller values are smoother, but more expensive for hw cursors
    threshold = 2

    # override the mode behaviour per shape
    # this is a keyword and can be repeated many times
    # by default, there are no rules added
    # see the dedicated `shape rules` section below!
    shaperule = <shape-name>, <mode> (optional), <property>: <value>, ...
    shaperule = <shape-name>, <mode> (optional), <property>: <value>, ...
    ...

    # for mode = rotate
    rotate {

        # length in px of the simulated stick used to rotate the cursor
        # most realistic if this is your actual cursor size
        length = 20

        # clockwise offset applied to the angle in degrees
        # this will apply to ALL shapes
        offset = 0.0
    }

    # for mode = tilt
    tilt {

        # controls how powerful the tilt is, the lower, the more power
        # this value controls at which speed (px/s) the full tilt is reached
        # the full tilt being 60° in both directions
        limit = 5000

        # relationship between speed and tilt, supports these values:
        # linear             - a linear function is used
        # quadratic          - a quadratic function is used (most realistic to actual air drag)
        # negative_quadratic - negative version of the quadratic one, feels more aggressive
        # see `activation` in `src/mode/utils.cpp` for how exactly the calculation is done
        function = negative_quadratic

        # time window (ms) over which the speed is calculated
        # higher values will make slow motions smoother but more delayed
        window = 100
    }

    # for mode = stretch
    stretch {

        # controls how much the cursor is stretched
        # this value controls at which speed (px/s) the full stretch is reached
        # the full stretch being twice the original length
        limit = 3000

        # relationship between speed and stretch amount, supports these values:
        # linear             - a linear function is used
        # quadratic          - a quadratic function is used
        # negative_quadratic - negative version of the quadratic one, feels more aggressive
        # see `activation` in `src/mode/utils.cpp` for how exactly the calculation is done
        function = quadratic

        # time window (ms) over which the speed is calculated
        # higher values will make slow motions smoother but more delayed
        window = 100
    }

    # configure shake to find
    # magnifies the cursor if its is being shaken
    shake {

        # enables shake to find
        enabled = true

        # use nearest-neighbour (pixelated) scaling when shaking
        # may look weird when effects are enabled
        nearest = true

        # controls how soon a shake is detected
        # lower values mean sooner
        threshold = 6.0

        # magnification level immediately after shake start
        base = 4.0
        # magnification increase per second when continuing to shake
        speed = 4.0
        # how much the speed is influenced by the current shake intensitiy
        influence = 0.0

        # maximal magnification the cursor can reach
        # values below 1 disable the limit (e.g. 0)
        limit = 0.0

        # time in millseconds the cursor will stay magnified after a shake has ended
        timeout = 2000

        # show cursor behaviour `tilt`, `rotate`, etc. while shaking
        effects = false

        # enable ipc events for shake
        # see the `ipc` section below
        ipc = false
    }

    # use hyprcursor to get a higher resolution texture when the cursor is magnified
    # see the `hyprcursor` section below
    hyprcursor {

        # use nearest-neighbour (pixelated) scaling when magnifing beyond texture size
        # this will also have effect without hyprcursor support being enabled
        # 0 / false - never use pixelated scaling
        # 1 / true  - use pixelated when no highres image
        # 2         - always use pixleated scaling
        nearest = true

        # enable dedicated hyprcursor support
        enabled = true

        # resolution in pixels to load the magnified shapes at
        # be warned that loading a very high-resolution image will take a long time and might impact memory consumption
        # -1 means we use [normal cursor size] * [shake:base option]
        resolution = -1

        # shape to use when clientside cursors are being magnified
        # see the shape-name property of shape rules for possible names
        # specifying clientside will use the actual shape, but will be pixelated
        fallback = clientside
    }
}
```

### shape rules
Shape Rules can be used to override the mode or its behaviour on a per-shape basis. They can be defined with the keyword `shaperule` in the config file, preferably in the `plugin:dynamic-cursors` section.

**Note:** Shape rules only apply to server side cursor shapes. Sadly, not everyone supports server side cursors yet, which means shape rules won't work in some applications.

A shape rule usually consists of three parts:
```
shaperule = shape-name, mode (optional), property: value, property: value, ...
```
- `shape-name`: This is the name of the shape, this rule will apply to. Should be one of [those specified in the protocol](https://wayland.app/protocols/cursor-shape-v1#wp_cursor_shape_device_v1:enum:shape). You can use the special shape `clientside` to apply your rule to **ALL** client side cursors. Also note that the compositor will set the shape to `left_ptr` if you are on the wallpaper.
- `mode` (optional): Can override the mode used by this shape, see `mode` in the config. This argument is optional and can be left out.
- `property: value`: At the end of the rule follow zero or more property-value pairs. These are config values that will be overridden if this rule is active. Only config values from the sections `rotate`, `tilt`, `stretch` as seen above can be used.

Here are a few example rules to get you started:
```ini
plugin:dynamic-cursors {
    # apply a 90° offset in rotate mode to the text shape
    shaperule = text, rotate:offset: 90

    # use stretch mode when grabbing, and set the limit low
    shaperule = grab, stretch, stretch:limit: 2000

    # do not show any effects on clientside cursors
    shaperule = clientside, none
}
```

### ipc
This plugin can expose cursor shake events via IPC. This behaviour must be explicitly enabled via the `plugin:dynamic-cursors:shake:ipc` option, as it will spam the socket quite a bit during a shake. These events will appear on [Hyprland's event socket](https://wiki.hyprland.org/IPC/#xdg_runtime_dirhyprhissocket2sock).

The following events with the described arguments are available, when IPC is enabled:
- `shakestart`: fired when a shake is detected.
- `shakeupdate`: fired on frame during the shake, has arguments `x, y, trail, diagonal, zoom`:
  - `x`, `y` are the current cursor position.
  - `trail` and `diagonal` are two floats, the first indicating the distance the mouse travelled, and second the diagonal this movement was within. Their quotient `trail / diagonal` indicates how intense the shaking is.
  - `zoom` is the current cursor magnification level, as currently shown by this plugin, depending on the shake configuration. It is also interpolated smoothly.
- `shakeend`: fired when a shake has ended (after the `timeout`)

If you only want the IPC events and not the plugin actually changing the cursor size, you can set the properties `base` to `1`, `speed`, `influence` and `timeout` to `0` in the `plugin:dynamic-cursors:shake` section such that the cursor is not magified during the shake.

### hyprcursor
This plugin supports using [hyprcursor](https://github.com/hyprwm/hyprcursor) to get higher-resolution images for when the cursor is magnified, i.e. when using shake to find. Due to the nature of cursors on wayland, there are some caveats to it. All configuration for it is located in the `plugin:dynamic-cursors:hyprcursor` section.

To use hyprcursor for magnified shapes, the following must be met:
- `plugin:dynamic-cursors:hyprcursor:enabled` must be true (is by default)
- `cursor:enable_hyprcursor` must be true (is by default)
- you must be using a hyprcursor theme
- the hyprcursor theme should be **SVG-based**

As mentioned, there are some caveats to it. Here are the most common ones:
- **Still pixelated some apps and xwayland** - These apps probably are using clientside cursors, so the program itself is specifying the cursor shape, hence we cannot load a higher resolution for it. You can set a specific shape to show in these cases with the `fallback` option (see config).
- **Blurred at very large sizes** - The high resolution cursors are preloaded at a fixed size. If you magnify your cursor beyond this size, your cursors will look blurry. You can increase the preload size with the `resolution` option (see config), at the expense of some memory and higher loading times.

Loading a cursor theme at a high resolution is relatively resource intensive. This plugin thus loads the theme asynchronously on a seperate thread, meaning your session will stay interactive during this time. But this means that when loading the plugin or changing cursor theme, your CPU might spike momentarily and the high-resolution theme will only be available after a short time (usually just a couple of seconds).

### dispatchers
This plugin has a couple of dispatchers to trigger certain effects with a keybind. Here's a list:
- `plugin:dynamic-cursors:magnify` with arguments `duration?, size?` triggers cursor magnification like on a shake
  - `duration` (optional): overrides duration in milliseconds to stay magnified
  - `size` (optional): overrides magnification factor

## performance
> **TL;DR:** Hardware cursor performance is about the same as if an animated cursor shape was shown whenever you move your mouse. Sofware cursor performance is not impacted. When the cursor is magnified during a shake, the compositor will temporarily switch to software cursors. If your are using an nvidia gpu, this plugin will fall back to software cursors, see [compatibility](#compatibility).

Depending on your Hyprland configuration, this plugin can have a different performance impact, mainly depending on whether you are using software or hardware cursors:

**Software Cursors**: No (additional) performance impact. <br>
Transforming the cursor can be done in the same draw call that is used to draw the cursor anyway, so there is no additional performance impact. Note however that software cursors in of themselves are not really efficient.

**Hardware Cursors**: Medium performance impact. <br>
To transform the cursor smoothly, the cursor shape needs to be changed quite often. This is not exactly compatible with how hardware cursors are intended to work. Thus, performance can be compared to how an animated cursor shape would be rendered, every time the cursor is not stationary. It is however still more performant than software cursors. <br> Another limitation of hardware cursors are the size they are rendered at. This means that when the cursor is being magnified, software cursors will be used temporarily. <br> If your are using an Nvidia GPU, the plugin will also fall back to software cursors because of driver limitations (see [compatibility](#compatibility)).

If you have any ideas to improve performance, let me know!

## compatibility
Compatibility with other plugins is not guaranteed. It probably should work with most plugins, unless they also change your cursor's behaviour. It will however work with any cursor theme.

The plugin does also not support _hardware cursors_ on Nvidia GPUs. If you are on nvidia, Hyprland will use CPU rendering to draw into your hardware cursor buffer, because of driver limitations. When using an effect with this plugin however, we potentially draw into the cursor buffer every frame (when the cursor is moving) which is really resource intensive if done on the CPU. Additionally the whole drawing logic would have to be implemented again to be able run on the CPU too. This is why on Nvidia GPUs, the plugin will automatically force the compositor to use software cursors, avoiding the above issues at a slight performance penalty.

## development
To work on this plugin, you can clone this repository and use the Makefile to build it. I suggest opening a nested Hyprland session, and loading the plugin there:

```sh
make load
```

In some cases when working in a nest, nothing will happen with the plugin loaded. This is because the mouse input is handled differently in a wayland nest. In these cases, set `plugin:dynamic-cursors:ignore_warps` to `false`, to disable warp ignoring, which should fix the issue.

If you want to debug hardware cursors, this plugin also has an additional configuration option, `plugin:dynamic-cursors:hw_debug` which when true will show where the whole cursor buffer is, and also shows when it is updated.

Also make sure you disable the plugin on your host session if your are using hardware cursors, otherwise your cursor will be rotated twice.

## license
This plugin is licensed under the MIT License. Have a look at the `LICENSE.md` file for more information.
