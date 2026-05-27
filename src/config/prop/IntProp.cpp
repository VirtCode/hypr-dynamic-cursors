#include "IntProp.hpp"
#include "../ShapeRule.hpp"

Config::INTEGER CIntProp::value() const {
    if (m_rules && m_rules->active) {
        auto& prop = m_rules->active->at(m_id);
        if (prop) return std::get<Config::INTEGER>(prop.value());
    }

    return m_config->value();
}

Config::INTEGER CIntProp::original() const {
    return m_config->value();
}

const std::type_info* CIntProp::underlying() const {
    return m_config->underlying();
}

const char* CIntProp::name() const {
    return m_config->name();
}
