#pragma once

#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/config/shared/Types.hpp>
#include <cstdint>
#include <optional>
#include <variant>

typedef uint64_t PropID;

/* the actual types a value can have */
typedef std::variant<Config::STRING, Config::FLOAT, Config::INTEGER, Config::BOOL> PropValue;

/* shape rule prop id generator */
inline PropID g_currentShapeRule = 0;

class IProp {
  protected:
    /* creates a prop */
    IProp();

  public:
    /* unique id of this rule */
    PropID m_id;

    /* applies a rule value, or resets to the default when empty */
    virtual void activate(std::optional<PropValue> value) = 0;

    /* validates the value of a prop, returns error if any */
    virtual std::optional<std::string> validate(std::optional<PropValue> value);

    /* returns the type_info of the underlying type */
    virtual const std::type_info* underlying() const = 0;

    /* returns the name of the property */
    virtual const char* name() const = 0;
};
