#include "VariantProp.hpp"

#include <format>
#include <hyprland/src/debug/log/Logger.hpp>

std::optional<std::string> CVariantProp::validate(std::optional<PropValue> value) {
    if (!value)
        return {};

    auto str = std::get<Config::STRING>(*value);
    if (!m_config->lookup(str))
        return std::format("invalid variant `{}`, must be one of {}", str, m_config->formatVariants());

    return {};
}

void CVariantProp::activate(std::optional<PropValue> value) {
    if (value) {
        auto str    = std::get<Config::STRING>(*value);
        auto result = m_config->lookup(str);

        // if this is not there, we'd already have emitted a config error
        if (result)
            m_value = *result;
    } else
        m_value = m_config->value();
}

const std::type_info* CVariantProp::underlying() const {
    return m_config->internal()->underlying();
}

const char* CVariantProp::name() const {
    return m_config->internal()->name();
}
