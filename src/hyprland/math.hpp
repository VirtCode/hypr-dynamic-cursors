#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/math/Math.hpp>

/*
The following functions are copied 1:1 from the hyprland codebase.
This is nessecary because we cannot use functions which are not declared in any header.
*/

void matrixTransform(float mat[9], eTransform transform);
void matrixTranslate(float mat[9], float x, float y);
void matrixMultiply(float mat[9], const float a[9], const float b[9]);
void matrixIdentity(float mat[9]);
void matrixRotate(float mat[9], float rad);
