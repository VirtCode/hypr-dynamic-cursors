#include "IntProp.hpp"

void CIntProp::activate(std::optional<PropValue> value) {
    m_value = value ? std::get<Config::INTEGER>(*value) : m_config->value();
}

const std::type_info* CIntProp::underlying() const {
    return m_config->underlying();
}

const char* CIntProp::name() const {
    return m_config->name();
}
