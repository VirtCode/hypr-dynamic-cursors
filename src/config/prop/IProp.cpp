#include "IProp.hpp"

IProp::IProp() {
    m_id = g_currentShapeRule++;
}

void IProp::setHandler(WP<CShapeRuleHandler> handler) {
    m_rules = handler;
}
