
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprland/src/Compositor.hpp>

#include <hyprland/wlr/interfaces/wlr_output.h>
#include <hyprland/wlr/render/interface.h>
#include <hyprland/wlr/render/wlr_renderer.h>

/*
The following functions are copied 1:1 from the hyprland codebase.
This is nessecary, as these are static interop functions which wlroots which are not accessible through headers.
Will probably be obsolete anyway after https://github.com/hyprwm/Hyprland/pull/6608
*/

// TODO: make nicer
// this will come with the eventual rewrite of wlr_drm, etc...
bool wlr_drm_format_intersect(wlr_drm_format* dst, const wlr_drm_format* a, const wlr_drm_format* b) {
    ASSERT(a->format == b->format);

    size_t    capacity  = a->len < b->len ? a->len : b->len;
    uint64_t* modifiers = (uint64_t*)malloc(sizeof(*modifiers) * capacity);
    if (!modifiers)
        return false;

    struct wlr_drm_format fmt = {
        .format    = a->format,
        .len       = 0,
        .capacity  = capacity,
        .modifiers = modifiers,
    };

    for (size_t i = 0; i < a->len; i++) {
        for (size_t j = 0; j < b->len; j++) {
            if (a->modifiers[i] == b->modifiers[j]) {
                ASSERT(fmt.len < fmt.capacity);
                fmt.modifiers[fmt.len++] = a->modifiers[i];
                break;
            }
        }
    }

    wlr_drm_format_finish(dst);
    *dst = fmt;
    return true;
}

bool wlr_drm_format_copy(wlr_drm_format* dst, const wlr_drm_format* src) {
    ASSERT(src->len <= src->capacity);

    uint64_t* modifiers = (uint64_t*)malloc(sizeof(*modifiers) * src->len);
    if (!modifiers)
        return false;

    memcpy(modifiers, src->modifiers, sizeof(*modifiers) * src->len);

    wlr_drm_format_finish(dst);
    dst->capacity  = src->len;
    dst->len       = src->len;
    dst->format    = src->format;
    dst->modifiers = modifiers;
    return true;
}

const wlr_drm_format_set* wlr_renderer_get_render_formats(wlr_renderer* r) {
    if (!r->impl->get_render_formats)
        return nullptr;

    return r->impl->get_render_formats(r);
}

bool output_pick_format(wlr_output* output, const wlr_drm_format_set* display_formats, wlr_drm_format* format, uint32_t fmt) {

    const wlr_drm_format_set* render_formats = wlr_renderer_get_render_formats(g_pCompositor->m_sWLRRenderer);
    if (render_formats == NULL) {
        wlr_log(WLR_ERROR, "Failed to get render formats");
        return false;
    }

    const wlr_drm_format* render_format = wlr_drm_format_set_get(render_formats, fmt);
    if (render_format == NULL) {
        wlr_log(WLR_DEBUG, "Renderer doesn't support format 0x%" PRIX32, fmt);
        return false;
    }

    if (display_formats != NULL) {
        const wlr_drm_format* display_format = wlr_drm_format_set_get(display_formats, fmt);
        if (display_format == NULL) {
            wlr_log(WLR_DEBUG, "Output doesn't support format 0x%" PRIX32, fmt);
            return false;
        }
        if (!wlr_drm_format_intersect(format, display_format, render_format)) {
            wlr_log(WLR_DEBUG,
                    "Failed to intersect display and render "
                    "modifiers for format 0x%" PRIX32 " on output %s",
                    fmt, output->name);
            return false;
        }
    } else {
        // The output can display any format
        if (!wlr_drm_format_copy(format, render_format))
            return false;
    }

    if (format->len == 0) {
        wlr_drm_format_finish(format);
        wlr_log(WLR_DEBUG, "Failed to pick output format");
        return false;
    }

    return true;
}

bool output_pick_cursor_format(struct wlr_output* output, struct wlr_drm_format* format) {
    struct wlr_allocator* allocator = output->allocator;
    ASSERT(allocator != NULL);

    const struct wlr_drm_format_set* display_formats = NULL;
    if (output->impl->get_cursor_formats) {
        display_formats = output->impl->get_cursor_formats(output, allocator->buffer_caps);
        if (display_formats == NULL) {
            wlr_log(WLR_DEBUG, "Failed to get cursor display formats");
            return false;
        }
    }

    // Note: taken from https://gitlab.freedesktop.org/wlroots/wlroots/-/merge_requests/4596/diffs#diff-content-e3ea164da86650995728d70bd118f6aa8c386797
    // If this fails to find a shared modifier try to use a linear
    // modifier. This avoids a scenario where the hardware cannot render to
    // linear textures but only linear textures are supported for cursors,
    // as is the case with Nvidia and VmWare GPUs
    if (!output_pick_format(output, display_formats, format, DRM_FORMAT_ARGB8888)) {
        // Clear the format as output_pick_format doesn't zero it
        memset(format, 0, sizeof(*format));
        return output_pick_format(output, NULL, format, DRM_FORMAT_ARGB8888);
    }
    return true;
}
