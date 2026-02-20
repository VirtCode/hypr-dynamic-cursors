#include "CursorPassElement.hpp"
#include "renderer.hpp"

#include <hyprland/src/render/pass/TexPassElement.hpp>
#include <hyprland/src/render/OpenGL.hpp>

#include <hyprutils/utils/ScopeGuard.hpp>
#include <utility>
using namespace Hyprutils::Utils;

CCursorPassElement::CCursorPassElement(const CCursorPassElement::SRenderData& data_) : data(data_) {
    ;
}

void CCursorPassElement::draw(const CRegion& damage) {
    Mat3x3 proj = transformMonitorTransform(g_pHyprOpenGL->m_renderData.monitorProjection, data.box, data.box.rot, data.hotspot, data.stretchAngle, data.stretchMagnitude);
    data.box.rot = 0;

    std::swap(g_pHyprOpenGL->m_renderData.monitorProjection, proj);
    g_pHyprOpenGL->renderTexture(data.tex, data.box, {
        .damage = data.damage.empty() ? &damage : &data.damage,
    });
    std::swap(g_pHyprOpenGL->m_renderData.monitorProjection, proj);
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
