#include "../globals.hpp"
#include "../cursor.hpp"
#include "config.hpp"

#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprlang.hpp>
#include <stdexcept>
#include <variant>

bool isEnabled() {
    static auto* const* PENABLED = (Hyprlang::INT* const*) getConfig(CONFIG_ENABLED);
    return **PENABLED && g_pHyprRenderer->m_mostHzMonitor && g_pDynamicCursors; // make sure the compositor is properly initialized
}

Hyprlang::CConfigValue toHyprlang(std::variant<std::string, float, int> value) {

    if (std::holds_alternative<std::string>(value))
        return Hyprlang::STRING { std::get<std::string>(value).c_str() };

    if (std::holds_alternative<float>(value))
        return Hyprlang::FLOAT { std::get<float>(value) };

    if (std::holds_alternative<int>(value))
        return Hyprlang::INT { std::get<int>(value) };

    throw new std::logic_error("invalid type in variant?!");
}

EShapeRuleType toShapeRule(std::variant<std::string, float, int> value) {

    if (std::holds_alternative<std::string>(value))
        return EShapeRuleType::STRING;

    if (std::holds_alternative<float>(value))
        return EShapeRuleType::FLOAT;

    if (std::holds_alternative<int>(value))
        return EShapeRuleType::INT;

    throw new std::logic_error("invalid type in variant?!");
}

void startConfig() {
    g_pShapeRuleHandler = std::make_unique<CShapeRuleHandler>();
}

void addConfig(std::string name, std::variant<std::string, float, int> value) {
    HyprlandAPI::addConfigValue(PHANDLE, NAMESPACE + name, toHyprlang(value));
}

void addShapeConfig(std::string name, std::variant<std::string, float, int> value) {
    addConfig(name, value);

    g_pShapeRuleHandler->addProperty(name, toShapeRule(value));
}

void* const* getConfig(std::string name) {
    return HyprlandAPI::getConfigValue(PHANDLE, NAMESPACE + name)->getDataStaticPtr();
}

void* const* getHyprlandConfig(std::string name) {
    return HyprlandAPI::getConfigValue(PHANDLE, name)->getDataStaticPtr();
}

void addRulesConfig() {
    HyprlandAPI::addConfigKeyword(PHANDLE, CONFIG_SHAPERULE, onShapeRuleKeyword, Hyprlang::SHandlerOptions {});

    // clear on reload
    static const auto PCALLBACK = HyprlandAPI::registerCallbackDynamic( PHANDLE, "preConfigReload", [&](void* self, SCallbackInfo&, std::any data) {
        g_pShapeRuleHandler->clear();
    });
}

void finishConfig() {
    HyprlandAPI::reloadConfig();
}

void addDispatcher(std::string name, std::function<std::optional<std::string>(Hyprutils::String::CVarList)> handler) {
    HyprlandAPI::addDispatcherV2(PHANDLE, NAMESPACE + name, [=](std::string in) {
        auto error = handler(Hyprutils::String::CVarList(in));

        SDispatchResult result;
        if (error.has_value()) {
            Debug::log(ERR, "[dynamic-cursors] dispatcher {} recieved invalid args: {}", name, error.value());
            result.error = error.value();
        }

        return result;
    });
}
