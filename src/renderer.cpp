#include "globals.hpp"
#include "src/debug/Log.hpp"
#include <GLES2/gl2.h>

#define private public
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/helpers/math/Math.hpp>
#undef private

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigValue.hpp>

#include "renderer.hpp"
#include "hyprland/math.hpp"

/*
This is the projectBox method from hyprland, but with support for rotation around a point, the hotspot.
*/
void projectCursorBox(float mat[9], CBox& box, eTransform transform, float rotation, const float projection[9], Vector2D hotspot, float stretchAngle, Vector2D stretch) {
    double x      = box.x;
    double y      = box.y;
    double width  = box.width;
    double height = box.height;

    matrixIdentity(mat);
    matrixTranslate(mat, x, y);

    if (stretch != Vector2D{1,1}) {
        // center to origin, rotate, shift up, scale, undo
        // we do the shifting up so the stretch is "only to one side"

        matrixTranslate(mat, box.w / 2, box.h / 2);
        matrixRotate(mat, stretchAngle);
        matrixTranslate(mat, 0, box.h / 2);
        matrixScale(mat, stretch.x, stretch.y);
        matrixTranslate(mat, 0, box.h / -2);
        matrixRotate(mat, -stretchAngle);
        matrixTranslate(mat, box.w / -2, box.h / -2);
    }

    if (rotation != 0) {
        matrixTranslate(mat, hotspot.x, hotspot.y);
        matrixRotate(mat, rotation);
        matrixTranslate(mat, -hotspot.x, -hotspot.y);
    }

    wlr_matrix_scale(mat, width, height);

    if (transform != HYPRUTILS_TRANSFORM_NORMAL) {
        matrixTranslate(mat, 0.5, 0.5);
        matrixTransform(mat, transform);
        matrixTranslate(mat, -0.5, -0.5);
    }

    matrixMultiply(mat, projection, mat);
}

/*
This renders a texture with damage but rotates the texture around a given hotspot.
*/
void renderCursorTextureInternalWithDamage(SP<CTexture> tex, CBox* pBox, CRegion* damage, float alpha, Vector2D hotspot, bool nearest, float stretchAngle, Vector2D stretch) {
    TRACY_GPU_ZONE("RenderDynamicCursor");

    alpha = std::clamp(alpha, 0.f, 1.f);

    if (damage->empty())
        return;

    CBox newBox = *pBox;
    g_pHyprOpenGL->m_RenderData.renderModif.applyToBox(newBox);

    // get transform
    const auto TRANSFORM = wlTransformToHyprutils(wlr_output_transform_invert(!g_pHyprOpenGL->m_bEndFrame ? WL_OUTPUT_TRANSFORM_NORMAL : g_pHyprOpenGL->m_RenderData.pMonitor->transform));
    float      matrix[9];
    projectCursorBox(matrix, newBox, TRANSFORM, newBox.rot, g_pHyprOpenGL->m_RenderData.monitorProjection.data(), hotspot, stretchAngle, stretch);

    float glMatrix[9];
    wlr_matrix_multiply(glMatrix, g_pHyprOpenGL->m_RenderData.projection, matrix);

    CShader*   shader = nullptr;

    switch (tex->m_iType) {
        case TEXTURE_RGBA: shader = &g_pHyprOpenGL->m_RenderData.pCurrentMonData->m_shRGBA; break;
        case TEXTURE_RGBX: shader = &g_pHyprOpenGL->m_RenderData.pCurrentMonData->m_shRGBX; break;
        case TEXTURE_EXTERNAL: shader = &g_pHyprOpenGL->m_RenderData.pCurrentMonData->m_shEXT; break;
        default: RASSERT(false, "tex->m_iTarget unsupported!");
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(tex->m_iTarget, tex->m_iTexID);

    if (g_pHyprOpenGL->m_RenderData.useNearestNeighbor || nearest) {
        glTexParameteri(tex->m_iTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(tex->m_iTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(tex->m_iTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(tex->m_iTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glUseProgram(shader->program);

#ifndef GLES2
    glUniformMatrix3fv(shader->proj, 1, GL_TRUE, glMatrix);
#else
    wlr_matrix_transpose(glMatrix, glMatrix);
    glUniformMatrix3fv(shader->proj, 1, GL_FALSE, glMatrix);
#endif
    glUniform1i(shader->tex, 0);
    glUniform1f(shader->alpha, alpha);
    glUniform1i(shader->discardOpaque, 0);
    glUniform1i(shader->discardAlpha, 0);

    CBox transformedBox = newBox;
    transformedBox.transform(wlTransformToHyprutils(wlr_output_transform_invert(g_pHyprOpenGL->m_RenderData.pMonitor->transform)), g_pHyprOpenGL->m_RenderData.pMonitor->vecTransformedSize.x, g_pHyprOpenGL->m_RenderData.pMonitor->vecTransformedSize.y);

    const auto TOPLEFT  = Vector2D(transformedBox.x, transformedBox.y);
    const auto FULLSIZE = Vector2D(transformedBox.width, transformedBox.height);

    glUniform2f(shader->topLeft, TOPLEFT.x, TOPLEFT.y);
    glUniform2f(shader->fullSize, FULLSIZE.x, FULLSIZE.y);
    glUniform1f(shader->radius, 0);

    glUniform1i(shader->applyTint, 0);

    glVertexAttribPointer(shader->posAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);
    glVertexAttribPointer(shader->texAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);

    glEnableVertexAttribArray(shader->posAttrib);
    glEnableVertexAttribArray(shader->texAttrib);

    if (g_pHyprOpenGL->m_RenderData.clipBox.width != 0 && g_pHyprOpenGL->m_RenderData.clipBox.height != 0) {
        CRegion damageClip{g_pHyprOpenGL->m_RenderData.clipBox.x, g_pHyprOpenGL->m_RenderData.clipBox.y, g_pHyprOpenGL->m_RenderData.clipBox.width, g_pHyprOpenGL->m_RenderData.clipBox.height};
        damageClip.intersect(*damage);

        if (!damageClip.empty()) {
            for (auto& RECT : damageClip.getRects()) {
                g_pHyprOpenGL->scissor(&RECT);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
    } else {
        for (auto& RECT : damage->getRects()) {
            g_pHyprOpenGL->scissor(&RECT);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }

    glDisableVertexAttribArray(shader->posAttrib);
    glDisableVertexAttribArray(shader->texAttrib);

    glBindTexture(tex->m_iTarget, 0);
}
