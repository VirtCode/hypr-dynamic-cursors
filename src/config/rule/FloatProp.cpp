#include "FloatProp.hpp"
#include "../ShapeRule.hpp"

Config::FLOAT CFloatProp::value() const {
    if (m_rules && m_rules->active) {
        auto& prop = m_rules->active->at(m_id);
        if (prop) return std::get<Config::BOOL>(prop.value());
    }

    return m_config->value();
}

Config::FLOAT CFloatProp::original() const {
    return m_config->value();
}

const std::type_info* CFloatProp::underlying() const {
    return m_config->underlying();
}

const char* CFloatProp::name() const {
    return m_config->name();
}
