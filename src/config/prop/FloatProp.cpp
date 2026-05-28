#include "FloatProp.hpp"

void CFloatProp::activate(std::optional<PropValue> value) {
    m_value = value ? std::get<Config::FLOAT>(*value) : m_config->value();
}

const std::type_info* CFloatProp::underlying() const {
    return m_config->underlying();
}

const char* CFloatProp::name() const {
    return m_config->name();
}
