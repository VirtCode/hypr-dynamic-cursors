#include "render/Shader.hpp"
#include <hyprland/src/helpers/math/Math.hpp>

#include "renderer.hpp"

/*
    Creates a transformation to bake into the monitor transformation to transform the cursor with a normal render call
    (this is how we avoid copying the whole render function here)
*/
Mat3x3 transformMonitorTransform(const Mat3x3& monitor, CBox& box, float rotation, Vector2D hotspot, float stretchAngle, Vector2D stretch) {
    auto mat = Mat3x3::identity();
    mat.translate(box.pos());

    if (stretch != Vector2D{1,1}) {
        // center to origin, rotate, shift up, scale, undo
        // we do the shifting up so the stretch is "only to one side"

        mat.translate({box.w / 2, box.h / 2});
        mat.rotate(stretchAngle);
        mat.translate({0.f, box.h / 2});
        mat.scale(stretch);
        mat.translate({0.f, box.h / -2});
        mat.rotate(-stretchAngle);
        mat.translate({box.w / -2, box.h / -2});
    }

    if (rotation != 0) {
        mat.translate(hotspot);
        mat.rotate(rotation);
        mat.translate(-hotspot);
    }

    mat.translate(-box.pos());

    return monitor.copy().multiply(mat);
}
