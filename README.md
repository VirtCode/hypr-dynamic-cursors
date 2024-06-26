# hypr-dynamic-cursors
This plugin makes your cursor more realistic by simulating how it would behave if it was an actual object being dragged across your screen. This means that it will change orientation based on in which direction you are moving.

Why did I implement this again?

## showcase
Currently, the cursor is being simulated as a stick being dragged across a surface. In action, this looks like this.

https://github.com/VirtCode/hypr-dynamic-cursor/assets/41426325/ccd6d742-8e2b-4073-a35e-318c7e19705c

## state
This plugin is still very early in its development. **Currently, only the `-git` version of hyprland is supported**. There are also multiple things which may or may not be implemented in the future:

- [X] Software Cursor rendering
- [X] Hardware Cursor rendering (as good as possible)
- [X] Stick simulation
- [ ] Per-shape length and starting angle (at least with serverside cursors)
- [ ] Pendulum simulation
- [X] Air Drag simulation

If anything here sounds interesting to you, don't hesitate to contribute.

Please note that this plugin was created more or less as a joke. I mainly wanted to see how using a rotating cursor was like. So I will not guarantee any future updates and bugfixes.

## installation
Installation is supported via `hyprpm`. Supported hyprland versions are `v0.42.0` (yet unreleased) and upwards. The main branch generally tries to target `-git`.

```sh
hyprpm add https://github.com/virtcode/hypr-dynamic-cursors
hyprpm enable dynamic-cursors
```

Compatibility with other plugins is not guaranteed. It probably should work with most plugins, unless they also change your cursor's behaviour. It will however work with any cursor theme.

## configuration
This plugin can be configured in its dedicated configuration section (`plugin:dynamic-cursors`). The default values are shown below.
```ini
plugin:dynamic-cursors {

    # enables the plugin
    enabled = true

    # length in px of the simulated stick used to rotate the cursor
    # it is recommended to set this to your actual cursor size
    # longer sticks feel more "drifty"
    length = 20

}
```

## performance
Depending on your hyprland configuration, this plugin can have a different performance impact. It mainly depends on whether you are using software or hardware cursors:

**Software Cursors**: No (additional) performance impact. <br>
Rotating the cursor can be done in the same draw call that is used to draw the cursor anyways, so there is no additional performance impact. Note however that software cursors in of themselves are not really efficient.

**Hardware Cursors**: Medium performance impact. <br>
To rotate the cursor smoothly, the cursor shape needs to be changed quite often. This is not exactly compatible with how hardware cursors are intended to work. With this plugin, the overhead of changing the cursor shape can be noticed more often, making it use more resources. Compared to normal hardware cursors, there is a big performance hit, but it is still more efficient than using software cursor though.

## development
To work on this plugin, you can clone this repository and use the Makefile to build it. I suggest opening a nested Hyprland session, and loading the plugin there:

```sh
make load
```

If you want to debug hardware cursors, this plugin also has an additional configuration option, `hw_debug` which when true will show where the whole cursor buffer is, and also shows when it is updated.

## license
This plugin is licensed under the MIT License. Have a look at the `LICENSE.md` file for more information.
