#include <GLES2/gl2.h>
#include <hyprland/src/defines.hpp> // don't unprivate stuff in here

#define private public
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/helpers/math/Math.hpp>
#undef private

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/debug/Log.hpp>

#include "renderer.hpp"

/*
This is the projectBox method from hyprland, but with support for rotation around a point, the hotspot.
*/
Mat3x3 projectCursorBox(CBox& box, eTransform transform, float rotation, const Mat3x3& projection, Vector2D hotspot, float stretchAngle, Vector2D stretch) {
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

    mat.scale(box.size());

    if (transform != HYPRUTILS_TRANSFORM_NORMAL) {
        mat.translate({0.5, 0.5});
        mat.transform(transform);
        mat.translate({-0.5, -0.5});
    }

    return projection.copy().multiply(mat);
}

/*
This renders a texture with damage but rotates the texture around a given hotspot.
*/
void renderCursorTextureInternalWithDamage(SP<CTexture> tex, CBox* pBox, const CRegion& damage, float alpha, Vector2D hotspot, bool nearest, float stretchAngle, Vector2D stretch) {
    TRACY_GPU_ZONE("RenderDynamicCursor");

    alpha = std::clamp(alpha, 0.f, 1.f);

    if (damage.empty())
        return;

    CBox newBox = *pBox;
    g_pHyprOpenGL->m_RenderData.renderModif.applyToBox(newBox);

    // get transform
    const auto TRANSFORM = wlTransformToHyprutils(invertTransform(!g_pHyprOpenGL->m_bEndFrame ? WL_OUTPUT_TRANSFORM_NORMAL : g_pHyprOpenGL->m_RenderData.pMonitor->transform));
    Mat3x3 matrix = projectCursorBox(newBox, TRANSFORM, newBox.rot, g_pHyprOpenGL->m_RenderData.monitorProjection, hotspot, stretchAngle, stretch);

    Mat3x3 glMatrix = g_pHyprOpenGL->m_RenderData.projection.copy().multiply(matrix);

    CShader*   shader = nullptr;

    switch (tex->m_iType) {
        case TEXTURE_RGBA: shader = &g_pHyprOpenGL->m_shaders->m_shRGBA; break;
        case TEXTURE_RGBX: shader = &g_pHyprOpenGL->m_shaders->m_shRGBX; break;
        case TEXTURE_EXTERNAL: shader = &g_pHyprOpenGL->m_shaders->m_shEXT; break;
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
    glUniformMatrix3fv(shader->proj, 1, GL_TRUE, glMatrix.getMatrix().data());
#else
    glUniformMatrix3fv(shader->proj, 1, GL_FALSE, glMatrix.transpose().getMatrix().data());
#endif

    glUniform1i(shader->tex, 0);
    glUniform1f(shader->alpha, alpha);
    glUniform1i(shader->discardOpaque, 0);
    glUniform1i(shader->discardAlpha, 0);

    CBox transformedBox = newBox;
    transformedBox.transform(wlTransformToHyprutils(invertTransform(g_pHyprOpenGL->m_RenderData.pMonitor->transform)), g_pHyprOpenGL->m_RenderData.pMonitor->vecTransformedSize.x, g_pHyprOpenGL->m_RenderData.pMonitor->vecTransformedSize.y);

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
        damageClip.intersect(damage);

        if (!damageClip.empty()) {
            for (auto& RECT : damageClip.getRects()) {
                g_pHyprOpenGL->scissor(&RECT);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
    } else {
        for (auto& RECT : damage.getRects()) {
            g_pHyprOpenGL->scissor(&RECT);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }

    glDisableVertexAttribArray(shader->posAttrib);
    glDisableVertexAttribArray(shader->texAttrib);

    glBindTexture(tex->m_iTarget, 0);
}
