#include "render/Shader.hpp"
#include <GLES2/gl2.h>
#include <GLES3/gl32.h>
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
    g_pHyprOpenGL->m_renderData.renderModif.applyToBox(newBox);

    // get transform
    const auto TRANSFORM = wlTransformToHyprutils(invertTransform(!g_pHyprOpenGL->m_monitorTransformEnabled ? WL_OUTPUT_TRANSFORM_NORMAL : g_pHyprOpenGL->m_renderData.pMonitor->m_transform));
    Mat3x3 matrix = projectCursorBox(newBox, TRANSFORM, newBox.rot, g_pHyprOpenGL->m_renderData.monitorProjection, hotspot, stretchAngle, stretch);

    Mat3x3 glMatrix = g_pHyprOpenGL->m_renderData.projection.copy().multiply(matrix);

    SShader*   shader = nullptr;

    switch (tex->m_type) {
        case TEXTURE_RGBA: shader = &g_pHyprOpenGL->m_shaders->m_shRGBA; break;
        case TEXTURE_RGBX: shader = &g_pHyprOpenGL->m_shaders->m_shRGBX; break;
        case TEXTURE_EXTERNAL: shader = &g_pHyprOpenGL->m_shaders->m_shEXT; break;
        default: RASSERT(false, "tex->m_iTarget unsupported!");
    }

    glActiveTexture(GL_TEXTURE0);
    tex->bind();

    auto scaling = g_pHyprOpenGL->m_renderData.useNearestNeighbor || nearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(tex->m_target, GL_TEXTURE_MAG_FILTER, scaling);
    glTexParameteri(tex->m_target, GL_TEXTURE_MIN_FILTER, scaling);

    g_pHyprOpenGL->useProgram(shader->program);

    shader->setUniformMatrix3fv(SHADER_PROJ, 1, GL_TRUE, glMatrix.getMatrix());
    shader->setUniformInt(SHADER_TEX, 0);
    shader->setUniformFloat(SHADER_ALPHA, alpha);
    shader->setUniformInt(SHADER_DISCARD_OPAQUE, 0);
    shader->setUniformInt(SHADER_DISCARD_ALPHA, 0);

    CBox transformedBox = newBox;
    transformedBox.transform(wlTransformToHyprutils(invertTransform(g_pHyprOpenGL->m_renderData.pMonitor->m_transform)), g_pHyprOpenGL->m_renderData.pMonitor->m_transformedSize.x, g_pHyprOpenGL->m_renderData.pMonitor->m_transformedSize.y);

    const auto TOPLEFT  = Vector2D(transformedBox.x, transformedBox.y);
    const auto FULLSIZE = Vector2D(transformedBox.width, transformedBox.height);

    shader->setUniformFloat2(SHADER_TOP_LEFT, TOPLEFT.x, TOPLEFT.y);
    shader->setUniformFloat2(SHADER_FULL_SIZE, FULLSIZE.x, FULLSIZE.y);
    shader->setUniformFloat(SHADER_RADIUS, 0);

    shader->setUniformInt(SHADER_APPLY_TINT, 0);

    glBindVertexArray(shader->uniformLocations[SHADER_SHADER_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, shader->uniformLocations[SHADER_SHADER_VBO_UV]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fullVerts), fullVerts);

    if (g_pHyprOpenGL->m_renderData.clipBox.width != 0 && g_pHyprOpenGL->m_renderData.clipBox.height != 0) {
        CRegion damageClip{g_pHyprOpenGL->m_renderData.clipBox.x, g_pHyprOpenGL->m_renderData.clipBox.y, g_pHyprOpenGL->m_renderData.clipBox.width, g_pHyprOpenGL->m_renderData.clipBox.height};
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

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    tex->unbind();
}
