#include "ShapeRule.hpp"
#include "config.hpp"
#include "debug/log/Logger.hpp"
#include "rule/IProp.hpp"
#include <hyprutils/string/VarList.hpp>
#include <print>
#include <stdexcept>
#include <string>
#include <hyprutils/string/String.hpp>
#include <hyprutils/string/VarList.hpp>
#include <vector>

using namespace Hyprutils::String;

void CShapeRuleHandler::clear() {
    m_rules.clear();
    active = nullptr;
}

void CShapeRuleHandler::activate(const std::string& shape) {
    if (m_rules.contains(shape))
        active = &m_rules[shape];
    else
        active = nullptr;
}

void CShapeRuleHandler::registerProp(SP<IProp> prop) {
    // FIXME: make this more safe please
    const char* name = prop->name() + NS_LEN;

    m_properties[std::string { name }] = prop;
}

template<typename T>
std::optional<std::string> CShapeRuleHandler::set(const std::string& shape, const std::string& name, T value) {
    if (!m_properties.contains(name))
        return std::format("no such property `{}`", name);

    WP<IProp> prop = m_properties[name];

    if (*prop->underlying() != typeid(T))
        return std::format("invalid type for property `{}`", name);

    // initialize rule with large enough vector
    if (!m_rules.contains(shape))
        m_rules[shape] = std::vector<std::optional<PropValue>>(g_currentShapeRule);

    m_rules[shape][prop->m_id] = value;

    return {};
}

const std::type_info* CShapeRuleHandler::type(const std::string& prop) {
    if (!m_properties.contains(prop))
        return nullptr;

    return m_properties[prop]->underlying();
}

Hyprlang::CParseResult onShapeRuleKeyword(const char* COMMAND, const char* VALUE) {
    Hyprlang::CParseResult res;

    try {
        CVarList list = CVarList(VALUE);
        std::optional<std::string> name;

        for (auto arg : list) {
            // first arg always is shape name
            if (!name.has_value()) {
                name = arg;
                continue;
            }

            auto pos = arg.rfind(':');

            // mode value
            if (pos == std::string::npos) {
                auto error = g_pConfigHandler->m_shapeRules->set<Config::STRING>(name.value(), "mode", arg);

                if (error.has_value())
                    throw std::logic_error(error.value());

            // settings value
            } else {
                auto key = arg.substr(0, pos);
                auto value = arg.substr(pos + 1);

                auto type = g_pConfigHandler->m_shapeRules->type(key);
                if (!type)
                    throw std::logic_error("unkown property " + key);

                std::optional<std::string> error = {};
                if (*type == typeid(Config::STRING))
                    error = g_pConfigHandler->m_shapeRules->set<Config::STRING>(name.value(), key, value);
                else if (*type == typeid(Config::INTEGER))
                    error = g_pConfigHandler->m_shapeRules->set<Config::INTEGER>(name.value(), key, std::stoi(value));
                else if (*type == typeid(Config::FLOAT))
                    error = g_pConfigHandler->m_shapeRules->set<Config::FLOAT>(name.value(), key, std::stof(value));
                else
                    error = "unknown type for property " + key;

                if (error.has_value())
                    throw std::logic_error(error.value());
            }
        }
    } catch (const std::exception& ex) {
        res.setError(ex.what());
    }

    return res;
}
