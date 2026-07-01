#pragma once

#include "IProp.hpp"

#include <hyprland/src/config/shared/Types.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>

using namespace Config::Values;

class CIntProp : public IProp {
    /* underlying configuration variable */
    SP<CIntValue> m_config;

    /* currently active value */
    Config::INTEGER m_value;

  public:
    CIntProp(SP<CIntValue> config) : IProp(), m_config(config), m_value(config->value()) {}

    /* returns the value of the property, or the default otherwise */
    Config::INTEGER value() const {
        return m_value;
    }

    /* returns the original value as if not overridden */
    Config::INTEGER original() const {
        return m_config->value();
    }

    virtual void                  activate(std::optional<PropValue> value) override;
    virtual const std::type_info* underlying() const override;
    virtual const char*           name() const override;
};
