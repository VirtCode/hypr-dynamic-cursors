#include "StringProp.hpp"
#include "../ShapeRule.hpp"

Config::STRING CStringProp::value() const {
    if (m_rules && m_rules->active) {
        auto& prop = m_rules->active->at(m_id);
        if (prop)
            return std::get<Config::STRING>(prop.value());
    }

    return m_config->value();
}

Config::STRING CStringProp::original() const {
    return m_config->value();
}

const std::type_info* CStringProp::underlying() const {
    return m_config->underlying();
}

const char* CStringProp::name() const {
    return m_config->name();
}
