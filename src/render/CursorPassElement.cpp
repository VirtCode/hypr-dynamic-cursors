#include "CursorPassElement.hpp"
#include "renderer.hpp"

#include <hyprland/src/render/Renderer.hpp>

using namespace Hyprutils::Utils;

CCursorPassElement::CCursorPassElement(const CCursorPassElement::SRenderData& data) : m_data(data) {
    ;
}

std::vector<UP<IPassElement>> CCursorPassElement::draw() {
    Mat3x3 transform = toTransform(m_data.box, m_data.box.rot, m_data.hotspot, m_data.stretchAngle, m_data.stretchMagnitude);
    m_data.box.rot = 0;

    drawCursor(transform, m_data.tex, m_data.box, {0, 0, INT16_MAX, INT16_MAX}, m_data.nearest); // TODO: pass the current damage here somehow

    return {}; // no passes to be submitted later
}

bool CCursorPassElement::needsLiveBlur() {
    return false; // TODO?
}

bool CCursorPassElement::needsPrecomputeBlur() {
    return false; // TODO?
}

std::optional<CBox> CCursorPassElement::boundingBox() {
    return m_data.box.copy().scale(1.F / g_pHyprRenderer->m_renderData.pMonitor->m_scale).round();
}

CRegion CCursorPassElement::opaqueRegion() {
    return {}; // TODO:
}

void CCursorPassElement::discard() {
    ;
}
