#include "CursorPassElement.hpp"
#include "renderer.hpp"

#include <hyprland/src/render/Renderer.hpp>

using namespace Hyprutils::Utils;

CCursorPassElement::CCursorPassElement(const CCursorPassElement::SRenderData& data) : m_data(data) {
    ;
}

std::vector<UP<IPassElement>> CCursorPassElement::draw(Render::CRenderContext& ctx) {
    CBox   box       = m_data.box;
    Mat3x3 transform = toTransform(box, box.rot, m_data.hotspot, m_data.stretchAngle, m_data.stretchMagnitude, m_data.clickScale);
    box.rot          = 0;

    drawCursor(ctx, transform, m_data.tex, box, ctx.m_data.damage, m_data.nearest);

    return {}; // no passes to be submitted later
}

bool CCursorPassElement::needsLiveBlur(Render::CRenderContext& ctx) {
    return false; // TODO?
}

bool CCursorPassElement::needsPrecomputeBlur(Render::CRenderContext& ctx) {
    return false; // TODO?
}

std::optional<CBox> CCursorPassElement::boundingBox(Render::CRenderContext& ctx) {
    CBox box = m_data.box.copy();
    if (m_data.clickScale != 1.0f) {
        box.x += m_data.hotspot.x * (1.0f - m_data.clickScale);
        box.y += m_data.hotspot.y * (1.0f - m_data.clickScale);
        box.w *= m_data.clickScale;
        box.h *= m_data.clickScale;
    }
    return box.scale(1.F / ctx.m_data.pMonitor->m_scale).round();
}

CRegion CCursorPassElement::opaqueRegion(Render::CRenderContext& ctx) {
    return {}; // TODO:
}

void CCursorPassElement::discard(Render::CRenderContext& ctx) {
    ;
}
