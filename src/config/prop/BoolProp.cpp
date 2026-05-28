#include "BoolProp.hpp"

void CBoolProp::activate(std::optional<PropValue> value) {
    m_value = value ? std::get<Config::BOOL>(*value) : m_config->value();
}

const std::type_info* CBoolProp::underlying() const {
    return m_config->underlying();
}

const char* CBoolProp::name() const {
    return m_config->name();
}
