#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include <hyprlang.hpp>

/* stores possible types in a shape rule */
enum EShapeRuleType {
    STRING,
    FLOAT,
    INT
};

struct SShapeRule {
    std::optional<std::string> mode;
    std::unordered_map<std::string, std::variant<std::string, float, int>> content;
};

class CShapeRuleHandler {
    /* induvidual rule content */
    std::unordered_map<std::string, EShapeRuleType> content;

    /* possible rules */
    std::unordered_map<std::string, SShapeRule> rules;

    /* currently active rule, nullptr if none */
    SShapeRule* active = nullptr;

  public:
    /* adds a valid shape rule property */
    void addProperty(std::string key, EShapeRuleType type);

    /* removes currently added shape rules */
    void clear();
    /* adds a new shape rule from string */
    void parseRule(std::string string);

    /* activates the shape rule for the given shape */
    void activate(std::string name);

    std::string getModeOr(std::string def);
    std::string getStringOr(std::string key, std::string def);
    int getIntOr(std::string key, int def);
    float getFloatOr(std::string key, float def);
};

/* method called by hyprland api */
Hyprlang::CParseResult onShapeRuleKeyword(const char* COMMAND, const char* VALUE);

inline std::unique_ptr<CShapeRuleHandler> g_pShapeRuleHandler;
