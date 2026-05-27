#include "BoolProp.hpp"
#include "../ShapeRule.hpp"

Config::BOOL CBoolProp::value() const {
    if (m_rules && m_rules->active) {
        auto& prop = m_rules->active->at(m_id);
        if (prop)
            return std::get<Config::BOOL>(prop.value());
    }

    return m_config->value();
}

Config::BOOL CBoolProp::original() const {
    return m_config->value();
}

const std::type_info* CBoolProp::underlying() const {
    return m_config->underlying();
}

const char* CBoolProp::name() const {
    return m_config->name();
}
