#include <hyprland/src/Compositor.hpp>

/*
The following functions are copied 1:1 from the hyprland codebase.
This is nessecary because we cannot use functions which are not declared in any header.
*/

void matrixIdentity(float mat[9]) {
    static const float identity[9] = {
        1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    };
    memcpy(mat, identity, sizeof(identity));
}

void matrixMultiply(float mat[9], const float a[9], const float b[9]) {
    float product[9];

    product[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
    product[1] = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
    product[2] = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];

    product[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
    product[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
    product[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];

    product[6] = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];
    product[7] = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];
    product[8] = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];

    memcpy(mat, product, sizeof(product));
}

void matrixTranslate(float mat[9], float x, float y) {
    float translate[9] = {
        1.0f, 0.0f, x, 0.0f, 1.0f, y, 0.0f, 0.0f, 1.0f,
    };
    wlr_matrix_multiply(mat, mat, translate);
}

void matrixRotate(float mat[9], float rad) {
    float rotate[9] = {
        cos(rad), -sin(rad), 0.0f, sin(rad), cos(rad), 0.0f, 0.0f, 0.0f, 1.0f,
    };
    wlr_matrix_multiply(mat, mat, rotate);
}

static std::unordered_map<eTransform, std::array<float, 9>> transforms = {
    {HYPRUTILS_TRANSFORM_NORMAL,
     {
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_90,
     {
         0.0f,
         1.0f,
         0.0f,
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_180,
     {
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_270,
     {
         0.0f,
         -1.0f,
         0.0f,
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_FLIPPED,
     {
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_FLIPPED_90,
     {
         0.0f,
         1.0f,
         0.0f,
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_FLIPPED_180,
     {
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_FLIPPED_270,
     {
         0.0f,
         -1.0f,
         0.0f,
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
};

void matrixTransform(float mat[9], eTransform transform) {
    matrixMultiply(mat, mat, transforms.at(transform).data());
}
