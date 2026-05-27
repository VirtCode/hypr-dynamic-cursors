#pragma once

#include "IProp.hpp"

#include <hyprland/src/config/shared/Types.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>

using namespace Config::Values;

class CStringProp : public IProp {
    /* underlying configuration variable */
    SP<CStringValue> m_config;

  public:
    CStringProp(SP<CStringValue> config) : IProp(), m_config(config) {}

    /* returns the value of the property, or the default otherwise */
    Config::STRING value() const;

    /* returns the original value as if not overridden */
    Config::STRING original() const;

    virtual const std::type_info* underlying() const override;
    virtual const char*           name() const override;
};
