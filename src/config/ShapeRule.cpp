#include "ShapeRule.hpp"
#include "config.hpp"
#include "rule/IProp.hpp"
#include <hyprutils/string/VarList.hpp>
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

void CShapeRuleHandler::activate(const std::string& key) {
    if (m_rules.contains(key))
        active = &m_rules[key];
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
