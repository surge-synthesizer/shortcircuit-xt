//
// Created by Paul Walker on 2/5/23.
//

#ifndef SHORTCIRCUIT_CLIENT_JSON_DETAILS_H
#define SHORTCIRCUIT_CLIENT_JSON_DETAILS_H

#include "tao/json/traits.hpp"

namespace scxt::messaging::client::details
{
template <typename T> struct client_message_traits : public tao::json::traits<T>
{
};
using client_message_value = tao::json::basic_value<client_message_traits>;
} // namespace scxt::messaging::client::details
#endif // SHORTCIRCUIT_CLIENT_JSON_DETAILS_H
