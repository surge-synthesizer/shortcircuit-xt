//
// Created by Paul Walker on 2/5/23.
//

#ifndef SHORTCIRCUIT_SCXT_TRAITS_H
#define SHORTCIRCUIT_SCXT_TRAITS_H

#include "tao/json/traits.hpp"

namespace scxt::json
{
template <typename T> struct scxt_traits : public tao::json::traits<T>
{
};
using scxt_value = tao::json::basic_value<scxt_traits>;
} // namespace scxt::json
#endif // SHORTCIRCUIT_SCXT_TRAITS_H
