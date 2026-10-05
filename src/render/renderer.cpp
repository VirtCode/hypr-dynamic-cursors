#include "renderer.hpp"

#include <hyprland/src/render/Renderer.hpp>

Mat3x3 toTransform(CBox& box, float rotation, Vector2D hotspot, float stretchAngle, Vector2D stretch) {
    auto mat = Mat3x3::identity();
    mat.translate(box.pos());

    if (stretch != Vector2D{1, 1}) {
        // center to origin, rotate, shift up, scale, undo
        // we do the shifting up, so the stretch is "only to one side"

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

void drawCursor(Render::CRenderContext& ctx, const Mat3x3& transform, SP<Render::ITexture> tex, CBox box, CRegion damage, bool nearest) {
    Mat3x3 proj = ctx.m_data.targetProjection.copy().multiply(transform);

    std::swap(ctx.m_data.targetProjection, proj);
    ctx.m_data.useNearestNeighbor = nearest;

    g_pHyprRenderer->draw(ctx, CTexPassElement::SRenderData{.tex = tex, .box = box}, damage);

    std::swap(ctx.m_data.targetProjection, proj);
    ctx.m_data.useNearestNeighbor = false;
}
