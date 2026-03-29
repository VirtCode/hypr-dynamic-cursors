#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/helpers/math/Math.hpp>

/*
 *  creates a transformation intended to be baked into the monitor transformation to transform the cursor with a normal render call
 *  (this is how we avoid copying the whole render function into the plugin)
 *
 *  TODO: refactor box out of here and do the compensation back in drawCursor()
 */
Mat3x3 toTransform(CBox& box, float rotation, Vector2D hotspot, float stretchAngle, Vector2D stretch);

/*
 * dispatches a draw call using the current hyprland renderer to draw the cursor with the provided transform
 * will modify the monitor transform and do a tex pass immediately
 */
void drawCursor(const Mat3x3& transform, SP<Render::ITexture> tex, CBox box, CRegion damage, bool nearest);
