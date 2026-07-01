#pragma once

#include "../cache/VariantValue.hpp"
#include "IProp.hpp"

#include <hyprland/src/config/shared/Types.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>

using namespace Config::Values;

class CVariantProp : public IProp {
  protected:
    /* underlying cached value */
    SP<CVariantValue> m_config;

    /* currently active value, taking shape rule overrides into account */
    int m_value;

  public:
    CVariantProp(SP<CVariantValue> config) : IProp(), m_config(config), m_value(config->value()) {}

    /* returns the value of the property, or the default otherwise */
    int value() const {
        return m_value;
    }

    /* returns the original cached value as if not overridden */
    int original() const {
        return m_config->value();
    }

    virtual void                       activate(std::optional<PropValue> value) override;
    virtual std::optional<std::string> validate(std::optional<PropValue> value) override;
    virtual const std::type_info*      underlying() const override;
    virtual const char*                name() const override;
};
