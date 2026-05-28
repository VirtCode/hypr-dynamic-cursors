#pragma once

#include <hyprland/src/config/values/types/IValue.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>

#include <optional>
#include <string>

class ICachedValue {
  public:
    virtual ~ICachedValue() = default;

    /* returns the underlying hyprland configuration value */
    virtual WP<Config::Values::IValue> internal() = 0;

    /* recomputes the cached value, returning an error string if any */
    virtual std::optional<std::string> reload() = 0;
};
