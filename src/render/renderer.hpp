#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/helpers/math/Math.hpp>

Mat3x3 transformMonitorTransform(const Mat3x3& monitor, CBox& box, float rotation, Vector2D hotspot, float stretchAngle, Vector2D stretch);
