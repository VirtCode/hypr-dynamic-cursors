#include "renderer.hpp"
#include <hyprland/src/render/Renderer.hpp>

Mat3x3 toTransform(CBox& box, float rotation, Vector2D hotspot, float stretchAngle, Vector2D stretch) {
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

    mat.translate(-box.pos());

    return mat;
}


void drawCursor(const Mat3x3& transform, SP<Render::ITexture> tex, CBox box, CRegion damage) {
    Mat3x3 proj = g_pHyprRenderer->m_renderData.targetProjection.copy().multiply(transform);

    std::swap(g_pHyprRenderer->m_renderData.targetProjection, proj);
    g_pHyprRenderer->draw(CTexPassElement::SRenderData{.tex = tex, .box = box}, damage);
    std::swap(g_pHyprRenderer->m_renderData.targetProjection, proj);
}
