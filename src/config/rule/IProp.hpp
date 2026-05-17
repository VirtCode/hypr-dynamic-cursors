#pragma once

#include <cstdint>
#include <hyprland/src/helpers/memory/Memory.hpp>

class CShapeRuleHandler;

typedef uint64_t PropID;

/* shape rule prop id generator */
inline PropID g_currentShapeRule = 0;

class IProp {
protected:
    /* pointer to the current shape rule handler */
    WP<CShapeRuleHandler> m_rules;

    /* creates a prop */
    IProp();

public:
    /* unique id of this rule */
    PropID m_id;

    /* sets the handler to get data from */
    void setHandler(WP<CShapeRuleHandler>);

    /* returns the type_info of the underlying type */
    virtual const std::type_info* underlying() const = 0;

    /* returns the name of the property */
    virtual const char* name() const = 0;
};
