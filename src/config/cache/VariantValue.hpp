#pragma once

#include "ICachedValue.hpp"

#include <hyprland/src/config/values/types/StringValue.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>

#include <string>
#include <unordered_map>

using namespace Config::Values;

class CVariantValue : public ICachedValue {
  protected:
    /* underlying configuration variable */
    SP<CStringValue> m_config;

    /* mapping from config string to int value */
    std::unordered_map<std::string, int> m_map;

    /* currently set int */
    int m_cached = 0;

  public:
    CVariantValue(const char* name, const char* def, const char* desc, std::unordered_map<std::string, int> map);

    /* returns the currently set value */
    int value() const {
        return m_cached;
    }

    /* looks up the given string in the configured map */
    std::optional<int> lookup(const std::string& str) const;

    /* returns a string containing all variants */
    std::string formatVariants() const;

    virtual WP<IValue>                 internal() override;
    virtual std::optional<std::string> reload() override;
};
