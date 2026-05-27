#pragma once

#include "IProp.hpp"

#include <hyprland/src/config/shared/Types.hpp>
#include <hyprland/src/config/values/types/BoolValue.hpp>

using namespace Config::Values;

class CBoolProp : public IProp {
    /* underlying configuration variable */
    SP<CBoolValue> m_config;

  public:
    CBoolProp(SP<CBoolValue> config) : IProp(), m_config(config) {}

    /* returns the value of the property, or the default otherwise */
    Config::BOOL value() const;

    /* returns the original value as if not overridden */
    Config::BOOL original() const;

    virtual const std::type_info* underlying() const override;
    virtual const char*           name() const override;
};
