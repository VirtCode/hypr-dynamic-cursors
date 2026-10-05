#include "CursorPassElement.hpp"
#include "renderer.hpp"

#include <hyprland/src/render/Renderer.hpp>

using namespace Hyprutils::Utils;

CCursorPassElement::CCursorPassElement(const CCursorPassElement::SRenderData& data) : m_data(data) {
    ;
}

std::vector<UP<IPassElement>> CCursorPassElement::draw(Render::CRenderContext& ctx) {
    Mat3x3 transform = toTransform(m_data.box, m_data.box.rot, m_data.hotspot, m_data.stretchAngle, m_data.stretchMagnitude);
    m_data.box.rot   = 0;

    drawCursor(ctx, transform, m_data.tex, m_data.box, ctx.m_data.damage, m_data.nearest);

    return {}; // no passes to be submitted later
}

bool CCursorPassElement::needsLiveBlur(Render::CRenderContext& ctx) {
    return false; // TODO?
}

bool CCursorPassElement::needsPrecomputeBlur(Render::CRenderContext& ctx) {
    return false; // TODO?
}

std::optional<CBox> CCursorPassElement::boundingBox(Render::CRenderContext& ctx) {
    return m_data.box.copy().scale(1.F / ctx.m_data.pMonitor->m_scale).round();
}

CRegion CCursorPassElement::opaqueRegion(Render::CRenderContext& ctx) {
    return {}; // TODO:
}

void CCursorPassElement::discard(Render::CRenderContext& ctx) {
    ;
}
