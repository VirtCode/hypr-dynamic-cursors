#pragma once

#include "IProp.hpp"

#include <hyprland/src/config/shared/Types.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>

using namespace Config::Values;

class CIntProp : public IProp {
    /* underlying configuration variable */
    SP<CIntValue> m_config;

  public:
    CIntProp(SP<CIntValue> config) : IProp(), m_config(config) {}

    /* returns the value of the property, or the default otherwise */
    Config::INTEGER value() const;

    /* returns the original value as if not overridden */
    Config::INTEGER original() const;

    virtual const std::type_info* underlying() const override;
    virtual const char*           name() const override;
};
