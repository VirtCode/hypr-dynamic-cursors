#include "ShapeRule.hpp"
#include <hyprutils/string/VarList.hpp>
#include <stdexcept>
#include <string>
#include <hyprutils/string/String.hpp>
#include <hyprutils/string/VarList.hpp>

using namespace Hyprutils::String;

void CShapeRuleHandler::clear() {
    rules.clear();
    active = nullptr;
}

void CShapeRuleHandler::activate(std::string key) {
    if (rules.contains(key))
        active = &rules[key];
    else
        active = nullptr;
}

void CShapeRuleHandler::addProperty(std::string key, EShapeRuleType type) {
    content[key] = type;
}

std::variant<std::string, float, int> parse(std::string value, EShapeRuleType type) {
    switch (type) {
        case EShapeRuleType::STRING:
            return value;
        case EShapeRuleType::FLOAT:
            return std::stof(value);
        case EShapeRuleType::INT:
            return std::stoi(value);
    }

    throw std::logic_error("unknown type");
}

void CShapeRuleHandler::parseRule(std::string string) {
    std::optional<std::string> name;
    SShapeRule rule;

    CVarList list = CVarList(string);

    for (auto arg : list) {
        if (!name.has_value()) name = arg;
        else {
            auto pos = arg.rfind(':');

            // mode value
            if (pos == std::string::npos) {
                if (rule.mode.has_value())
                    throw std::logic_error("cannot specify mode twice");

                rule.mode = arg;

            // settings value
            } else {
                auto key = arg.substr(0, pos);
                auto value = arg.substr(pos + 1);

                if (rule.content.contains(key))
                    throw std::logic_error("cannot specify property " + key + " twice");

                if (!content.contains(key))
                    throw std::logic_error("unkown property " + key);

                auto type = content[key];

                try {
                    rule.content[key] = parse(value, type);
                } catch (...) {
                    throw std::logic_error("invalid type for property " + key);
                }
            }
        }
    }

    if (!name.has_value())
        throw std::logic_error("need to specify at least shape name");

    if (rules.contains(name.value()))
        throw std::logic_error("cannot have two rules for shape " + name.value());

    rules[name.value()] = rule;
}

Hyprlang::CParseResult onShapeRuleKeyword(const char* COMMAND, const char* VALUE) {
    Hyprlang::CParseResult res;

    try {
        g_pShapeRuleHandler->parseRule(std::string{VALUE});
    } catch (const std::exception& ex) {
        res.setError(ex.what());
    }

    return res;
}

std::string CShapeRuleHandler::getModeOr(std::string def) {
    if (active) return active->mode.value_or(def);
    else return def;
}

std::string CShapeRuleHandler::getStringOr(std::string key, std::string def) {
    if (active && active->content.contains(key)) return std::get<std::string>(active->content[key]);
    else return def;
}

int CShapeRuleHandler::getIntOr(std::string key, int def) {
    if (active && active->content.contains(key)) return std::get<int>(active->content[key]);
    else return def;
}

float CShapeRuleHandler::getFloatOr(std::string key, float def) {
    if (active && active->content.contains(key)) return std::get<float>(active->content[key]);
    else return def;
}
