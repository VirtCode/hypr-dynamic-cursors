#include "StringProp.hpp"

void CStringProp::activate(std::optional<PropValue> value) {
    m_value = value ? std::get<Config::STRING>(*value) : m_config->value();
}

const std::type_info* CStringProp::underlying() const {
    return m_config->underlying();
}

const char* CStringProp::name() const {
    return m_config->name();
}
