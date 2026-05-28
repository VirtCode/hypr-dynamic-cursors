#include "IProp.hpp"

IProp::IProp() {
    m_id = g_currentShapeRule++;
}

std::optional<std::string> IProp::validate(std::optional<PropValue> value) {
    // by default, values are valid
    return {};
}
