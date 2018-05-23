#ifndef WATER_COMPONET_TOOLS_H
#define WATER_COMPONET_TOOLS_H


#include "string_kit.h"


template <typename E>
constexpr auto underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}


#endif

