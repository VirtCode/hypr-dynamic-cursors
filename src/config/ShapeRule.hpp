#pragma once

#include "prop/IProp.hpp"

#include <hyprland/src/config/shared/Types.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include <hyprlang.hpp>

/* the actual types a value can have */
typedef std::variant<Config::STRING, Config::FLOAT, Config::INTEGER, Config::BOOL> PropValue;

/* how we store all the properties for a rule */
typedef std::vector<std::optional<PropValue>> PropStore;

class CShapeRuleHandler {
    /* available properties */
    std::unordered_map<std::string, SP<IProp>> m_properties;

    /* possible rules */
    std::unordered_map<std::string, PropStore> m_rules;

  public:
    /* currently active rule, nullptr if none */
    PropStore* active = nullptr;

    /* adds a new shape rule property */
    void registerProp(SP<IProp>);

    /* removes currently added shape rules */
    void clear();

    /* sets a property for the given shape, returns errors as a string if any */
    template <typename T>
    std::optional<std::string> set(const std::string& shape, const std::string& name, T value);

    /* activates the shape rule for the given shape */
    void activate(const std::string& shape);

    /* returns the type info of a given prop */
    // TODO: remove this once we no longer need the legacy config parser
    const std::type_info* type(const std::string& prop);
};

/* method called by hyprland api */
Hyprlang::CParseResult onShapeRuleKeyword(const char* COMMAND, const char* VALUE);

/* method to parse lua shape rule */
int luaShapeRule(lua_State* L);
