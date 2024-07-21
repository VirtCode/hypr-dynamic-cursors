#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/helpers/math/Math.hpp>

void renderCursorTextureInternalWithDamage(SP<CTexture> tex, CBox* pBox, CRegion* damage, float alpha, SP<CSyncTimeline> waitTimeline, uint64_t waitPoint, Vector2D hotspot, bool nearest, float stretchAngle, Vector2D stretch);
