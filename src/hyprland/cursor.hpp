#include <hyprland/src/managers/PointerManager.hpp>

/*
The following functions are copied 1:1 from the hyprland codebase.
This is nessecary, as these are static interop functions which wlroots which are not accessible through headers.
Will probably be obsolete anyway after https://github.com/hyprwm/Hyprland/pull/6608
*/

bool wlr_drm_format_intersect(wlr_drm_format* dst, const wlr_drm_format* a, const wlr_drm_format* b);
bool wlr_drm_format_copy(wlr_drm_format* dst, const wlr_drm_format* src);
const wlr_drm_format_set* wlr_renderer_get_render_formats(wlr_renderer* r);
bool output_pick_format(wlr_output* output, const wlr_drm_format_set* display_formats, wlr_drm_format* format, uint32_t fmt);
bool output_pick_cursor_format(struct wlr_output* output, struct wlr_drm_format* format);
