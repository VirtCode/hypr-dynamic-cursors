# hypr-dynamic-cursors
This plugin makes your cursor more realistic by simulating how it would behave if it was an actual object being dragged across your screen. This means that your cursor can change based on how it is used, e.g. tilt in the direction you are moving or straight out rotate towards it.

Why did I implement this again?

Inspired by KDE, it also supports shake to find, to enlarge the cursor when it is shaken so it is easier to find it. It can be enabled separately or together with one simulation mode.

### simulation modes
The plugin supports a few different modes. They can all be customized induvidually.

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
The plugin supports shake to find, akin to how KDE Plasma, MacOS, etc. do it. It can also be extensively configured and is enabled by default. If you only want shake to find, and no weird cursor behaviour, you can disable the above modes with the mode `none`.

https://github.com/VirtCode/hypr-dynamic-cursors/assets/41426325/9ff64a9b-64e5-4595-b721-dcb4d62bee18

## state
This plugin is still very early in its development. There are also multiple things which may or may not be implemented in the future:

- [X] software cursor rendering
- [X] hardware cursor rendering (as far as possible with wlroots)
- [X] stick simulation
- [X] air drag simulation
- [ ] pendulum simulation
- [ ] per-shape length and starting angle (if possible)
- [X] cursor shake to find
- [ ] overdue refactoring (wait for aquamarine merge)
- [ ] ~~inverted cursor?~~ (i think out of scope here, but see the [inverted branch](https://github.com/VirtCode/hypr-dynamic-cursors/tree/inverted))
- [ ] hyprcursor magified shape

If anything here sounds interesting to you, don't hesitate to contribute.

Please note that this plugin was created more or less as a joke. I mainly wanted to see how using a rotating or tilted cursor was like. So I will not guarantee any future updates and bugfixes. The only useful feature, shake to find, was implemented more or less as an afterthought.

## installation

### hyprpm

Installation is supported via `hyprpm`. Supported hyprland versions are `v0.41.2` and upwards. The main branch generally tries to target `-git`.

```sh
hyprpm add https://github.com/virtcode/hypr-dynamic-cursors
hyprpm enable dynamic-cursors
```

Compatibility with other plugins is not guaranteed. It probably should work with most plugins, unless they also change your cursor's behaviour. It will however work with any cursor theme.

### NixOS

Add this flake to your inputs, note that using a mismatched/unsupported Hyprland release will fail to build or load the plugin:

```nix
inputs = {
    hyprland.url = "git+https://github.com/hyprwm/Hyprland?submodules=1"; # follows development branch of hyprland
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
    # tilt   - tilt the cursor based on x-velocity
    # rotate - rotate the cursor based on movement direction
    # stretch - stretch the cursor shape based on direction and velocity
    # none   - do not change the cursors behaviour
    mode = tilt

    # minimum angle difference in degrees after which the shape is changed
    # smaller values are smoother, but more expensive for hw cursors
    threshold = 2

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

        # controls how powerful the tilt is, the lower the more power
        # this value controls at which speed (px/s) the full tilt is reached
        limit = 5000

        # relationship between speed and tilt, supports these vaules:
        # linear             - a linear function is used
        # quadratic          - a quadratic function is used (most realistic to actual air drag)
        # negative_quadratic - negative version of the quadratic one, feels more aggressive
        function = negative_quadratic
    }

    # for mode = stretch
    stretch {

        # controls how much the cursor is stretched
        # this value controls at which speed (px/s) the full stretch is reached
        limit = 3000

        # relationship between speed and stretch amount, supports these vaules:
        # linear             - a linear function is used
        # quadratic          - a quadratic function is used
        # negative_quadratic - negative version of the quadratic one, feels more aggressive
        function = quadratic
    }

    # configure shake to find
    shake {

        # enables shake to find
        # magnifies the cursor if its is being shaken
        enabled = true

        # controls how soon a shake is detected
        # lower values mean sooner
        threshold = 4.0

        # controls how fast the cursor gets larger
        factor = 1.5

        # show cursor behaviour `tilt`, `rotate`, etc. while shaking
        effects = false

        # use nearest-neighbour (pixelated) scaling when shaking
        # may look weird when effects are enabled
        nearest = true

        # enable ipc events for shake
        # see #3
        ipc = false
    }
}
```

## performance
> **TL;DR:** Hardware cursor performance is about the same as if an animated cursor shape was shown whenever you move your mouse. Sofware cursor performance is not impacted. When the cursor is magnified during a shake, the compositor will temporarily switch to software cursors.

Depending on your hyprland configuration, this plugin can have a different performance impact, mainly depending on whether you are using software or hardware cursors:

**Software Cursors**: No (additional) performance impact. <br>
Rotating the cursor can be done in the same draw call that is used to draw the cursor anyways, so there is no additional performance impact. Note however that software cursors in of themselves are not really efficient.

**Hardware Cursors**: Medium performance impact. <br>
To rotate the cursor smoothly, the cursor shape needs to be changed quite often. This is not exactly compatible with how hardware cursors are intended to work. Thus performance can be compared to how an animated cursor shape would be rendered, every time the cursor is not stationary. It is however still more performant than software cursors. <br> Another limitation of hardware cursors are the size they are rendered at. This means that when the cursor is being magnified, software cursors will be used temporarily.

If you have any ideas to improve performance, let me know!

## development
To work on this plugin, you can clone this repository and use the Makefile to build it. I suggest opening a nested Hyprland session, and loading the plugin there:

```sh
make load
```

If you want to debug hardware cursors, this plugin also has an additional configuration option, `plugin:dynamic-cursors:hw_debug` which when true will show where the whole cursor buffer is, and also shows when it is updated.

Also make sure you disable the plugin on your host session, otherwise your cursor will be rotated twice.

## license
This plugin is licensed under the MIT License. Have a look at the `LICENSE.md` file for more information.
