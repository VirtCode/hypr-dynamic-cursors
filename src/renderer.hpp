#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/helpers/math/Math.hpp>

void renderCursorTextureInternalWithDamage(SP<CTexture> tex, CBox* pBox, CRegion* damage, float alpha, Vector2D hotspot);
void projectCursorBox(float mat[9], CBox& box, eTransform transform, float rotation, const float projection[9], Vector2D hotspot);
