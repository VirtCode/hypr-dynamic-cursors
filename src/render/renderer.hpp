#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/helpers/math/Math.hpp>

void renderCursorTextureInternalWithDamage(SP<ITexture> tex, CBox* pBox, const CRegion& damage, float alpha, Vector2D hotspot, bool nearest, float stretchAngle, Vector2D stretch);
