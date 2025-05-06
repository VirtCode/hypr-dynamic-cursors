#include "CursorPassElement.hpp"
#include "renderer.hpp"

#include <hyprland/src/render/pass/TexPassElement.hpp>
#include <hyprland/src/render/OpenGL.hpp>

#include <hyprutils/utils/ScopeGuard.hpp>
using namespace Hyprutils::Utils;

CCursorPassElement::CCursorPassElement(const CCursorPassElement::SRenderData& data_) : data(data_) {
    ;
}

void CCursorPassElement::draw(const CRegion& damage) {
    renderCursorTextureInternalWithDamage(
        data.tex,
        &data.box,
        data.damage.empty() ? damage : data.damage,
        1.F,
        data.hotspot,
        data.nearest,
        data.stretchAngle,
        data.stretchMagnitude
    );
}

bool CCursorPassElement::needsLiveBlur() {
    return false; // TODO?
}

bool CCursorPassElement::needsPrecomputeBlur() {
    return false; // TODO?
}

std::optional<CBox> CCursorPassElement::boundingBox() {
    return data.box.copy().scale(1.F / g_pHyprOpenGL->m_renderData.pMonitor->m_scale).round();
}

CRegion CCursorPassElement::opaqueRegion() {
    return {}; // TODO:
}

void CCursorPassElement::discard() {
    ;
}
