#pragma once

#include "xbase/xns_macro.h"

#include <functional>

NS_BEG3(top, contract_common, properties)

enum class xenum_property_category: std::uint8_t {
    invalid,
    sys_kernel,
    sys_business,
    user,
    all
};
using xproperty_category_t = xenum_property_category;

char category_character(xproperty_category_t const c) noexcept;

NS_END3

#if !defined(XCXX14_OR_ABOVE)
NS_BEG1(std)

template <>
struct hash<top::contract_common::properties::xproperty_category_t> {
    size_t operator()(top::contract_common::properties::xproperty_category_t const property_category) const noexcept;
};

NS_END1
#endif
